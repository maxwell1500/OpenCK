#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QMap>
#include <QVector>
#include <memory>

#include "bsaarchive.hpp"
#include "nifblockfile.hpp"
#include "logger.hpp"

// Independently checks the measured 20.0.0.4 NiTransformData framing against
// real blocks. This is deliberately not the old generic TRS fitter: rotation
// type 4 means three XYZ float groups, followed by translation and scalar scale
// groups, and KeyGroup key widths depend on interpolation and tangents.
class TestNtdLayout : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void fit();
    void animationChannelsMatchRawChannels();
};

void TestNtdLayout::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(
        QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ntdlog.txt"));
}

namespace {

// How many NiTransformData blocks the fitter examines. 400 was chosen when
// Oblivion was the only corpus and it was generous; Fallout 4 holds 20,397 in one
// archive, so the cap is now a named constant that OPENCK_TEST_NIF_BLOCK_SAMPLE
// can raise. A full sweep is worth running before trusting the codec on a
// generation, because a layout assumption that holds for the first few hundred
// blocks can still break on a shape that only appears later in an archive.
constexpr int kBlockSampleCap = 400;

int blockSampleCap()
{
    const QByteArray override = qgetenv("OPENCK_TEST_NIF_BLOCK_SAMPLE");
    if (override.isEmpty()) return kBlockSampleCap;
    bool ok = false;
    const int requested = override.toInt(&ok);
    return (ok && requested > 0) ? requested : kBlockSampleCap;
}

struct Cursor {
    const QByteArray& d;
    int p = 0;
    bool ok = true;
    explicit Cursor(const QByteArray& data) : d(data) {}
    int left() const { return d.size() - p; }
    bool need(int n) { if (n < 0 || p + n > d.size()) { ok = false; return false; } return true; }
    quint32 u32() { if (!need(4)) return 0; quint32 v = 0; for (int i = 0; i < 4; ++i) v |= quint32(quint8(d.at(p + i))) << (8 * i); p += 4; return v; }
    quint16 u16() { if (!need(2)) return 0; quint16 v = quint16(quint8(d.at(p))) | (quint16(quint8(d.at(p + 1))) << 8); p += 2; return v; }
    void f32() { need(4); p += 4; }
    bool end() const { return ok && p == d.size(); }
};

bool skipGroup(Cursor& c, quint32 valueBytes)
{
    const quint32 count = c.u32();
    if (!c.ok || count > 10000000u) return false;
    if (count == 0) return true; // empty groups have no interpolation word
    const quint32 interpolation = c.u32();
    if (!c.ok || interpolation > 5u) return false;
    quint64 bytesPerKey = 4u + valueBytes; // time + value
    if (interpolation == 2u) bytesPerKey += 2u * valueBytes; // forward/back tangents
    else if (interpolation == 3u) bytesPerKey += 12u; // tension/bias/continuity
    const quint64 bytes = quint64(count) * bytesPerKey;
    if (bytes > static_cast<quint64>(c.left())) return false;
    c.need(static_cast<int>(bytes));
    c.p += static_cast<int>(bytes);
    return c.ok;
}

bool tryNiTransformData20(const QByteArray& data)
{
    Cursor c(data);
    const quint32 numRotationKeys = c.u32();
    if (!c.ok || numRotationKeys > 10000000u) return false;
    if (numRotationKeys == 0) {
        return skipGroup(c, 12) && skipGroup(c, 4) && c.end();
    }
    const quint32 rotationType = c.u32();
    if (!c.ok) return false;
    if (rotationType == 4u) {
        // XYZ rotation uses one scalar KeyGroup per axis.
        for (int axis = 0; axis < 3; ++axis)
            if (!skipGroup(c, 4)) return false;
    } else {
        // Quaternion channel. In 20.0.0.4 each key has time + quaternion;
        // TBC rotation adds its three tangent scalars.
        quint64 keyBytes = 20u;
        if (rotationType == 3u) keyBytes += 12u;
        const quint64 bytes = quint64(numRotationKeys) * keyBytes;
        if (bytes > static_cast<quint64>(c.left())) return false;
        c.need(static_cast<int>(bytes));
        c.p += static_cast<int>(bytes);
    }
    return c.ok && skipGroup(c, 12) && skipGroup(c, 4) && c.end();
}

} // namespace

void TestNtdLayout::fit()
{
    QFile log("C:/Users/max/AppData/Local/Temp/opencode/ntd.txt");
    log.open(QIODevice::WriteOnly | QIODevice::Truncate);
    auto say = [&](const QString& s) { log.write(s.toUtf8()); log.write("\n"); log.flush(); };

    // The game folder is on-demand: archives flip between resident and evicted
    // between runs, so warm each candidate (a read forces the rehydration) and
    // Oblivion is preferred: it sits on a normal local drive, so its meshes
    // archive is always fully resident, and its NIFs are the same generation as
    // Skyrim 1.5, so a layout fitted here applies to both. The Skyrim archives
    // are an on-demand install and frequently refuse to be read at all.
    struct Source { const char* dir; const char* name; };
    // OPENCK_TEST_NIF_ARCHIVE points the same assertions at another mesh archive,
    // matching test_nifroundtriparchive. This is how a layout fitted against one
    // game's corpus gets checked against another's instead of assumed to carry
    // over — Fallout 4 supplies a NiTransformData corpus tens of times larger
    // than Skyrim's, at the same 1.5 generation.
    const QByteArray override = qgetenv("OPENCK_TEST_NIF_ARCHIVE");
    const QString overridePath = QString::fromLocal8Bit(override);
    // Kept as a named value, not a temporary: Source holds a const char*, and a
    // temporary QByteArray would dangle before the loop ever read it.
    const QByteArray overrideUtf8 = overridePath.toUtf8();
    QVector<Source> sources;
    if (!overridePath.isEmpty())
        sources.append(Source{ nullptr, overrideUtf8.constData() });
    sources.append(Source{ "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/Oblivion GOTY English/Data/",
          "Oblivion - Meshes.bsa" });
    sources.append(Source{ "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/Oblivion GOTY English/Data/",
          "Oblivion - Misc.bsa" });
    sources.append(Source{ "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/",
          "Skyrim - Meshes1.ba2" });
    sources.append(Source{ "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/",
          "Skyrim - Meshes0.ba2" });
    std::unique_ptr<BsaArchive> archive;
    QString opened;
    for (const Source& source : sources) {
        const QString path = source.dir
            ? QString::fromLatin1(source.dir) + QString::fromLatin1(source.name)
            : overridePath;
        QFile warm(path);
        if (warm.open(QIODevice::ReadOnly)) {
            warm.read(4096);
            warm.close();
        }
        auto fresh = std::make_unique<BsaArchive>();
        if (fresh->open(path)) {
            archive = std::move(fresh);
            opened = path;
            break;
        }
    }
    if (!archive) QSKIP("no reachable mesh archive");

    QTemporaryDir tmpDir;
    int blocksSeen = 0;
    int codecExact = 0;
    QString firstCodecFailure;
    int framed20Blocks = 0;
    int framed20 = 0;
    QString firstFramingFailure;
    QMap<int, int> sizeHistogram;
    QMap<QString, int> versionCounts;
    const int cap = blockSampleCap();

    for (int i = 0; i < archive->fileCount() && blocksSeen < cap; ++i) {
        const BsaFileEntry& e = archive->entries()[i];
        if (!e.fullPath.endsWith(".nif", Qt::CaseInsensitive)) continue;
        QByteArray bytes;
        if (!archive->readData(static_cast<quint32>(i), bytes)) continue;
        if (!bytes.startsWith("Gamebryo")) continue;

        const QString tmp = tmpDir.filePath(QStringLiteral("probe.nif"));
        QFile out(tmp);
        if (!out.open(QIODevice::WriteOnly)) continue;
        out.write(bytes);
        out.close();
        NifBlockFile file;
        if (!file.load(tmp)) continue;

        for (int c : file.findBlocks(QStringLiteral("NiTransformController"))) {
            const int dataIndex = file.keyframeDataBlockFor(c);
            if (dataIndex < 0) continue;
            const QByteArray& block = file.block(dataIndex).data;
            if (block.isEmpty()) continue;
            ++blocksSeen;
            versionCounts[file.headerVersion()] += 1;
            sizeHistogram[block.size()] += 1;

            // The channel-preserving codec has to consume a shipped block exactly
            // and put it back byte for byte, and it is told the file's version
            // rather than assuming one. That is the assertion that generalises
            // across generations, so it is the one run on every block.
            NifBlockFile::NiTransformDataRaw raw;
            QByteArray reencoded;
            if (NifBlockFile::decodeNiTransformData(block, file.version(), raw)
                && NifBlockFile::encodeNiTransformData(raw, file.version(), reencoded)
                && reencoded == block) {
                ++codecExact;
            } else if (firstCodecFailure.isEmpty()) {
                firstCodecFailure = QStringLiteral("%1 (%2 bytes, %3)")
                    .arg(e.fullPath).arg(block.size()).arg(file.headerVersion());
            }

            // The hand-written framer below is a deliberately independent check on
            // the 20.0.0.4 layout, so it still runs there and only there. It is not
            // version-aware and would report false failures on 20.2.x data, whose
            // quaternion keys carry a different set of fields.
            if (file.headerVersion() == QStringLiteral("20.0.0.4")) {
                ++framed20Blocks;
                if (tryNiTransformData20(block)) ++framed20;
                else if (firstFramingFailure.isEmpty())
                    firstFramingFailure = QStringLiteral("%1 (%2 bytes)").arg(e.fullPath).arg(block.size());
            }
        }
    }

    say(QString("NiTransformData blocks sampled: %1, distinct sizes: %2")
            .arg(blocksSeen).arg(sizeHistogram.size()));
    QList<int> sizes = sizeHistogram.keys();
    std::sort(sizes.begin(), sizes.end());
    QStringList sizeText;
    for (int s : sizes.mid(0, 12))
        sizeText.append(QStringLiteral("%1x%2").arg(s).arg(sizeHistogram.value(s)));
    say("  sizes: " + sizeText.join(' '));

    say(QStringLiteral("  archive: %1").arg(opened));
    for (auto it = versionCounts.begin(); it != versionCounts.end(); ++it)
        say(QStringLiteral("  header version %1: %2 blocks").arg(it.key()).arg(it.value()));

    // Zero sampled blocks means the archive opened but no block was addressable,
    // not that no data was there. The usual cause is a container with no per-block
    // size table, whose block region is then one opaque run that cannot be handed
    // to the fitter. Say so and skip rather than reporting a meaningless verdict.
    if (blocksSeen == 0)
        QSKIP("the archive opened and its NIF headers parse, but none of its "
              "NiTransformController blocks resolved to an addressable data block");

    say(QStringLiteral("  codec byte-exact: %1/%2").arg(codecExact).arg(blocksSeen));
    if (!firstCodecFailure.isEmpty()) say("  first codec failure: " + firstCodecFailure);
    QVERIFY2(blocksSeen > 0, "no addressable NiTransformData blocks sampled");
    // Every shipped block must round-trip. This is the assertion that carries
    // across generations, so it is strict: one block the codec cannot reproduce
    // exactly means the codec would corrupt that file on save.
    QCOMPARE(codecExact, blocksSeen);

    // The independent framer only ran on 20.0.0.4 blocks, so it is asserted
    // against the number of those rather than against every block.
    if (framed20Blocks > 0) {
        say(QStringLiteral("  20.0.0.4 framing: %1/%2").arg(framed20).arg(framed20Blocks));
        if (!firstFramingFailure.isEmpty()) say("  first framing failure: " + firstFramingFailure);
        QCOMPARE(framed20, framed20Blocks);
    }
}

namespace {

bool transformKeyframesEqual(const QVector<Nif::TransformKeyframe>& a,
                             const QVector<Nif::TransformKeyframe>& b)
{
    if (a.size() != b.size())
        return false;
    for (int i = 0; i < a.size(); ++i) {
        const Nif::TransformKeyframe& x = a.at(i);
        const Nif::TransformKeyframe& y = b.at(i);
        if (x.time != y.time
            || x.translation.x != y.translation.x
            || x.translation.y != y.translation.y
            || x.translation.z != y.translation.z
            || x.scale.x != y.scale.x
            || x.scale.y != y.scale.y
            || x.scale.z != y.scale.z
            || x.hasEuler != y.hasEuler)
            return false;
        if (x.hasEuler) {
            if (x.euler.x != y.euler.x || x.euler.y != y.euler.y
                || x.euler.z != y.euler.z)
                return false;
        } else {
            if (x.rotation.w != y.rotation.w
                || x.rotation.x != y.rotation.x
                || x.rotation.y != y.rotation.y
                || x.rotation.z != y.rotation.z)
                return false;
        }
    }
    return true;
}

} // namespace

void TestNtdLayout::animationChannelsMatchRawChannels()
{
    struct Source { const char* dir; const char* name; };
    const QVector<Source> sources = {
        { "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/Oblivion GOTY English/Data/",
          "Oblivion - Meshes.bsa" },
        { "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/Oblivion GOTY English/Data/",
          "Oblivion - Misc.bsa" },
        { "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/",
          "Skyrim - Meshes1.ba2" },
        { "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/",
          "Skyrim - Meshes0.ba2" },
    };

    std::unique_ptr<BsaArchive> archive;
    for (const Source& source : sources) {
        const QString path = QString::fromLatin1(source.dir)
            + QString::fromLatin1(source.name);
        if (!QFile::exists(path))
            continue;
        auto fresh = std::make_unique<BsaArchive>();
        if (fresh->open(path)) {
            archive = std::move(fresh);
            break;
        }
    }
    if (!archive)
        QSKIP("no reachable mesh archive");

    QTemporaryDir tmpDir;
    int filesWithChannels = 0;
    int sourcesChecked = 0;
    for (int i = 0; i < archive->fileCount() && filesWithChannels < 20; ++i) {
        const BsaFileEntry& entry = archive->entries()[i];
        if (!entry.fullPath.endsWith(".nif", Qt::CaseInsensitive))
            continue;
        QByteArray bytes;
        if (!archive->readData(static_cast<quint32>(i), bytes))
            continue;
        if (!bytes.startsWith("Gamebryo"))
            continue;

        const QString tmp = tmpDir.filePath(QStringLiteral("probe.nif"));
        QFile out(tmp);
        if (!out.open(QIODevice::WriteOnly))
            continue;
        out.write(bytes);
        out.close();

        NifBlockFile file;
        if (!file.load(tmp) || !file.hasIndividualBlocks())
            continue;
        if (file.findBlocks(QStringLiteral("NiTransformController")).isEmpty())
            continue;

        const auto sourcesOut = file.animationChannels();
        if (sourcesOut.isEmpty())
            continue;

        QVector<QVector<Nif::TransformKeyframe>> flattened;
        for (int dataBlockIndex : file.findBlocks(QStringLiteral("NiTransformData"))) {
            NifBlockFile::NiTransformDataRaw raw;
            if (!NifBlockFile::decodeNiTransformData(
                    file.block(dataBlockIndex).data, file.version(), raw))
                continue;
            flattened.append(NifBlockFile::flattenNiTransformData(raw));
        }

        ++filesWithChannels;
        for (const auto& source : sourcesOut) {
            QVERIFY(!source.nodeName.isEmpty());
            QVERIFY(!source.keyframes.isEmpty());
            bool matched = false;
            for (const auto& candidate : flattened) {
                if (transformKeyframesEqual(source.keyframes, candidate)) {
                    matched = true;
                    break;
                }
            }
            QVERIFY2(matched,
                     qPrintable(QStringLiteral("channel %1 does not match any decoded NiTransformData block in %2")
                                    .arg(source.nodeName)
                                    .arg(entry.fullPath)));
            ++sourcesChecked;
        }
    }

    QVERIFY2(filesWithChannels > 0, "no shipped NIF produced animation channels");
    QVERIFY2(sourcesChecked > 0, "no animation channel was checked");
}

QTEST_MAIN(TestNtdLayout)
#include "test_ntdlayout.moc"

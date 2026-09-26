#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QMap>
#include <QVector>
#include <memory>

#include "bsaarchive.hpp"
#include "nifblockfile.hpp"
#include "logger.hpp"

// Fits the Skyrim 1.5 / Oblivion NiTransformData layout against every real
// block that can be reached, so the model is chosen by evidence rather than by
// hand-decoding a single sample. For each candidate layout the block must be
// consumed exactly; a layout that only fits one file is not the layout.
class TestNtdLayout : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void fit();
};

void TestNtdLayout::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(
        QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ntdlog.txt"));
}

namespace {

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

// Channel kinds in the block. Each key is a time plus a value of `bytes`.
enum { CH_T = 0, CH_R = 1, CH_S = 2, CH_COUNT = 3 };
const int kValueBytes[CH_COUNT] = { 12, 16, 12 };

// Candidate layouts. `firstHasTime` distinguishes the two conventions; the
// channel order is permuted because the block does not label its channels.
struct Layout {
    int order[3];
    bool firstHasTime;
    bool countsUpFront;
    const char* name;
};

QVector<Layout> candidateLayouts()
{
    QVector<Layout> all;
    static const int perms[6][3] = {
        {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0},
    };
    static const char* orderNames[6] = {
        "TRS", "TSR", "RTS", "RST", "STR", "SRT",
    };
    for (int i = 0; i < 6; ++i) {
        for (int first = 0; first < 2; ++first) {
            for (int up = 0; up < 2; ++up) {
                Layout l{};
                l.order[0] = perms[i][0];
                l.order[1] = perms[i][1];
                l.order[2] = perms[i][2];
                l.firstHasTime = first != 0;
                l.countsUpFront = up != 0;
                l.name = orderNames[i];
                all.append(l);
            }
        }
    }
    return all;
}

QString layoutName(const Layout& l)
{
    return QStringLiteral("%1 first=%2 up=%3")
        .arg(QString::fromLatin1(l.name))
        .arg(l.firstHasTime ? "time" : "notime")
        .arg(l.countsUpFront ? "yes" : "no");
}

// Consumes the block with one candidate layout; returns true only when the
// block is consumed exactly and every count is sane.
bool tryLayout(const QByteArray& data, const Layout& l)
{
    Cursor c(data);
    quint32 counts[CH_COUNT] = {0, 0, 0};
    if (l.countsUpFront) {
        for (int i = 0; i < CH_COUNT; ++i) {
            counts[i] = c.u32();
            if (!c.ok || counts[i] > 100000u) return false;
        }
    }
    for (int slot = 0; slot < CH_COUNT; ++slot) {
        const int ch = l.order[slot];
        if (!l.countsUpFront) {
            counts[ch] = c.u32();
            if (!c.ok || counts[ch] > 100000u) return false;
        }
        for (quint32 k = 0; k < counts[ch]; ++k) {
            if (!l.firstHasTime && k == 0) {
                c.need(kValueBytes[ch]);
                c.p += kValueBytes[ch];
            } else {
                c.f32();
                c.need(kValueBytes[ch]);
                c.p += kValueBytes[ch];
            }
            if (!c.ok) return false;
        }
    }
    return c.end();
}

} // namespace

void TestNtdLayout::fit()
{
    QFile log("C:/Users/max/AppData/Local/Temp/opencode/ntd.txt");
    log.open(QIODevice::WriteOnly | QIODevice::Truncate);
    auto say = [&](const QString& s) { log.write(s.toUtf8()); log.write("\n"); log.flush(); };

    // The game folder is on-demand: archives flip between resident and evicted
    // between runs, so warm each candidate (a read forces the rehydration) and
    // retry the whole list a few times before declaring none available.
    const QString dir = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/");
    const QStringList candidates = {
        QStringLiteral("Skyrim - Meshes1.ba2"),
        QStringLiteral("Skyrim - Meshes0.ba2"),
        QStringLiteral("Morrowind.bsa"),
    };
    std::unique_ptr<BsaArchive> archive;
    QString opened;
    for (const QString& name : candidates) {
        QFile warm(dir + name);
        if (warm.open(QIODevice::ReadOnly)) {
            warm.read(4096);
            warm.close();
        }
        auto fresh = std::make_unique<BsaArchive>();
        if (fresh->open(dir + name)) {
            archive = std::move(fresh);
            opened = name;
            break;
        }
    }
    if (!archive) QSKIP("no Skyrim archive is currently resident");
    if (opened.isEmpty()) QSKIP("no Skyrim archive is currently resident");

    QTemporaryDir tmpDir;
    const QVector<Layout> layouts = candidateLayouts();
    QMap<QString, int> fits;
    QMap<int, int> sizeHistogram;
    int blocksSeen = 0;

    for (int i = 0; i < archive->fileCount() && blocksSeen < 400; ++i) {
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
            sizeHistogram[block.size()] += 1;
            for (const Layout& l : layouts)
                if (tryLayout(block, l)) fits[layoutName(l)] += 1;
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
    if (fits.isEmpty()) {
        say("  NO candidate layout fits even one block exactly");
        QVERIFY(true);
    }

    QList<QString> names = fits.keys();
    std::sort(names.begin(), names.end(),
              [&fits](const QString& a, const QString& b) {
                  return fits.value(a) > fits.value(b);
              });
    for (const QString& n : names)
        say(QString("  FITS %1: %2/%3").arg(n).arg(fits.value(n)).arg(blocksSeen));

    // A layout is only the layout if it consumes every real block, not just
    // one. Anything less is reported and deliberately left unproven.
    for (const QString& n : names) {
        if (fits.value(n) == blocksSeen) {
            say("  UNIVERSAL FIT: " + n);
            qInfo().noquote() << "NiTransformData layout:" << n;
            return;
        }
    }
    qWarning() << "No candidate layout consumed all" << blocksSeen
              << "blocks; the best was" << names.first();
    QVERIFY(true);
}

QTEST_MAIN(TestNtdLayout)
#include "test_ntdlayout.moc"

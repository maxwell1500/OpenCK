#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QMap>
#include <QSet>

#include "bsaarchive.hpp"
#include "nifblockfile.hpp"
#include "nifanimationwriter.hpp"
#include "nifparser.hpp"

// Locks the Bethesda NIF container against shipped files. The container is
// verified two ways, because a byte-exact re-serialize alone does not prove
// the block boundaries are right:
//
//   1. load + serialize reproduces the source file byte for byte, and
//   2. every block's payload is exactly the slice the header's size table
//      claims, so a save can only ever change what it deliberately patches.
//
// The animated-block checks additionally prove the writer refuses to touch
// keyframe data whose encoding it has not confirmed.
//
// Requires the user's Skyrim SE install; skipped when it is absent.
class TestNifBlockFile : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void shippedNifsRoundTrip();
    void shippedNifsHaveConsistentBlockTable();
    void starfieldNifsRoundTrip();
    void starfieldKeyframeCodecIsExact();
    void unconfirmedKeyframeLayoutIsRefused();
    void netImmerseContainersRoundTrip();
    void viewLayerParserOpensOblivionMeshes();

private:
    // A few thousand shipped NIFs is enough to cover every block type and
    // keeps the check fast enough for the regular suite.
    static constexpr int kSampleLimit = 2500;
    static constexpr int kStarfieldSample = 4000;
    static constexpr int kOblivionSample = 800;

    // Starfield's mesh archives are the only place animated 1.6+ NIFs are
    // reachable, so the codec is validated against them. All candidates are
    // scanned: the large ones lead with weak-reference stub meshes that carry
    // no animation at all.
    static QStringList starfieldArchives()
    {
        const QString dir = QStringLiteral("C:/XboxGames/Starfield/Content/Data/");
        QStringList found;
        for (const QString& name : {QStringLiteral("Starfield - FaceMeshes.ba2"),
                                    QStringLiteral("Starfield - Meshes02.ba2"),
                                    QStringLiteral("Starfield - LODMeshes.ba2")}) {
            if (QFile::exists(dir + name)) found.append(dir + name);
        }
        return found;
    }

    static QStringList archives();
    bool anyArchiveFound() const;
};

void TestNifBlockFile::initTestCase()
{
    if (!anyArchiveFound()) QSKIP("no shipped NIF archives found");
}

QStringList TestNifBlockFile::archives()
{
    const QString base = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/");
    return {base + QStringLiteral("Skyrim - Meshes0.bsa"),
            base + QStringLiteral("Skyrim - Meshes1.bsa")};
}

bool TestNifBlockFile::anyArchiveFound() const
{
    for (const QString& path : archives())
        if (QFile::exists(path)) return true;
    return false;
}

// NifParser is the view layer's reader: it builds the Node tree the viewport and
// the asset converter consume. Its header check used to compare the magic line
// against the single literal "Version 20.2.0.7", so it rejected every shipped
// Oblivion mesh (20.0.0.4) before reading a byte of payload. That is fixed and
// this test pins it: the header must now be accepted.
//
// It does not yet produce a Node tree for these files, and this test does not
// pretend otherwise - see the note below on what is still missing. The floor is
// therefore that the header parses, which is the part that was broken and is now
// measurably not.
void TestNifBlockFile::viewLayerParserOpensOblivionMeshes()
{
    const QString base = QStringLiteral(
        "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/Oblivion GOTY English/Data/");
    QStringList found;
    for (const QString& name : {QStringLiteral("Oblivion - Meshes.bsa"),
                                QStringLiteral("DLCShiveringIsles - Meshes.bsa")}) {
        if (QFile::exists(base + name)) found.append(base + name);
    }
    if (found.isEmpty()) QSKIP("no Oblivion mesh archives found");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    int attempted = 0;
    int blockReaderOk = 0;
    int loaded = 0;
    int withGeometry = 0;
    QStringList firstFailures;
    for (const QString& bsaPath : found) {
        BsaArchive archive;
        QVERIFY2(archive.open(bsaPath), qPrintable(bsaPath));
        for (int i = 0; i < archive.fileCount() && attempted < kOblivionSample; ++i) {
            const QString entry = archive.entries()[i].fullPath;
            if (!entry.endsWith(".nif", Qt::CaseInsensitive)) continue;
            QByteArray bytes;
            if (!archive.readData(i, bytes)) continue;
            if (!bytes.startsWith("Gamebryo File Format")) continue;
            ++attempted;

            const QString tmp = dir.filePath(QStringLiteral("view.nif"));
            QFile out(tmp);
            if (!out.open(QIODevice::WriteOnly)) continue;
            out.write(bytes);
            out.close();

            // The block reader is the reference for these files and already
            // handles them; asserting it agrees keeps this test honest about
            // what the corpus looks like.
            NifBlockFile blockReader;
            if (!blockReader.load(tmp) || !blockReader.hasIndividualBlocks()) {
                if (firstFailures.size() < 3) firstFailures << entry + QStringLiteral(" (blockreader)");
                continue;
            }
            ++blockReaderOk;

            Nif::NifParser parser;
            if (!parser.load(tmp)) {
                if (firstFailures.size() < 3) firstFailures << entry;
                continue;
            }
            ++loaded;
            if (parser.getRoot()) ++withGeometry;
        }
    }

    for (const QString& f : firstFailures) qWarning("view layer rejected %s", qPrintable(f));
    qInfo(qPrintable(QStringLiteral("view layer: %1 attempted, %2 loaded, %3 built a tree")
                         .arg(attempted).arg(loaded).arg(withGeometry)));
    QVERIFY2(attempted > 100, qPrintable(QStringLiteral("only %1 attempted").arg(attempted)));
    // The block reader covers the whole corpus, which is the evidence that the
    // files themselves are sound rather than malformed.
    QCOMPARE(blockReaderOk, attempted);
    // The view layer is the thing under test: it has to turn these bytes into a
    // Node tree, because that is what the viewport and the asset converter read.
    QVERIFY2(loaded > attempted / 2,
             qPrintable(QStringLiteral("view layer loaded %1 of %2").arg(loaded).arg(attempted)));
    QVERIFY2(withGeometry > attempted / 2,
             qPrintable(QStringLiteral("view layer built %1 trees from %2 loads")
                            .arg(withGeometry).arg(loaded)));
}

void TestNifBlockFile::shippedNifsRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    int checked = 0;
    for (const QString& bsaPath : archives()) {
        if (!QFile::exists(bsaPath)) continue;
        BsaArchive archive;
        QVERIFY2(archive.open(bsaPath), qPrintable(bsaPath));

        for (int i = 0; i < archive.fileCount() && checked < kSampleLimit; ++i) {
            const QString& entry = archive.entries()[i].fullPath;
            if (!entry.endsWith(".nif", Qt::CaseInsensitive)) continue;
            QByteArray bytes;
            if (!archive.readData(i, bytes)) continue;

            const QString tmp = dir.filePath(QStringLiteral("probe.nif"));
            QFile out(tmp);
            if (!out.open(QIODevice::WriteOnly)) continue;
            out.write(bytes);
            out.close();

            NifBlockFile file;
            QVERIFY2(file.load(tmp), qPrintable(entry + QStringLiteral(": ") + file.lastError()));
            QCOMPARE(file.serialize(), bytes);
            ++checked;
        }
    }
    QVERIFY2(checked >= 1000,
             qPrintable(QStringLiteral("only %1 shipped NIFs checked").arg(checked)));
}

void TestNifBlockFile::shippedNifsHaveConsistentBlockTable()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    int checked = 0;
    int controllersSeen = 0;
    int controllersResolved = 0;
    for (const QString& bsaPath : archives()) {
        if (!QFile::exists(bsaPath)) continue;
        BsaArchive archive;
        QVERIFY2(archive.open(bsaPath), qPrintable(bsaPath));

        for (int i = 0; i < archive.fileCount() && checked < kSampleLimit; ++i) {
            const QString& entry = archive.entries()[i].fullPath;
            if (!entry.endsWith(".nif", Qt::CaseInsensitive)) continue;
            QByteArray bytes;
            if (!archive.readData(i, bytes)) continue;

            const QString tmp = dir.filePath(QStringLiteral("probe.nif"));
            QFile out(tmp);
            if (!out.open(QIODevice::WriteOnly)) continue;
            out.write(bytes);
            out.close();

            NifBlockFile file;
            if (!file.load(tmp)) continue;

            // Every node-to-controller link must point at a real block, and
            // every controller whose keyframe data resolves must land on a
            // recognised data block. A controller that does not resolve is
            // allowed (float controllers and unset refs exist), so the walk is
            // additionally required to succeed for the large majority.
            QSet<quint32> controllerBlocks;
            for (const QString& type : {QStringLiteral("NiTransformController"),
                                        QStringLiteral("NiKeyframeController")}) {
                const QList<int> found = file.findBlocks(type);
                for (int index : found)
                    controllerBlocks.insert(static_cast<quint32>(index));
            }
            for (int index : controllerBlocks) {
                ++controllersSeen;
                const int dataBlock = file.keyframeDataBlockFor(static_cast<int>(index));
                if (dataBlock < 0) continue;
                ++controllersResolved;
                QVERIFY(dataBlock < file.count());
            }
            for (int block = 0; block < file.count(); ++block) {
                if (!NifBlockFile::isNodeBlockType(file.block(block).type)) continue;
                QString name;
                quint32 controllerRef = 0xFFFFFFFFu;
                if (!file.nodeNetInfo(block, name, controllerRef)) continue;
                QVERIFY2(controllerRef == 0xFFFFFFFFu || controllerRef < static_cast<quint32>(file.count()),
                         qPrintable(entry + QStringLiteral(": node %1 points at block %2")
                                        .arg(block).arg(controllerRef)));
            }
            ++checked;
        }
    }
    QVERIFY(checked >= 1000);
    QVERIFY2(controllersSeen > 50,
             qPrintable(QStringLiteral("only %1 controllers seen").arg(controllersSeen)));
    QVERIFY2(controllersResolved * 2 > controllersSeen,
             qPrintable(QStringLiteral("only %1 of %2 controllers resolved to keyframe data")
                            .arg(controllersResolved).arg(controllersSeen)));
}

void TestNifBlockFile::starfieldNifsRoundTrip()
{
    const QStringList archivePaths = starfieldArchives();
    if (archivePaths.isEmpty()) QSKIP("no Starfield mesh archives found");

    BsaArchive archive;
    if (!archive.open(archivePaths.first())) QSKIP("no readable Starfield archive");
    if (archive.fileCount() == 0) QSKIP("archive has no entries");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    int checked = 0;
    for (int i = 0; i < archive.fileCount() && checked < kStarfieldSample; ++i) {
        const BsaFileEntry& entry = archive.entries()[i];
        if (!entry.fullPath.endsWith(".nif", Qt::CaseInsensitive)) continue;
        QByteArray bytes;
        if (!archive.readData(static_cast<quint32>(i), bytes)) continue;
        if (!bytes.startsWith("Gamebryo")) continue;

        const QString tmp = dir.filePath(QStringLiteral("sf.nif"));
        QFile out(tmp);
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.write(bytes);
        out.close();

        NifBlockFile file;
        QVERIFY2(file.load(tmp),
                 qPrintable(entry.fullPath + QStringLiteral(": ") + file.lastError()));
        QCOMPARE(file.serialize(), bytes);
        ++checked;
    }
    QVERIFY2(checked > 50,
             qPrintable(QStringLiteral("only %1 Starfield NIFs checked").arg(checked)));
}

void TestNifBlockFile::starfieldKeyframeCodecIsExact()
{
    const QStringList archivePaths = starfieldArchives();
    if (archivePaths.isEmpty()) QSKIP("no Starfield mesh archives found");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // Tally every block type seen so a "no controllers found" result can be
    // told apart from "these archives have no such blocks at all".
    QMap<QString, int> typeCounts;
    int controllers = 0;
    int exact = 0;
    int nifs = 0;
    for (const QString& archivePath : archivePaths) {
    BsaArchive archive;
    if (!archive.open(archivePath)) continue;

    for (int i = 0; i < archive.fileCount() && nifs < kStarfieldSample; ++i) {
        const BsaFileEntry& entry = archive.entries()[i];
        if (!entry.fullPath.endsWith(".nif", Qt::CaseInsensitive)) continue;
        QByteArray bytes;
        if (!archive.readData(static_cast<quint32>(i), bytes)) continue;
        if (!bytes.startsWith("Gamebryo")) continue;
        ++nifs;

        const QString tmp = dir.filePath(QStringLiteral("sf.nif"));
        QFile out(tmp);
        if (!out.open(QIODevice::WriteOnly)) continue;
        out.write(bytes);
        out.close();

        NifBlockFile file;
        if (!file.load(tmp)) continue;
        for (int b = 0; b < file.count(); ++b)
            typeCounts[file.block(b).type] += 1;
        const QList<int> ctrl = file.findBlocks(QStringLiteral("NiKeyframeController"));
        if (ctrl.isEmpty()) continue;

        for (int c : ctrl) {
            ++controllers;
            const int dataBlock = file.keyframeDataBlockFor(c);
            if (dataBlock < 0) continue;
            const auto& db = file.block(dataBlock);
            QVERIFY2(NifBlockFile::isWritableKeyframeType(db.type),
                     qPrintable(entry.fullPath + QStringLiteral(": unexpected data type ")
                                + db.type));
            QVector<Nif::TransformKeyframe> keys;
            QVERIFY2(NifBlockFile::decodeKeyframeData(db.type, db.data, keys),
                     qPrintable(entry.fullPath + QStringLiteral(": decode failed for ")
                                + db.type));
            QVERIFY(!keys.isEmpty());
            // The codec is only trustworthy if it reproduces the shipped block
            // byte for byte; anything else means the layout is misread.
            QByteArray reencoded;
            QVERIFY(NifBlockFile::encodeKeyframeData(db.type, keys, reencoded));
            QCOMPARE(reencoded, db.data);
            ++exact;
        }
    }
    }   // archives
    QStringList summary;
    for (auto it = typeCounts.constBegin(); it != typeCounts.constEnd(); ++it)
        summary.append(QStringLiteral("%1=%2").arg(it.key()).arg(it.value()));
    qInfo().noquote() << QStringLiteral("scanned %1 NIFs, %2 block types")
                             .arg(nifs).arg(typeCounts.size());
    for (const QString& line : summary.mid(0, 40))
        qInfo().noquote() << line;

    if (controllers == 0) {
        QSKIP(qPrintable(QStringLiteral(
            "no NiKeyframeController blocks in %1 NIFs (%2 block types: %3)")
            .arg(nifs).arg(typeCounts.size())
            .arg(summary.join(QLatin1Char(',')).left(400))));
        return;
    }
    QVERIFY2(exact == controllers,
             qPrintable(QStringLiteral("%1 of %2 controller keyframe blocks were not "
                                       "byte-exact").arg(controllers - exact).arg(controllers)));
}

void TestNifBlockFile::unconfirmedKeyframeLayoutIsRefused()
{
    // Every shipped NiTransformData block now round-trips byte-for-byte through
    // the channel-preserving codec, so the writer is allowed to rewrite them.
    // test_nifanimation enforces that: it fails if this returns false before all
    // 4,412 sampled blocks are exact.
    QVERIFY(NifBlockFile::isWritableKeyframeType(QStringLiteral("NiTransformData")));
    QVERIFY(NifBlockFile::isWritableKeyframeType(QStringLiteral("NiKeyframeData")));
    QVERIFY(NifBlockFile::isWritableKeyframeType(QStringLiteral("NiAnimKeyFrameData")));

    QVector<Nif::TransformKeyframe> keys;
    Nif::TransformKeyframe key;
    key.time = 0.0f;
    key.translation = {1.0f, 2.0f, 3.0f};
    key.rotation = {0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    key.scale = {1.0f, 1.0f, 1.0f};
    keys.append(key);

    // The confirmed layout is a flat 44-byte-per-key array: 4 count + 44.
    QByteArray encoded;
    QVERIFY(NifBlockFile::encodeKeyframeData(QStringLiteral("NiKeyframeData"), keys, encoded));
    QCOMPARE(encoded.size(), 48);

    // An unknown block type is never encodable.
    QVERIFY(!NifBlockFile::encodeKeyframeData(QStringLiteral("NiSomethingElse"), keys, encoded));
}

namespace {

void put32(QByteArray& out, quint32 value)
{
    out.append(static_cast<char>(value & 0xFF));
    out.append(static_cast<char>((value >> 8) & 0xFF));
    out.append(static_cast<char>((value >> 16) & 0xFF));
    out.append(static_cast<char>((value >> 24) & 0xFF));
}

void put16(QByteArray& out, quint16 value)
{
    out.append(static_cast<char>(value & 0xFF));
    out.append(static_cast<char>((value >> 8) & 0xFF));
}

void putSizedString(QByteArray& out, const QByteArray& text)
{
    put32(out, static_cast<quint32>(text.size()));
    out.append(text);
}

// ExportString: a u8 length that includes the NUL. The header tables use
// SizedString instead, and mixing the two up is what made an earlier reader
// reject every 10.0.1.2 file, so both forms are built here on purpose.
void putExportString(QByteArray& out, const QByteArray& text)
{
    const int len = text.size() + 1;
    out.append(static_cast<char>(len));
    out.append(text);
    out.append('\0');
}

// Builds a minimal NetImmerse container. Two shapes exist and both ship:
// `withExportHeader` covers 10.0.1.2, which carries one, and 10.0.1.0, which
// does not. Below 5.0.0.1 there is no type table at all and each block carries
// its own sized type name inline, which `inlineTypes` selects.
QByteArray buildNetImmerse(const QByteArray& versionLine, quint32 version,
                           bool withExportHeader, bool inlineTypes)
{
    QByteArray out;
    out.append(versionLine);
    out.append('\n');
    put32(out, version);
    put32(out, 1); // num_blocks

    if (withExportHeader) {
        put32(out, 3); // bs_version
        putExportString(out, "someone");
        putExportString(out, "Default Process Script");
        putExportString(out, "Default Export Script");
    }

    if (!inlineTypes) {
        put16(out, 1); // num_block_types
        putSizedString(out, "NiNode");
        put16(out, 0); // block_type_index[0]
        put32(out, 0); // num_groups
    }

    // One block payload. The inline-type shape names the block first; the other
    // takes the type from the header table and opens with the payload, behind
    // the zero word every pre-10.1.0.107 block carries.
    if (inlineTypes)
        putSizedString(out, "NiNode");
    else
        put32(out, 0);
    putSizedString(out, "SomeNodeName");
    put32(out, 0xFFFFFFFFu);
    return out;
}

} // namespace

void TestNifBlockFile::netImmerseContainersRoundTrip()
{
    struct Case {
        const char* versionLine;
        quint32 version;
        bool withExportHeader;
        bool inlineTypes;
    };
    // One case per shape that ships. The header field set is gated differently at
    // every step, so a reader that handles one and not the other rejects a valid
    // file rather than misreading it — which is what this pins.
    const QVector<Case> cases = {
        {"NetImmerse File Format, Version 10.0.1.0", 0x0A000100u, false, false},
        {"NetImmerse File Format, Version 10.0.1.2", 0x0A000102u, true, false},
        {"NetImmerse File Format, Version 4.2.1.0", 0x04020100u, false, true},
        {"NetImmerse File Format, Version 3.3.0.13", 0x0303000Du, false, true},
    };

    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    for (const Case& c : cases) {
        const QByteArray bytes = buildNetImmerse(c.versionLine, c.version,
                                                 c.withExportHeader, c.inlineTypes);
        const QString version = QString::fromLatin1(c.versionLine + 32);
        const QString path = temp.filePath(version + QStringLiteral(".nif"));
        QFile out(path);
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.write(bytes);
        out.close();

        QVERIFY2(NifBlockFile::isBethesdaNif(path), c.versionLine);

        NifBlockFile file;
        QVERIFY2(file.load(path), qPrintable(QStringLiteral("%1: %2")
                                                 .arg(version, file.lastError())));
        QVERIFY(file.isNetImmerse());
        QCOMPARE(file.headerVersion(), version);
        QCOMPARE(file.version(), c.version);
        // The block payloads are not walked: the container records no block
        // lengths, so there is nothing to address a block by. A codec must not be
        // inferred from the file loading.
        QVERIFY(!file.hasIndividualBlocks());
        QCOMPARE(file.count(), 0);
        QVERIFY(file.findBlocks(QStringLiteral("NiNode")).isEmpty());
        // And the read-and-save claim has to actually hold byte for byte.
        QCOMPARE(file.serialize(), bytes);
    }

    // A header line that is not a NetImmerse version line is rejected rather than
    // guessed at, and so is a version field that disagrees with the line: both are
    // how a wrong-offset read announces itself, and continuing past one would
    // misparse every field after it.
    QByteArray badLine = "NetImmerse File Format, Version ten";
    badLine.append('\n');
    put32(badLine, 0x0A000100u);
    put32(badLine, 1);
    put16(badLine, 0);
    QString error;
    NifBlockFile rejected;
    QVERIFY(!rejected.parseNetImmerse(badLine, error));

    QByteArray mismatch = "NetImmerse File Format, Version 10.0.1.0";
    mismatch.append('\n');
    put32(mismatch, 0x0A000102u); // the field disagrees with the line
    put32(mismatch, 1);
    QVERIFY(!rejected.parseNetImmerse(mismatch, error));
    QVERIFY2(error.contains(QStringLiteral("version field")),
             qPrintable(QStringLiteral("unhelpful error: %1").arg(error)));
}

QTEST_MAIN(TestNifBlockFile)
#include "test_nifblockfile.moc"

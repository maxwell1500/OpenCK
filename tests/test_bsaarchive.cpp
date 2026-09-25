#include <QTest>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QFile>

#include "bsaarchive.hpp"
#include "fuzparser.hpp"
#include "logger.hpp"

// Validates the BsaArchive reader against the user's Skyrim SE install
// (requires the game; not registered with CTest by default).
class TestBsaArchive : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testOpenVoicesArchive();
    void testOpenMeshesArchive();
    void testOpenMorrowindArchive();
    void testOpenStarfieldBtdx();
    void testStarfieldBtdxExtractsNif();
    void testExtractFuzRoundTrip();
};

void TestBsaArchive::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_bsaarchive_log.txt"));
}

void TestBsaArchive::testOpenVoicesArchive()
{
    const QString path = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim - Voices_en0.bsa");
    if (!QFileInfo::exists(path)) QSKIP("Skyrim SE not found");

    BsaArchive archive;
    QVERIFY(archive.open(path));
    QVERIFY(archive.fileCount() > 1000);
    QCOMPARE(archive.version(), 0x69); // SSE

    // Find a .fuz entry to confirm the structure is parsed.
    int fuzCount = 0;
    int firstFuz = -1;
    for (int i = 0; i < archive.fileCount(); ++i) {
        if (archive.entries()[i].fullPath.endsWith(QStringLiteral(".fuz"), Qt::CaseInsensitive)) {
            ++fuzCount;
            if (firstFuz < 0) firstFuz = i;
        }
    }
    qDebug() << "fuz entries:" << fuzCount;
    QVERIFY(fuzCount > 0);

    // Verify the first entry's path looks like a voice path.
    const auto& e = archive.entries()[firstFuz];
    qDebug() << "first fuz:" << e.fullPath << "size=" << e.rawSize();
    QVERIFY(e.fullPath.contains(QStringLiteral("voice"), Qt::CaseInsensitive));
}

void TestBsaArchive::testOpenMeshesArchive()
{
    const QString path = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim - Meshes0.bsa");
    if (!QFileInfo::exists(path)) QSKIP("Skyrim Meshes0.bsa not found");

    BsaArchive archive;
    QVERIFY(archive.open(path));
    QVERIFY(archive.fileCount() > 1000);

    // The meshes archive uses compression; verify a .nif entry reads back.
    int nifIndex = -1;
    for (int i = 0; i < archive.fileCount(); ++i) {
        if (archive.entries()[i].fullPath.endsWith(QStringLiteral(".nif"), Qt::CaseInsensitive)) {
            nifIndex = i;
            break;
        }
    }
    QVERIFY(nifIndex >= 0);
    QByteArray data;
    QVERIFY(archive.readData(nifIndex, data));
    qDebug() << "extracted nif:" << archive.entries()[nifIndex].fullPath << data.size() << "bytes";
    QVERIFY(data.size() > 16);
}

void TestBsaArchive::testOpenMorrowindArchive()
{
    const QString path = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls III- Morrowind (PC)/Content/Morrowind GOTY English/Data Files/Morrowind.bsa");
    if (!QFileInfo::exists(path)) QSKIP("Morrowind not found");

    BsaArchive archive;
    QVERIFY(archive.open(path));
    qDebug() << "morrowind entries:" << archive.fileCount();
    QVERIFY(archive.fileCount() > 1000);

    // Extract the first entry and verify it reads back.
    QByteArray data;
    QVERIFY(archive.readData(0, data));
    QVERIFY(data.size() > 0);
    qDebug() << "first entry:" << archive.entries()[0].fullPath << data.size() << "bytes";
}

// Starfield's 'BTDX' container is a different layout from the classic BSA
// family: a 32-byte header, fixed 36-byte file declarations, the data, then a
// trailing u16-length name table. Version 2 is zlib, version 3 a raw LZ4 block.
static const char* kStarfieldData = "C:/XboxGames/Starfield/Content/Data/";
static const char* kStarfieldArchive = "Starfield - LODMeshes.ba2";

static bool starfieldInstalled()
{
    return QFile::exists(QString::fromLatin1(kStarfieldData) + QLatin1String(kStarfieldArchive));
}

void TestBsaArchive::testOpenStarfieldBtdx()
{
    if (!starfieldInstalled()) QSKIP("Starfield not found");

    BsaArchive archive;
    QVERIFY2(archive.open(QString::fromLatin1(kStarfieldData)
                          + QLatin1String(kStarfieldArchive)),
             "failed to open the Starfield BTDX archive");
    QVERIFY(archive.fileCount() > 1000);

    // Names come from the trailing name table and use forward slashes, so the
    // stored paths are archive-relative, not Windows paths.
    int withNifs = 0;
    for (int i = 0; i < archive.fileCount(); ++i)
        if (archive.entries()[i].fullPath.endsWith(".nif", Qt::CaseInsensitive))
            ++withNifs;
    QVERIFY2(withNifs > 0, "no NIF entries in the Starfield archive");

    // Every declaration carries the 0xBAADF00D marker and a name; a parse that
    // drifted would produce empty or absurd paths.
    for (int i = 0; i < qMin(archive.fileCount(), 500); ++i) {
        QVERIFY(!archive.entries()[i].fullPath.isEmpty());
        QVERIFY(archive.entries()[i].size > 0);
    }
}

void TestBsaArchive::testStarfieldBtdxExtractsNif()
{
    if (!starfieldInstalled()) QSKIP("Starfield not found");

    BsaArchive archive;
    QVERIFY(archive.open(QString::fromLatin1(kStarfieldData)
                         + QLatin1String(kStarfieldArchive)));

    int checked = 0;
    for (int i = 0; i < archive.fileCount() && checked < 5; ++i) {
        const BsaFileEntry& entry = archive.entries()[i];
        if (!entry.fullPath.endsWith(".nif", Qt::CaseInsensitive)) continue;
        QByteArray data;
        QVERIFY2(archive.readData(static_cast<quint32>(i), data),
                 qPrintable(entry.fullPath));
        // A NIF always starts with its version line; anything else means the
        // payload was mis-sliced or mis-decompressed.
        QVERIFY2(data.startsWith("Gamebryo"),
                 qPrintable(entry.fullPath + QStringLiteral(": starts with '")
                            + QString::fromLatin1(data.left(12)) + QStringLiteral("'")));
        QVERIFY(data.size() > 100);
        ++checked;
    }
    QVERIFY(checked > 0);
}

void TestBsaArchive::testExtractFuzRoundTrip()
{
    const QString path = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim - Voices_en0.bsa");
    if (!QFileInfo::exists(path)) QSKIP("Skyrim SE not found");

    BsaArchive archive;
    QVERIFY(archive.open(path));

    int index = -1;
    for (int i = 0; i < archive.fileCount(); ++i) {
        if (archive.entries()[i].fullPath.endsWith(QStringLiteral(".fuz"), Qt::CaseInsensitive)) {
            index = i;
            break;
        }
    }
    QVERIFY(index >= 0);

    // Read the raw bytes.
    QByteArray raw;
    QVERIFY(archive.readData(index, raw));
    qDebug() << "extracted" << raw.size() << "bytes";
    QVERIFY(raw.size() >= 4);
    qDebug() << "magic:" << QString::fromLatin1(raw.left(4));

    // It should be a FUZ container.
    FuzParser parser;
    if (FuzParser::parse(raw, parser)) {
        qDebug() << "audio fourcc:" << parser.audioFourCC
                 << "lip:" << parser.lipData.size()
                 << "audio:" << parser.audioData.size();
        QVERIFY(parser.hasLip() || parser.hasAudio());
    } else {
        // Some entries may not be FUZ (e.g. header-only); find one that is.
        bool foundGood = false;
        for (int i = 0; i < archive.fileCount(); ++i) {
            if (!archive.entries()[i].fullPath.endsWith(QStringLiteral(".fuz"), Qt::CaseInsensitive))
                continue;
            QByteArray probe;
            if (!archive.readData(i, probe)) continue;
            FuzParser p;
            if (FuzParser::parse(probe, p) && (p.hasLip() || p.hasAudio())) {
                foundGood = true;
                qDebug() << "found valid fuz at" << i << ":" << archive.entries()[i].fullPath
                         << "fourcc:" << p.audioFourCC;
                break;
            }
        }
        QVERIFY(foundGood);
    }

    // Extract to a temp dir and confirm byte-identical read-back.
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString outPath = tmpDir.filePath(QStringLiteral("extracted.fuz"));

    QVERIFY(archive.extract(index, outPath));
    QFile check(outPath);
    QVERIFY(check.open(QIODevice::ReadOnly));
    const QByteArray reread = check.readAll();
    check.close();
    QCOMPARE(reread, raw);
}

QTEST_MAIN(TestBsaArchive)
#include "test_bsaarchive.moc"

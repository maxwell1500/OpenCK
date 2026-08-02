#include <QTest>
#include <QTemporaryFile>
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
    QVERIFY2(QFileInfo::exists(path), "Skyrim SE not found");

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
    QVERIFY2(QFileInfo::exists(path), "Skyrim Meshes0.bsa not found");

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
    QVERIFY2(QFileInfo::exists(path), "Morrowind not found");

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

void TestBsaArchive::testExtractFuzRoundTrip()
{
    const QString path = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim - Voices_en0.bsa");
    QVERIFY2(QFileInfo::exists(path), "Skyrim SE not found");

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

    // Extract to a temp file and confirm byte-identical read-back.
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    const QString outPath = tmp.fileName();
    tmp.close();

    QVERIFY(archive.extract(index, outPath));
    QFile check(outPath);
    QVERIFY(check.open(QIODevice::ReadOnly));
    const QByteArray reread = check.readAll();
    check.close();
    QCOMPARE(reread, raw);
}

QTEST_MAIN(TestBsaArchive)
#include "test_bsaarchive.moc"

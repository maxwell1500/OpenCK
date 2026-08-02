#include <QTest>
#include <QFileInfo>

#include "model/tools/assetresolver.hpp"
#include "bsaarchive.hpp"
#include "logger.hpp"

// Validates the AssetResolver archive-aware lookup against the user's Skyrim
// SE + Morrowind installs: archived assets (inside .bsa) must resolve even
// though they are not loose files, and loose files still resolve. Not
// registered with CTest by default — requires a real game install.
class TestAssetResolver : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testScanSkyrim();
    void testArchivedPathsResolve();
    void testCaseAndSeparatorNormalization();
    void testMissingPath();
    void testMorrowind();
    void testEmptyDir();
};

void TestAssetResolver::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_assetresolver_log.txt"));
}

namespace {
const QString sDataDir = QStringLiteral(
    "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data");
const QString sMeshesBsa = QStringLiteral(
    "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim - Meshes0.bsa");
const QString sVoicesBsa = QStringLiteral(
    "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim - Voices_en0.bsa");
const QString sMorrowindDir = QStringLiteral(
    "C:/XboxGames/The Elder Scrolls III- Morrowind (PC)/Content/Morrowind GOTY English/Data Files");
const QString sMorrowindBsa = QStringLiteral(
    "C:/XboxGames/The Elder Scrolls III- Morrowind (PC)/Content/Morrowind GOTY English/Data Files/Morrowind.bsa");
} // namespace

void TestAssetResolver::testScanSkyrim()
{
    if (!QFileInfo::exists(sDataDir)) QSKIP("Skyrim SE data dir not found");
    AssetResolver res(sDataDir);
    qDebug() << "archives:" << res.archiveCount()
             << "paths:" << res.allPaths().size();
    QVERIFY(res.archiveCount() > 0);
    QVERIFY(!res.isEmpty());
}

void TestAssetResolver::testArchivedPathsResolve()
{
    if (!QFileInfo::exists(sMeshesBsa)) QSKIP("Skyrim SE Meshes BSA not found");
    AssetResolver res(sDataDir);

    // A mesh that exists only inside Skyrim - Meshes0.bsa.
    BsaArchive meshes;
    QVERIFY(meshes.open(sMeshesBsa));
    QString nifPath;
    for (int i = 0; i < meshes.fileCount(); ++i)
    {
        if (meshes.entries().at(i).fullPath.endsWith(QStringLiteral(".nif"), Qt::CaseInsensitive))
        {
            nifPath = meshes.entries().at(i).fullPath;
            break;
        }
    }
    QVERIFY(!nifPath.isEmpty());
    qDebug() << "archived nif:" << nifPath;
    QVERIFY(res.contains(nifPath));
    QVERIFY(!res.containsLoose(nifPath));
    QVERIFY(res.absoluteLoosePath(nifPath, sDataDir).isEmpty());

    // A voice .fuz that exists only inside Skyrim - Voices_en0.bsa.
    if (QFileInfo::exists(sVoicesBsa))
    {
        BsaArchive voices;
        QVERIFY(voices.open(sVoicesBsa));
        QString fuzPath;
        for (int i = 0; i < voices.fileCount(); ++i)
        {
            if (voices.entries().at(i).fullPath.endsWith(QStringLiteral(".fuz"), Qt::CaseInsensitive))
            {
                fuzPath = voices.entries().at(i).fullPath;
                break;
            }
        }
        QVERIFY(!fuzPath.isEmpty());
        QVERIFY(res.contains(fuzPath));
        QVERIFY(!res.containsLoose(fuzPath));
    }

    // A loose file (the master plugin) resolves and has an absolute path.
    QVERIFY(res.containsLoose("Skyrim.esm"));
    QVERIFY(!res.absoluteLoosePath("Skyrim.esm", sDataDir).isEmpty());
}

void TestAssetResolver::testCaseAndSeparatorNormalization()
{
    if (!QFileInfo::exists(sMeshesBsa)) QSKIP("Skyrim SE Meshes BSA not found");
    AssetResolver res(sDataDir);

    BsaArchive meshes;
    QVERIFY(meshes.open(sMeshesBsa));
    QString nifPath;
    for (int i = 0; i < meshes.fileCount(); ++i)
    {
        if (meshes.entries().at(i).fullPath.endsWith(QStringLiteral(".nif"), Qt::CaseInsensitive))
        {
            nifPath = meshes.entries().at(i).fullPath;
            break;
        }
    }
    QVERIFY(!nifPath.isEmpty());

    // Case and forward/backslash differences must not affect lookup.
    QVERIFY(res.contains(nifPath.toUpper()));
    QVERIFY(res.contains(nifPath.replace(QLatin1Char('\\'), QLatin1Char('/'))));
    QVERIFY(res.contains(nifPath.toUpper().replace(QLatin1Char('\\'), QLatin1Char('/'))));
}

void TestAssetResolver::testMissingPath()
{
    if (!QFileInfo::exists(sDataDir)) QSKIP("Skyrim SE data dir not found");
    AssetResolver res(sDataDir);
    QVERIFY(!res.contains("meshes\\nonexistent\\definitely_missing.nif"));
    QVERIFY(!res.contains(""));
    QVERIFY(!res.containsLoose("textures\\missing\\asset.dds"));
}

void TestAssetResolver::testMorrowind()
{
    if (!QFileInfo::exists(sMorrowindDir)) QSKIP("Morrowind not found");
    AssetResolver res(sMorrowindDir);

    BsaArchive morrowind;
    QVERIFY(morrowind.open(sMorrowindBsa));
    QVERIFY(morrowind.fileCount() > 0);
    const QString first = morrowind.entries().at(0).fullPath;
    qDebug() << "morrowind first entry:" << first;
    QVERIFY(res.contains(first));
    QVERIFY(!res.containsLoose(first));
}

void TestAssetResolver::testEmptyDir()
{
    AssetResolver res(QStringLiteral("C:/this/directory/does/not/exist/for/sure"));
    QVERIFY(res.isEmpty());
    QCOMPARE(res.archiveCount(), 0);
    QVERIFY(!res.contains("anything.nif"));
}

QTEST_MAIN(TestAssetResolver)
#include "test_assetresolver.moc"

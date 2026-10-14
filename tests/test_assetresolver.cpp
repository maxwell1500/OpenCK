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
    void testLazyArchiveResolveAcrossArchives();
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

void TestAssetResolver::testLazyArchiveResolveAcrossArchives()
{
    const QString sfDataDir = QStringLiteral("C:/XboxGames/Starfield/Content/Data");
    if (QFileInfo::exists(sfDataDir))
    {
        const QString m = QStringLiteral("meshes/setdressing/digitaltables/digitaltable01a.nif");
        const QString t1 = QStringLiteral("textures/common/metal/metalAlu_color.DDS");
        const QString t2 = QStringLiteral("textures/setdressing/manufactured_goods/vac_grade_explosive/vac_grade_explosive_base_color.dds");

        // Clear cached extractions so resolution has to hit the archives
        QFile::remove(sfDataDir + QStringLiteral("/.openck_cache/") + m);
        QFile::remove(sfDataDir + QStringLiteral("/.openck_cache/") + t1);
        QFile::remove(sfDataDir + QStringLiteral("/.openck_cache/") + t2);

        // Resolving an asset in Meshes01.ba2 must not prevent later lookups in Textures05.ba2 or Textures10.ba2
        const QString resM = AssetResolver::resolveOrExtract(m, sfDataDir);
        QVERIFY2(!resM.isEmpty() && QFileInfo::exists(resM), "mesh in Meshes01.ba2 must resolve");

        const QString resT1 = AssetResolver::resolveOrExtract(t1, sfDataDir);
        QVERIFY2(!resT1.isEmpty() && QFileInfo::exists(resT1), "texture in Textures05.ba2 must resolve after mesh");

        const QString resT2 = AssetResolver::resolveOrExtract(t2, sfDataDir);
        QVERIFY2(!resT2.isEmpty() && QFileInfo::exists(resT2), "texture in Textures10.ba2 must resolve after Textures05");
    }
    else if (QFileInfo::exists(sDataDir))
    {
        const QString mesh = AssetResolver::resolveOrExtract(QStringLiteral("meshes/actors/character/character assets/skeleton.nif"), sDataDir);
        QVERIFY(!mesh.isEmpty() && QFileInfo::exists(mesh));
    }
    else
    {
        QSKIP("No Starfield or Skyrim SE install available");
    }
}

QTEST_MAIN(TestAssetResolver)
#include "test_assetresolver.moc"

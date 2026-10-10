#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "src/model/tools/modmanagerdetection.hpp"
#include "src/model/tools/moddeploymentresolver.hpp"

class TestModManager : public QObject
{
    Q_OBJECT

private slots:
    void testParseMo2Ini();
    void testParseMo2IniMissing();
    void testParseVortexField();
    void testParseVortexFieldMissing();
    void testParseVortexDeployment();
    void testParseVortexDeploymentRejectsBadShape();
    void testDeployedAndSourcePaths();
    void testLinkStatusHardlink();
    void testLinkStatusDistinct();
    void testParseMo2ModList();
    void testWinningModAndOverrideChain();
    void testRealStarfieldDeployment();
};

void TestModManager::testParseMo2Ini()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString ini = dir.filePath("ModOrganizer.ini");
    QFile f(ini);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream ts(&f);
    ts << "[general]\n"
       << "gamePath=C:/Games/Starfield\n"
       << "modsDirectory=C:/Games/Starfield/mods\n"
       << "ignored=1\n"
       << "\n"
       << "[Profiles]\n"
       << "profileName=Default\n"
       << "profileName=MyMods\n"
       << "selectedProfile=MyMods\n";
    f.close();

    ModManagerDetection::ModManagerInfo info;
    QVERIFY(ModManagerDetection::parseMo2Ini(ini, info));
    QCOMPARE(info.profiles.size(), 2);
    QCOMPARE(info.profiles.at(0), QString("Default"));
    QCOMPARE(info.profiles.at(1), QString("MyMods"));
    QCOMPARE(info.selectedProfile, QString("MyMods"));
    QVERIFY(info.gamePath.contains("Starfield"));
    QVERIFY(info.modsDirectory.contains("Starfield"));
}

void TestModManager::testParseMo2IniMissing()
{
    ModManagerDetection::ModManagerInfo info;
    QVERIFY(!ModManagerDetection::parseMo2Ini("C:/definitely/not/here.ini", info));
}

void TestModManager::testParseVortexField()
{
    const QString json =
        "{\n"
        "  \"gamePath\": \"C:/Games/Starfield\",\n"
        "  \"activeProfile\": \"MyProfile\"\n"
        "}\n";
    QCOMPARE(ModManagerDetection::parseVortexField(json, "gamePath"),
             QString("C:/Games/Starfield"));
    QCOMPARE(ModManagerDetection::parseVortexField(json, "activeProfile"),
             QString("MyProfile"));
}

void TestModManager::testParseVortexFieldMissing()
{
    const QString json = "{ \"gamePath\": \"x\" }";
    QVERIFY(ModManagerDetection::parseVortexField(json, "nope").isEmpty());
}

// The manifest shape a real managed Starfield data directory writes.
void TestModManager::testParseVortexDeployment()
{
    const QString json =
        "{\n"
        "  \"instance\": \"578eb4ae-838d-4ced-afc6-1b928062e3ad\",\n"
        "  \"version\": 1,\n"
        "  \"deploymentMethod\": \"hardlink_activator\",\n"
        "  \"gameId\": \"starfield\",\n"
        "  \"deploymentTime\": 1782351802311,\n"
        "  \"stagingPath\": \"C:\\\\Users\\\\max\\\\AppData\\\\Roaming\\\\Vortex\\\\starfield\\\\mods\",\n"
        "  \"targetPath\": \"C:\\\\XboxGames\\\\Starfield\\\\Content\",\n"
        "  \"files\": [\n"
        "    { \"relPath\": \"JetpackVanilla.txt\",\n"
        "      \"source\": \"Jetpack Overhaul-569-1-05-1694446917\",\n"
        "      \"target\": \"\", \"time\": 1765686656000 },\n"
        "    { \"relPath\": \"Interface\\\\MyWidget.swf\",\n"
        "      \"source\": \"My Mod-1-0\",\n"
        "      \"target\": \"\", \"time\": 1765686656001 }\n"
        "  ]\n"
        "}\n";

    ModDeploymentResolver::VortexDeployment d;
    QVERIFY(ModDeploymentResolver::parseVortexDeployment(json.toUtf8(), d));
    QCOMPARE(d.version, 1);
    QCOMPARE(d.deploymentMethod, QString("hardlink_activator"));
    QCOMPARE(d.gameId, QString("starfield"));
    QCOMPARE(d.deploymentTime, static_cast<qint64>(1782351802311));
    QCOMPARE(d.files.size(), 2);
    QCOMPARE(d.files.at(0).relPath, QString("JetpackVanilla.txt"));
    QCOMPARE(d.files.at(0).source,
             QString("Jetpack Overhaul-569-1-05-1694446917"));
    // Staging/target keep their separators; relPath normalizes to '/'.
    QVERIFY(d.stagingPath.contains("Vortex/starfield/mods"));
    QVERIFY(d.targetPath.contains("Starfield/Content"));
    QCOMPARE(d.files.at(1).relPath, QString("Interface/MyWidget.swf"));
    QVERIFY(d.isValid());
}

void TestModManager::testParseVortexDeploymentRejectsBadShape()
{
    ModDeploymentResolver::VortexDeployment out;
    // Not JSON.
    QVERIFY(!ModDeploymentResolver::parseVortexDeployment("not json", out));
    // JSON but no files array.
    QVERIFY(!ModDeploymentResolver::parseVortexDeployment("{}", out));
    // Empty files array is as good as absent.
    QVERIFY(!ModDeploymentResolver::parseVortexDeployment(
                "{\"files\": []}", out));
    // Null targetPath is not a deployment.
    QVERIFY(!ModDeploymentResolver::parseVortexDeployment(
                "{\"targetPath\": \"\", \"files\": [{\"relPath\": \"a\"}]}", out));
}

void TestModManager::testDeployedAndSourcePaths()
{
    const QString json =
        "{\"version\":1,\"deploymentMethod\":\"hardlink_activator\","
        "\"gameId\":\"starfield\",\"stagingPath\":\"D:/vortex/mods\","
        "\"targetPath\":\"D:/Games/Starfield/Data\","
        "\"files\":[{\"relPath\":\"meshes/chair.nif\","
        "\"source\":\"Chair Mod-2-1\",\"target\":\"\",\"time\":1}],"
        "\"instance\":\"x\"}";
    ModDeploymentResolver::VortexDeployment d;
    QVERIFY(ModDeploymentResolver::parseVortexDeployment(json.toUtf8(), d));

    // Native-separator lookups hit the same entry.
    const QString deployed =
        ModDeploymentResolver::deployedFilePath(d, "meshes\\chair.nif");
    QVERIFY(deployed.contains("Starfield"));
    QVERIFY(deployed.endsWith(QDir::toNativeSeparators("meshes/chair.nif")));
    const QString source =
        ModDeploymentResolver::sourceFilePath(d, "meshes/chair.nif");
    QVERIFY(source.contains("Chair Mod-2-1"));
    QVERIFY(source.contains(QDir::toNativeSeparators("meshes/chair.nif")));

    // Absent entries return empty rather than a guess.
    QVERIFY(ModDeploymentResolver::deployedFilePath(d, "meshes/table.nif")
                .isEmpty());
#ifdef _WIN32
    const QString manifest = QDir::tempPath() + "/openck_test_vortex.json";
    QFile f(manifest);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(json.toUtf8());
    f.close();
    ModDeploymentResolver::VortexDeployment fromDisk;
    QVERIFY(ModDeploymentResolver::readVortexDeployment(manifest, fromDisk));
    QCOMPARE(fromDisk.files.size(), 1);
    // findVortexDeployment returns empty for a dir without the manifest.
    QVERIFY(ModDeploymentResolver::findVortexDeployment(QDir::tempPath())
                .isEmpty());
    f.remove();
#endif
}

void TestModManager::testLinkStatusHardlink()
{
#ifdef _WIN32
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString staged = dir.filePath("chair.nif");
    QFile f(staged);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("nif");
    f.close();

    const QString deployed = dir.filePath("deployed.nif");
    // CreateHardLink is the documented call for Vortex's hardlink_activator.
    const bool linked = CreateHardLinkW(
        reinterpret_cast<LPCWSTR>(deployed.utf16()),
        reinterpret_cast<LPCWSTR>(staged.utf16()), nullptr) != FALSE;
    if (!linked)
    {
        QSKIP("Hardlink creation not permitted on this filesystem");
    }
    QCOMPARE(ModDeploymentResolver::linkStatus(staged, deployed),
             ModDeploymentResolver::LinkKind::Hardlink);
    QCOMPARE(ModDeploymentResolver::linkStatus(staged, staged),
             ModDeploymentResolver::LinkKind::Hardlink);
    QCOMPARE(ModDeploymentResolver::linkKindName(
                 ModDeploymentResolver::LinkKind::Hardlink),
             QStringLiteral("Hardlink"));
#else
    QSKIP("Hardlink status is Windows-only");
#endif
}

void TestModManager::testLinkStatusDistinct()
{
#ifdef _WIN32
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString a = dir.filePath("a.bin");
    const QString b = dir.filePath("b.bin");
    QFile fa(a);
    QVERIFY(fa.open(QIODevice::WriteOnly));
    fa.write("x");
    fa.close();
    QFile fb(b);
    QVERIFY(fb.open(QIODevice::WriteOnly));
    fb.write("x");
    fb.close();
    QCOMPARE(ModDeploymentResolver::linkStatus(a, b),
             ModDeploymentResolver::LinkKind::Distinct);
    QCOMPARE(ModDeploymentResolver::linkStatus(a, "C:/no/such/file.bin"),
             ModDeploymentResolver::LinkKind::Unknown);
#else
    QSKIP("Link status is Windows-only");
#endif
}

void TestModManager::testParseMo2ModList()
{
    const QString content =
        "#Created by Mod Organizer 2\n"
        "+DLC: Dawnguard\n"
        "-Unofficial Skyrim Patch\n"
        "\n"
        "*\n"
        "+Immersive Armors\n";
    QVector<ModDeploymentResolver::ModEntry> entries;
    QVERIFY(ModDeploymentResolver::parseMo2ModList(content, entries));
    QCOMPARE(entries.size(), 3);
    QCOMPARE(entries.at(0).name, QString("DLC: Dawnguard"));
    QVERIFY(entries.at(0).enabled);
    QCOMPARE(entries.at(1).name, QString("Unofficial Skyrim Patch"));
    QVERIFY(!entries.at(1).enabled);
    QCOMPARE(entries.at(2).name, QString("Immersive Armors"));

    const QStringList enabled =
        ModDeploymentResolver::enabledMods(entries);
    QCOMPARE(enabled.size(), 2);
    QCOMPARE(enabled.at(0), QString("DLC: Dawnguard"));
    QCOMPARE(enabled.at(1), QString("Immersive Armors"));

    QVERIFY(!ModDeploymentResolver::parseMo2ModList("", entries));
}

void TestModManager::testWinningModAndOverrideChain()
{
    // MO2 applies modlist order so the LAST enabled line wins.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString modsDir = dir.filePath("mods");
    for (const QString& mod : {"Base", "Texture Replacer", "Extra"})
    {
        QVERIFY(QDir().mkpath(QDir(modsDir).filePath(mod)));
    }
    const QString rel = "meshes/chair.nif";
    auto put = [&](const QString& mod, const QByteArray& body)
    {
        QFile f(QDir(QDir(modsDir).filePath(mod)).filePath(rel));
        QVERIFY(QDir().mkpath(QFileInfo(f).absolutePath()));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(body);
        f.close();
    };
    put("Base", "base");
    put("Texture Replacer", "replacer");
    put("Extra", "extra");

    QVector<ModDeploymentResolver::ModEntry> entries;
    QVERIFY(ModDeploymentResolver::parseMo2ModList(
        "+Base\n-Texture Replacer\n+Extra\n", entries));

    QString winnerPath;
    const QString winner = ModDeploymentResolver::winningMod(
        entries, modsDir, rel, &winnerPath);
    // "Extra" is the last enabled line; the disabled replacer is skipped.
    QCOMPARE(winner, QString("Extra"));
    QVERIFY(winnerPath.endsWith(
        QDir::toNativeSeparators("Extra/meshes/chair.nif")));

    const auto chain = ModDeploymentResolver::overrideChain(
        entries, modsDir, rel);
    QCOMPARE(chain.size(), 2);
    QCOMPARE(chain.at(0).first, QString("Extra"));
    QCOMPARE(chain.at(1).first, QString("Base"));

    // A file only the disabled mod provides is not deployed at all.
    QVERIFY(ModDeploymentResolver::winningMod(entries, modsDir,
                                              "meshes/absent.nif")
                .isEmpty());
}

// Real-data gate: the managed Starfield installation on this machine writes
// its deployment manifests into the game root and the Data folder.
void TestModManager::testRealStarfieldDeployment()
{
    const QStringList candidates = {
        QStringLiteral("C:/XboxGames/Starfield/Content/Data"),
        QStringLiteral("C:/XboxGames/Starfield/Content"),
        QStringLiteral("C:/Program Files (x86)/Steam/steamapps/common/Starfield"),
    };
    QString found;
    for (const QString& candidate : candidates)
    {
        const QString manifest =
            ModDeploymentResolver::findVortexDeployment(candidate);
        if (!manifest.isEmpty())
        {
            found = manifest;
            break;
        }
    }
    if (found.isEmpty())
    {
        QSKIP("No managed Starfield installation with a Vortex deployment "
              "manifest");
    }

    ModDeploymentResolver::VortexDeployment d;
    QVERIFY(ModDeploymentResolver::readVortexDeployment(found, d));
    QVERIFY(d.isValid());
    QCOMPARE(d.deploymentMethod, QStringLiteral("hardlink_activator"));

    // Real manifests key files with native separators; the parser must
    // normalize them so callers can resolve either spelling.
    {
        QFile raw(found);
        QVERIFY(raw.open(QIODevice::ReadOnly));
        const QString text = QString::fromUtf8(raw.readAll());
        raw.close();
        QVERIFY2(text.contains(QLatin1String("relPath\": \"")),
                 "manifest has no relPath entries");
        bool sawBackslash = false;
        // Find an entry whose raw spelling used backslashes.
        const QStringList rawKeys = text.split(QLatin1String("relPath\": \""));
        for (int i = 1; i < rawKeys.size(); ++i)
        {
            // The file text is JSON-escaped, so undo \\. before the
            // backslash-to-slash normalization.
            QString rawKey = rawKeys.at(i).section(QLatin1Char('"'), 0, 0);
            rawKey.replace(QLatin1String("\\\\"), QLatin1String("\\"));
            if (!rawKey.contains(QLatin1Char('\\')))
            {
                continue;
            }
            const QString normalized = QDir::fromNativeSeparators(rawKey);
            QVERIFY2(!ModDeploymentResolver::deployedFilePath(d, normalized)
                         .isEmpty(),
                     qPrintable(QString("backslash key unresolved: %1")
                                .arg(normalized)));
            QVERIFY2(!ModDeploymentResolver::deployedFilePath(d, rawKey)
                         .isEmpty(),
                     qPrintable(QString("raw key unresolved: %1").arg(rawKey)));
            sawBackslash = true;
            break;
        }
        QVERIFY2(sawBackslash,
                 "expected native-separator relPaths in the manifest");
    }

    // Every entry whose staging root still exists must resolve on both sides
    // and stay linked the way the manifest's method claims.
    const QString stagingRoot = d.stagingPath;
    int checked = 0;
    for (const ModDeploymentResolver::DeployedFile& file : d.files)
    {
        const QString deployed =
            ModDeploymentResolver::deployedFilePath(d, file.relPath);
        if (!QFile::exists(deployed))
        {
            continue;
        }
        const QString staged =
            ModDeploymentResolver::sourceFilePath(d, file.relPath);
        if (!QFile::exists(staged))
        {
            continue;
        }
        QCOMPARE(ModDeploymentResolver::linkStatus(staged, deployed),
                 ModDeploymentResolver::LinkKind::Hardlink);
        ++checked;
    }
    QVERIFY2(checked > 0,
             qPrintable(QString("no staged files under %1").arg(stagingRoot)));
}

QTEST_MAIN(TestModManager)
#include "test_modmanager.moc"
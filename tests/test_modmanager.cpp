#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

#include "src/model/tools/modmanagerdetection.hpp"

class TestModManager : public QObject
{
    Q_OBJECT

private slots:
    void testParseMo2Ini();
    void testParseMo2IniMissing();
    void testParseVortexField();
    void testParseVortexFieldMissing();
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

QTEST_MAIN(TestModManager)
#include "test_modmanager.moc"

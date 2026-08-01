#include <QTest>
#include <QTemporaryFile>

#include "../../src/model/tools/papyrusproject.hpp"
#include "../../libs/files/log/logger.hpp"

class TestPapyrusProject : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testParseFull();
    void testParseEmpty();
    void testMalformedXml();
    void testLoadFile();
};

void TestPapyrusProject::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_papyrusproject_log.txt"));
}

void TestPapyrusProject::testParseFull()
{
    const QString xml = R"(<?xml version="1.0" encoding="utf-8"?>
<PapyrusProject>
    <Imports>
        <Import>Data/Scripts/Source</Import>
        <Import>Data/Scripts/Source/Base</Import>
    </Imports>
    <Folders>
        <Folder recurse="0">Data/Scripts/Source/MyMod</Folder>
        <Folder recurse="1">Data/Scripts/Source/Shared</Folder>
    </Folders>
    <Scripts>
        <Script>MyScript</Script>
    </Scripts>
    <Output>Data/Scripts</Output>
    <Flags>
        <Flag>Papyrus Flags.flg</Flag>
    </Flags>
    <Asm>None</Asm>
    <Optimize>true</Optimize>
    <Release>true</Release>
    <Final>false</Final>
</PapyrusProject>)";

    PapyrusProject project;
    QVERIFY(PapyrusProject::parse(xml, project));
    QCOMPARE(project.imports, QStringList({ QStringLiteral("Data/Scripts/Source"), QStringLiteral("Data/Scripts/Source/Base") }));
    QCOMPARE(project.folders.size(), 2);
    QCOMPARE(project.folders[0], QStringLiteral("Data/Scripts/Source/MyMod"));
    QVERIFY(!project.folderRecurse[0]);
    QVERIFY(project.folderRecurse[1]);
    QCOMPARE(project.scripts, QStringList({ QStringLiteral("MyScript") }));
    QCOMPARE(project.output, QStringLiteral("Data/Scripts"));
    QCOMPARE(project.flags, QStringList({ QStringLiteral("Papyrus Flags.flg") }));
    QCOMPARE(project.asmMode, QStringLiteral("None"));
    QVERIFY(project.optimize);
    QVERIFY(project.release);
    QVERIFY(!project.final_);
}

void TestPapyrusProject::testParseEmpty()
{
    PapyrusProject project;
    QVERIFY(PapyrusProject::parse(QStringLiteral("<PapyrusProject></PapyrusProject>"), project));
    QVERIFY(project.imports.isEmpty());
    QVERIFY(project.folders.isEmpty());
    QVERIFY(project.scripts.isEmpty());
    QVERIFY(project.output.isEmpty());
    QCOMPARE(project.asmMode, QString());
}

void TestPapyrusProject::testMalformedXml()
{
    PapyrusProject project;
    QVERIFY(!PapyrusProject::parse(QStringLiteral("<PapyrusProject><Imports>"), project));
}

void TestPapyrusProject::testLoadFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("<PapyrusProject><Output>out.pex</Output></PapyrusProject>");
    file.close();

    PapyrusProject project;
    QVERIFY(PapyrusProject::loadFile(file.fileName(), project));
    QCOMPARE(project.output, QStringLiteral("out.pex"));

    PapyrusProject missing;
    QVERIFY(!PapyrusProject::loadFile(QStringLiteral("Z:/missing.ppj"), missing));
}

QTEST_MAIN(TestPapyrusProject)
#include "test_papyrusproject.moc"

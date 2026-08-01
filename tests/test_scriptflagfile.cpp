#include <QTest>
#include <QTemporaryFile>

#include "../../src/model/tools/scriptflagfile.hpp"
#include "../../libs/files/log/logger.hpp"

class TestScriptFlagFile : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testParseBasic();
    void testParseCommentsAndBlanks();
    void testUnknownFlags();
    void testLoadFile();
    void testKnownFlags();
};

void TestScriptFlagFile::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_scriptflagfile_log.txt"));
}

void TestScriptFlagFile::testParseBasic()
{
    const QVector<ScriptFlagFile::Entry> entries = ScriptFlagFile::parse(
        "MyScript = Hidden|Conditional\nOtherScript = Default\n");
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries[0].scriptName, QStringLiteral("MyScript"));
    QCOMPARE(entries[0].flags, QStringList({ QStringLiteral("Hidden"), QStringLiteral("Conditional") }));
    QCOMPARE(entries[1].scriptName, QStringLiteral("OtherScript"));
    QCOMPARE(entries[1].flags, QStringList({ QStringLiteral("Default") }));
}

void TestScriptFlagFile::testParseCommentsAndBlanks()
{
    const QVector<ScriptFlagFile::Entry> entries = ScriptFlagFile::parse(
        "; header comment\n\n# another comment\n  \nScript = Hidden\n");
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries[0].scriptName, QStringLiteral("Script"));
}

void TestScriptFlagFile::testUnknownFlags()
{
    const QVector<ScriptFlagFile::Entry> entries = ScriptFlagFile::parse(
        "Good = Hidden\nBad = NotAFlag|Conditional\n");
    const QStringList unknown = ScriptFlagFile::unknownFlags(entries);
    QCOMPARE(unknown.size(), 1);
    QCOMPARE(unknown[0], QStringLiteral("Bad: NotAFlag"));
}

void TestScriptFlagFile::testLoadFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("Alpha = Hidden|Mandatory\n");
    file.close();

    QVector<ScriptFlagFile::Entry> out;
    QVERIFY(ScriptFlagFile::loadFile(file.fileName(), out));
    QCOMPARE(out.size(), 1);
    QCOMPARE(out[0].scriptName, QStringLiteral("Alpha"));

    QVector<ScriptFlagFile::Entry> none;
    QVERIFY(!ScriptFlagFile::loadFile(QStringLiteral("Z:/missing.flg"), none));
}

void TestScriptFlagFile::testKnownFlags()
{
    const QStringList flags = ScriptFlagFile::knownFlags();
    QCOMPARE(flags.size(), 6);
    QVERIFY(flags.contains(QStringLiteral("Hidden")));
    QVERIFY(flags.contains(QStringLiteral("Mandatory")));
    QVERIFY(flags.contains(QStringLiteral("CollapsedOnBase")));
}

QTEST_MAIN(TestScriptFlagFile)
#include "test_scriptflagfile.moc"

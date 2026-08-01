#include <QTest>
#include <QTemporaryFile>

#include "../../src/model/tools/opallist.hpp"
#include "../../libs/files/log/logger.hpp"

class TestOpalList : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testParseBasic();
    void testParseQuoted();
    void testCommentsAndPadding();
    void testLoadFile();
    void testValueLookup();
};

void TestOpalList::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_opallist_log.txt"));
}

void TestOpalList::testParseBasic()
{
    const QString content = QStringLiteral(
        "FormID,Count,Chance\n"
        "0001,3,50\n"
        "0002,1,100\n");
    const OpalList list = OpalList::parse(content);

    QCOMPARE(list.headers, QStringList({ QStringLiteral("FormID"), QStringLiteral("Count"), QStringLiteral("Chance") }));
    QCOMPARE(list.rowCount(), 2);
    QCOMPARE(list.rows[0], QVector<QString>({ QStringLiteral("0001"), QStringLiteral("3"), QStringLiteral("50") }));
    QCOMPARE(list.rows[1], QVector<QString>({ QStringLiteral("0002"), QStringLiteral("1"), QStringLiteral("100") }));
}

void TestOpalList::testParseQuoted()
{
    const QString content = QStringLiteral(
        "Name,Count\n"
        "\"Chair, Wooden\",5\n");
    const OpalList list = OpalList::parse(content);
    QCOMPARE(list.rowCount(), 1);
    QCOMPARE(list.rows[0][0], QStringLiteral("Chair, Wooden"));
    QCOMPARE(list.rows[0][1], QStringLiteral("5"));
}

void TestOpalList::testCommentsAndPadding()
{
    const QString content = QStringLiteral(
        "# header comment\n"
        "A,B,C\n"
        "1,2\n"
        "3,4,5\n");
    const OpalList list = OpalList::parse(content);
    QCOMPARE(list.rowCount(), 2);
    QCOMPARE(list.rows[0].size(), 3);
    QCOMPARE(list.rows[0][2], QString()); // padded
    QCOMPARE(list.rows[1][2], QStringLiteral("5"));
}

void TestOpalList::testLoadFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("FormID,Count\n0005,2\n");
    file.close();

    OpalList list;
    QVERIFY(OpalList::loadFile(file.fileName(), list));
    QCOMPARE(list.rowCount(), 1);
    QCOMPARE(list.rows[0][0], QStringLiteral("0005"));

    OpalList missing;
    QVERIFY(!OpalList::loadFile(QStringLiteral("Z:/missing.opl"), missing));
}

void TestOpalList::testValueLookup()
{
    const QString content = QStringLiteral(
        "FormID,Count,Chance\n"
        "0001,3,50\n");
    const OpalList list = OpalList::parse(content);
    QCOMPARE(list.value(0, QStringLiteral("FormID")), QStringLiteral("0001"));
    QCOMPARE(list.value(0, QStringLiteral("Chance")), QStringLiteral("50"));
    QCOMPARE(list.value(0, QStringLiteral("Missing")), QString());
    QCOMPARE(list.value(5, QStringLiteral("FormID")), QString());
}

QTEST_MAIN(TestOpalList)
#include "test_opallist.moc"

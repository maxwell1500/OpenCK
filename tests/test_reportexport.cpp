#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../src/model/doc/messages.hpp"
#include "../../src/model/tools/reports.hpp"
#include "../../libs/files/log/logger.hpp"

class TestReportExport : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testTextFormat();
    void testExportFile();
    void testEmptyMessages();
    void testSanitization();
};

void TestReportExport::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_reportexport_log.txt"));
}

void TestReportExport::testTextFormat()
{
    QVector<Message> messages;
    messages.append(Message(CkId(CkId::Type_Npc_, QStringLiteral("NPC001")),
        QStringLiteral("Missing editor ID"), QStringLiteral("Fix it"), Message::Warning));

    const QString text = ReportExport::messagesToText(messages);
    QVERIFY(text.startsWith(QStringLiteral("Level\tType\tID\tMessage\tHint\n")));
    QVERIFY(text.contains(QStringLiteral("Warning\tActor\tNPC001\tMissing editor ID\tFix it")));
}

void TestReportExport::testExportFile()
{
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    const QString path = tmpFile.fileName();
    tmpFile.close();

    QVector<Message> messages;
    messages.append(Message(CkId(CkId::Type_Weap_, QStringLiteral("WPN")),
        QStringLiteral("Bad value"), QStringLiteral(""), Message::Error));
    messages.append(Message(CkId(CkId::Type_Book_, QStringLiteral("BK")),
        QStringLiteral("Duplicate"), QStringLiteral("Rename"), Message::Warning));

    QVERIFY(ReportExport::exportMessages(path, messages));

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(f.readAll());
    f.close();

    QVERIFY(content.contains(QStringLiteral("Error\tWeapon\tWPN\tBad value\t")));
    QVERIFY(content.contains(QStringLiteral("Warning\tBook\tBK\tDuplicate\tRename")));
}

void TestReportExport::testEmptyMessages()
{
    const QString text = ReportExport::messagesToText(QVector<Message>());
    QVERIFY(text.startsWith(QStringLiteral("Level\tType\tID\tMessage\tHint\n")));
    QVERIFY(text.endsWith('\n'));
}

void TestReportExport::testSanitization()
{
    QVector<Message> messages;
    messages.append(Message(CkId(CkId::Type_Npc_, QStringLiteral("X")),
        QStringLiteral("tab\there\nnewline"), QStringLiteral("hint\twith\ttabs"), Message::Info));

    const QString text = ReportExport::messagesToText(messages);
    // Tabs and newlines inside a message must be replaced so the TSV stays valid.
    QVERIFY(text.contains(QStringLiteral("\nInformation\tActor\tX\ttab here newline\thint with tabs\n")));
    const int lines = text.count('\n');
    QCOMPARE(lines, 2); // header + one data row
}

QTEST_MAIN(TestReportExport)
#include "test_reportexport.moc"

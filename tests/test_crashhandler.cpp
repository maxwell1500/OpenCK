#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

#include "../../src/crashhandler.hpp"
#include "../../libs/files/log/logger.hpp"

class TestCrashHandler : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testInstallHandlers();
    void testWriteStackTrace();
    void testWriteCrashBundle();
};

void TestCrashHandler::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_crashhandler_log.txt"));
}

void TestCrashHandler::testInstallHandlers()
{
    // Installing terminate/signal handlers must be safe to call at startup.
    OpenCK::installCrashHandlers();
    QVERIFY(true);
}

void TestCrashHandler::testWriteStackTrace()
{
    // Walking the current thread's stack must not crash and must log.
    OpenCK::writeStackTrace("test-probe");

    const QString logPath = OpenCK::Logging::Logger::instance().logFilePath();
    QVERIFY(!logPath.isEmpty());
    QFile log(logPath);
    QVERIFY(log.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(log.readAll());
    log.close();
    QVERIFY(content.contains(QStringLiteral("Stack trace requested: test-probe")));
}

void TestCrashHandler::testWriteCrashBundle()
{
    const QString bundle = OpenCK::writeCrashBundle();
    QVERIFY(!bundle.isEmpty());
    QVERIFY(QFile::exists(bundle));
    QFile::remove(bundle);
}

QTEST_MAIN(TestCrashHandler)
#include "test_crashhandler.moc"

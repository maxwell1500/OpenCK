#include <QTest>
#include <QTemporaryDir>
#include <QFile>

#include "../../src/model/tools/perforcerepository.hpp"
#include "../../libs/files/log/logger.hpp"

// Validates the PerforceRepository p4 CLI wrapper. Requires p4 on PATH and a
// configured workspace; without them the tests skip (this machine has no p4
// server, so the availability check is what is exercised here).
class TestPerforceRepository : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testIsAvailable();
    void testRunWithoutWorkspace();
};

void TestPerforceRepository::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_perforce_log.txt"));
}

void TestPerforceRepository::testIsAvailable()
{
    const bool available = PerforceRepository::isAvailable();
    if (!available) {
        QSKIP("p4 is not on PATH; skipping Perforce tests");
    }
    QVERIFY(available);
}

void TestPerforceRepository::testRunWithoutWorkspace()
{
    if (!PerforceRepository::isAvailable()) {
        QSKIP("p4 is not on PATH");
    }

    // A temp dir is not inside any Perforce workspace; p4 info must fail and
    // isWorkspace must return false. clientName must be empty.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(!PerforceRepository::isWorkspace(dir.path()));
    QVERIFY(PerforceRepository::clientName(dir.path()).isEmpty());
}

QTEST_MAIN(TestPerforceRepository)
#include "test_perforcerepository.moc"

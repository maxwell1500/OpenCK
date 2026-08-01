#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>

#include "../../src/model/tools/gitrepository.hpp"
#include "../../libs/files/log/logger.hpp"

class TestGitRepository : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testIsAvailable();
    void testNotARepository();
    void testInitAndStatus();
    void testStageCommitStatus();
    void testBranchName();
};

void TestGitRepository::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_gitrepo_log.txt"));
}

void TestGitRepository::testIsAvailable()
{
    // If git is installed this must be true; otherwise the test would be a
    // no-op and we just log it.
    const bool available = GitRepository::isAvailable();
    if (!available) {
        QSKIP("git is not on PATH; skipping repository tests");
    }
    QVERIFY(available);
}

void TestGitRepository::testNotARepository()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(!GitRepository::isRepository(dir.path()));
    QVERIFY(GitRepository::currentBranch(dir.path()).isEmpty());
}

void TestGitRepository::testInitAndStatus()
{
    if (!GitRepository::isAvailable()) {
        QSKIP("git is not on PATH");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const GitRepository::Result init = GitRepository::run(dir.path(), { "init", "-q" });
    QVERIFY(init.ok);
    QVERIFY(GitRepository::isRepository(dir.path()));
    QVERIFY(!GitRepository::gitDir(dir.path()).isEmpty());

    // Configure a local identity so commits work on this machine.
    GitRepository::run(dir.path(), { "config", "user.email", "test@example.com" });
    GitRepository::run(dir.path(), { "config", "user.name", "Test" });

    QFile file(dir.path() + "/plugin.esp");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("plugin data");
    file.close();

    const GitRepository::Result status = GitRepository::status(dir.path());
    QVERIFY(status.ok);
    QVERIFY(status.stdoutText.contains("plugin.esp"));
}

void TestGitRepository::testStageCommitStatus()
{
    if (!GitRepository::isAvailable()) {
        QSKIP("git is not on PATH");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    GitRepository::run(dir.path(), { "init", "-q" });
    GitRepository::run(dir.path(), { "config", "user.email", "test@example.com" });
    GitRepository::run(dir.path(), { "config", "user.name", "Test" });

    QFile file(dir.path() + "/mod.esp");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("data");
    file.close();

    const GitRepository::Result staged = GitRepository::stageFiles(dir.path(), { "mod.esp" });
    QVERIFY(staged.ok);

    const GitRepository::Result committed = GitRepository::commitFiles(
        dir.path(), { "mod.esp" }, "Initial import");
    QVERIFY(committed.ok);
    QVERIFY(committed.stdoutText.contains("mod.esp")
        || committed.stdoutText.contains("1 file changed"));

    const GitRepository::Result status = GitRepository::status(dir.path());
    QVERIFY(status.ok);
    QVERIFY(!status.stdoutText.contains("mod.esp")); // clean now
}

void TestGitRepository::testBranchName()
{
    if (!GitRepository::isAvailable()) {
        QSKIP("git is not on PATH");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    GitRepository::run(dir.path(), { "init", "-q" });
    GitRepository::run(dir.path(), { "config", "user.email", "test@example.com" });
    GitRepository::run(dir.path(), { "config", "user.name", "Test" });

    // Empty repo has no commits -> branch name should be empty or a placeholder.
    const QString branch = GitRepository::currentBranch(dir.path());
    QVERIFY(branch.isEmpty() || branch == QStringLiteral("(detached)"));
}

QTEST_MAIN(TestGitRepository)
#include "test_gitrepository.moc"

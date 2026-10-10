#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>

#include "src/model/tools/gitrepository.hpp"
#include "libs/files/log/logger.hpp"

// Phase 10.2: the version-control panel's Git wrapper. These tests run the
// real `git` binary against throwaway repositories in a temp dir — a mock
// would prove nothing about branch switching or commit plumbing.
class TestGitIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testIsAvailable();
    void testInitAndCommit();
    void testBranchesAndCheckout();
    void testStatusAndDiffStat();
    void testNonRepository();
};

void TestGitIntegration::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Warning);
    OpenCK::Logging::Logger::instance().init(
        QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_gitintegration_log.txt"))
    ;
    // Deterministic committer identity for repos without user config.
    qputenv("GIT_AUTHOR_NAME", "Test");
    qputenv("GIT_AUTHOR_EMAIL", "test@example.com");
    qputenv("GIT_COMMITTER_NAME", "Test");
    qputenv("GIT_COMMITTER_EMAIL", "test@example.com");
}

void TestGitIntegration::testIsAvailable()
{
    // The whole feature is opt-in around the system git; on machines
    // without it the panel must disable itself rather than half-work.
    QVERIFY(GitRepository::isAvailable());
}

void TestGitIntegration::testInitAndCommit()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // Init via git itself: the wrapper only wraps, it does not re-implement.
    QVERIFY(GitRepository::run(dir.path(), { "init", "-q" }).ok);
    QVERIFY(GitRepository::isRepository(dir.path()));

    QFile f(QDir(dir.path()).filePath("plugin.esp"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("TES4");
    f.close();

    const GitRepository::Result staged =
        GitRepository::stageFiles(dir.path(), { "plugin.esp" });
    QVERIFY2(staged.ok, qPrintable(staged.stderrText));

    const GitRepository::Result committed = GitRepository::commitFiles(
        dir.path(), { "plugin.esp" }, "initial");
    QVERIFY2(committed.ok, qPrintable(committed.stderrText));

    // Nothing staged/modified left: the commit took the only change.
    const GitRepository::Result status = GitRepository::status(dir.path());
    QCOMPARE(status.stdoutText.trimmed(), QString());
    QCOMPARE(GitRepository::currentBranch(dir.path()), QString("master"));
}

void TestGitIntegration::testBranchesAndCheckout()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(GitRepository::run(dir.path(), { "init", "-q" }).ok);

    QFile f(QDir(dir.path()).filePath("readme.txt"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("base");
    f.close();
    QVERIFY(GitRepository::stageFiles(dir.path(), { "readme.txt" }).ok);
    QVERIFY(GitRepository::commitFiles(dir.path(), { "readme.txt" }, "base").ok);

    QVERIFY(GitRepository::run(dir.path(), { "branch", "feature" }).ok);

    const QStringList branches = GitRepository::branches(dir.path());
    QVERIFY(branches.contains("feature"));
    QVERIFY(branches.contains(GitRepository::currentBranch(dir.path())));

    // Switching branches must work and change the working tree.
    const GitRepository::Result switched =
        GitRepository::checkout(dir.path(), "feature");
    QVERIFY2(switched.ok, qPrintable(switched.stderrText));
    QCOMPARE(GitRepository::currentBranch(dir.path()), QString("feature"));

    // A branch that does not exist must fail, not silently stay put.
    const GitRepository::Result bad =
        GitRepository::checkout(dir.path(), "nope-not-a-branch");
    QVERIFY(!bad.ok);
    QCOMPARE(GitRepository::currentBranch(dir.path()), QString("feature"));
}

void TestGitIntegration::testStatusAndDiffStat()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(GitRepository::run(dir.path(), { "init", "-q" }).ok);

    QFile f(QDir(dir.path()).filePath("tracked.txt"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("one");
    f.close();
    QVERIFY(GitRepository::stageFiles(dir.path(), { "tracked.txt" }).ok);
    QVERIFY(GitRepository::commitFiles(dir.path(), { "tracked.txt" }, "v1").ok);

    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("two");
    f.close();

    const GitRepository::Result status = GitRepository::status(dir.path());
    QVERIFY2(status.ok, qPrintable(status.stderrText));
    QVERIFY2(status.stdoutText.contains("tracked.txt"),
             qPrintable(status.stdoutText));

    const GitRepository::Result diff = GitRepository::diffStat(dir.path());
    QVERIFY(diff.stdoutText.contains("tracked.txt"));

    // Commit the modification away and the status must go clean. `git commit
    // <paths>` alone does not stage, so stage first like the UI does.
    QVERIFY(GitRepository::stageFiles(dir.path(), { "tracked.txt" }).ok);
    QVERIFY(GitRepository::commitFiles(dir.path(), { "tracked.txt" }, "v2").ok);
    QCOMPARE(GitRepository::status(dir.path()).stdoutText.trimmed(), QString());
}

void TestGitIntegration::testNonRepository()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // A plain temp dir is not a repo: operations must report it, not hang
    // or half-run.
    QVERIFY(!GitRepository::isRepository(dir.path()));
    QCOMPARE(GitRepository::currentBranch(dir.path()), QString());
    QVERIFY(GitRepository::branches(dir.path()).isEmpty());
    QVERIFY(GitRepository::gitDir(dir.path()).isEmpty());
}

QTEST_APPLESS_MAIN(TestGitIntegration)
#include "test_gitintegration.moc"

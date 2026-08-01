#include "gitrepository.hpp"

#include <QProcess>
#include <QDir>
#include <QFileInfo>

#include "../../files/log/logger.hpp"

namespace {

GitRepository::Result runProcess(const QString& dir, const QStringList& args)
{
    GitRepository::Result result;
    QProcess process;
    process.setWorkingDirectory(dir);
    process.start(QStringLiteral("git"), args);
    if (!process.waitForStarted(5000))
    {
        result.stderrText = QStringLiteral("git did not start");
        return result;
    }
    if (!process.waitForFinished(30000))
    {
        process.kill();
        result.stderrText = QStringLiteral("git timed out");
        return result;
    }
    result.exitCode = process.exitCode();
    result.stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    result.stderrText = QString::fromUtf8(process.readAllStandardError());
    result.ok = (result.exitCode == 0);
    return result;
}

} // namespace

bool GitRepository::isAvailable()
{
    QProcess process;
    process.start(QStringLiteral("git"), { QStringLiteral("--version") });
    if (!process.waitForStarted(5000)) return false;
    if (!process.waitForFinished(10000)) return false;
    return process.exitCode() == 0;
}

GitRepository::Result GitRepository::run(const QString& dir, const QStringList& args)
{
    return runProcess(dir, args);
}

bool GitRepository::isRepository(const QString& dir)
{
    const Result r = runProcess(dir, { QStringLiteral("rev-parse"),
        QStringLiteral("--is-inside-work-tree") });
    return r.ok && r.stdoutText.trimmed() == QLatin1String("true");
}

QString GitRepository::gitDir(const QString& dir)
{
    const Result r = runProcess(dir, { QStringLiteral("rev-parse"),
        QStringLiteral("--absolute-git-dir") });
    return r.ok ? r.stdoutText.trimmed() : QString();
}

GitRepository::Result GitRepository::commitFiles(const QString& dir, const QStringList& paths,
                                                 const QString& message)
{
    QStringList args = { QStringLiteral("commit"), QStringLiteral("-m"), message };
    args += paths;
    const Result r = runProcess(dir, args);
    if (!r.ok)
    {
        LOG_WARNING(QString("git commit failed in %1: %2").arg(dir, r.stderrText.trimmed()));
    }
    return r;
}

GitRepository::Result GitRepository::stageFiles(const QString& dir, const QStringList& paths)
{
    QStringList args = { QStringLiteral("add"), QStringLiteral("--") };
    args += paths;
    return runProcess(dir, args);
}

GitRepository::Result GitRepository::status(const QString& dir)
{
    return runProcess(dir, { QStringLiteral("status"), QStringLiteral("--short") });
}

GitRepository::Result GitRepository::diffStat(const QString& dir)
{
    return runProcess(dir, { QStringLiteral("diff"), QStringLiteral("--stat") });
}

QString GitRepository::currentBranch(const QString& dir)
{
    const Result r = runProcess(dir, { QStringLiteral("rev-parse"),
        QStringLiteral("--abbrev-ref"), QStringLiteral("HEAD") });
    if (r.ok)
    {
        const QString branch = r.stdoutText.trimmed();
        return branch == QLatin1String("HEAD") ? QStringLiteral("(detached)") : branch;
    }
    return QString();
}

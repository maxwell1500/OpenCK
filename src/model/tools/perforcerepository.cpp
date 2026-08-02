#include "perforcerepository.hpp"

#include <QProcess>
#include <QDir>

#include "../../files/log/logger.hpp"

namespace {

PerforceRepository::Result runProcess(const QString& dir, const QStringList& args)
{
    PerforceRepository::Result result;
    QProcess process;
    process.setWorkingDirectory(dir);
    process.start(QStringLiteral("p4"), args);
    if (!process.waitForStarted(5000))
    {
        result.stderrText = QStringLiteral("p4 did not start");
        return result;
    }
    if (!process.waitForFinished(30000))
    {
        process.kill();
        result.stderrText = QStringLiteral("p4 timed out");
        return result;
    }
    result.exitCode = process.exitCode();
    result.stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    result.stderrText = QString::fromUtf8(process.readAllStandardError());
    result.ok = (result.exitCode == 0);
    return result;
}

} // namespace

bool PerforceRepository::isAvailable()
{
    QProcess process;
    process.start(QStringLiteral("p4"), { QStringLiteral("-V") });
    if (!process.waitForStarted(5000)) return false;
    if (!process.waitForFinished(10000)) return false;
    return process.exitCode() == 0;
}

PerforceRepository::Result PerforceRepository::run(const QString& dir, const QStringList& args)
{
    return runProcess(dir, args);
}

bool PerforceRepository::isWorkspace(const QString& dir)
{
    // p4 info with a directory context fails when there is no matching client.
    const Result r = runProcess(dir, { QStringLiteral("-d"), dir,
        QStringLiteral("info") });
    return r.ok && r.stdoutText.contains(QStringLiteral("Client name:"), Qt::CaseInsensitive);
}

QString PerforceRepository::clientName(const QString& dir)
{
    const Result r = runProcess(dir, { QStringLiteral("-d"), dir,
        QStringLiteral("info") });
    if (!r.ok) return QString();
    for (const auto& line : r.stdoutText.split(QLatin1Char('\n')))
    {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("Client name:"), Qt::CaseInsensitive))
            return trimmed.section(QLatin1Char(':'), 1).trimmed();
    }
    return QString();
}

PerforceRepository::Result PerforceRepository::checkOut(const QString& dir, const QStringList& paths)
{
    QStringList args = { QStringLiteral("edit") };
    args += paths;
    return runProcess(dir, args);
}

PerforceRepository::Result PerforceRepository::checkIn(const QString& dir, const QStringList& paths,
                                                       const QString& message)
{
    QStringList args = { QStringLiteral("submit"), QStringLiteral("-d"), message };
    args += paths;
    const Result r = runProcess(dir, args);
    if (!r.ok)
    {
        LOG_WARNING(QString("p4 submit failed in %1: %2").arg(dir, r.stderrText.trimmed()));
    }
    return r;
}

PerforceRepository::Result PerforceRepository::revert(const QString& dir, const QStringList& paths)
{
    QStringList args = { QStringLiteral("revert") };
    args += paths;
    return runProcess(dir, args);
}

PerforceRepository::Result PerforceRepository::opened(const QString& dir)
{
    return runProcess(dir, { QStringLiteral("opened") });
}

PerforceRepository::Result PerforceRepository::diffStat(const QString& dir)
{
    return runProcess(dir, { QStringLiteral("diff"), QStringLiteral("-sd") });
}

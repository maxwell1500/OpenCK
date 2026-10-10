#include "moddeploymentresolver.hpp"
#include "logger.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#ifdef _WIN32
#include <windows.h>
#endif

QString ModDeploymentResolver::normalizeRelPath(const QString& relPath)
{
    return QDir::fromNativeSeparators(relPath);
}

bool ModDeploymentResolver::parseVortexDeployment(const QByteArray& json,
                                                  VortexDeployment& out)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
    {
        LOG_WARNING(QString("ModDeploymentResolver: bad deployment JSON: %1")
                    .arg(err.errorString()));
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonValue filesValue = root.value(QStringLiteral("files"));
    if (!filesValue.isArray() || filesValue.toArray().isEmpty())
    {
        LOG_WARNING("ModDeploymentResolver: deployment JSON has no files array");
        return false;
    }

    out = VortexDeployment();
    out.instance = root.value(QStringLiteral("instance")).toString();
    out.version = root.value(QStringLiteral("version")).toInt();
    out.deploymentMethod =
        root.value(QStringLiteral("deploymentMethod")).toString();
    out.gameId = root.value(QStringLiteral("gameId")).toString();
    out.deploymentTime =
        static_cast<qint64>(root.value(QStringLiteral("deploymentTime")).toDouble());
    out.stagingPath =
        QDir::fromNativeSeparators(root.value(QStringLiteral("stagingPath")).toString());
    out.targetPath =
        QDir::fromNativeSeparators(root.value(QStringLiteral("targetPath")).toString());

    if (out.targetPath.isEmpty())
    {
        LOG_WARNING("ModDeploymentResolver: deployment JSON has no targetPath");
        return false;
    }

    const QJsonArray files = filesValue.toArray();
    out.files.reserve(files.size());
    for (const QJsonValue& entryValue : files)
    {
        const QJsonObject entry = entryValue.toObject();
        if (entry.isEmpty())
        {
            continue;
        }
        DeployedFile file;
        file.relPath = normalizeRelPath(
            entry.value(QStringLiteral("relPath")).toString());
        file.source = entry.value(QStringLiteral("source")).toString();
        file.target = QDir::fromNativeSeparators(
            entry.value(QStringLiteral("target")).toString());
        file.time =
            static_cast<qint64>(entry.value(QStringLiteral("time")).toDouble());
        if (file.relPath.isEmpty())
        {
            continue;
        }
        out.files.append(file);
    }

    if (out.files.isEmpty())
    {
        LOG_WARNING("ModDeploymentResolver: deployment files array was empty");
        return false;
    }
    return true;
}

bool ModDeploymentResolver::readVortexDeployment(const QString& path,
                                                 VortexDeployment& out)
{
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("ModDeploymentResolver: cannot open %1").arg(path));
        return false;
    }
    const QByteArray payload = file.readAll();
    file.close();
    return parseVortexDeployment(payload, out);
}

QString ModDeploymentResolver::findVortexDeployment(const QString& gameDataDir)
{
    // "vortex.deployment.json" names the game-root deployment; a Data-folder
    // deployment writes "vortex.deployment.<folder>.json" next to it.
    const QStringList filters{ QStringLiteral("vortex.deployment*.json") };
    const QFileInfoList candidates =
        QDir(gameDataDir).entryInfoList(filters, QDir::Files);
    if (candidates.isEmpty())
    {
        return QString();
    }
    return candidates.first().absoluteFilePath();
}

QString ModDeploymentResolver::deployedFilePath(const VortexDeployment& deployment,
                                                const QString& relPath)
{
    const QString wanted = normalizeRelPath(relPath);
    for (const DeployedFile& file : deployment.files)
    {
        if (file.relPath.compare(wanted, Qt::CaseInsensitive) == 0)
        {
            QDir base(deployment.targetPath);
            if (!file.target.isEmpty())
            {
                base = QDir(base.filePath(file.target));
            }
            return QDir::toNativeSeparators(base.filePath(file.relPath));
        }
    }
    return QString();
}

QString ModDeploymentResolver::sourceFilePath(const VortexDeployment& deployment,
                                              const QString& relPath)
{
    const QString wanted = normalizeRelPath(relPath);
    for (const DeployedFile& file : deployment.files)
    {
        if (file.relPath.compare(wanted, Qt::CaseInsensitive) == 0)
        {
            if (file.source.isEmpty() || deployment.stagingPath.isEmpty())
            {
                return QString();
            }
            const QString path = QDir(QDir(deployment.stagingPath).filePath(file.source))
                .filePath(file.relPath);
            return QDir::toNativeSeparators(path);
        }
    }
    return QString();
}

ModDeploymentResolver::LinkKind
ModDeploymentResolver::linkStatus(const QString& a, const QString& b)
{
#ifdef _WIN32
    QFileInfo infoA(a);
    QFileInfo infoB(b);
    if (!infoA.exists() || !infoB.exists())
    {
        return LinkKind::Unknown;
    }
    if (infoA.isSymLink() || infoB.isSymLink())
    {
        return LinkKind::Symlink;
    }

    auto fileId = [](const QString& path, quint64* id) -> bool
    {
        const HANDLE handle = CreateFileW(
            reinterpret_cast<LPCWSTR>(path.utf16()), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }
        BY_HANDLE_FILE_INFORMATION info{};
        const bool ok = GetFileInformationByHandle(handle, &info) != FALSE;
        CloseHandle(handle);
        if (ok)
        {
            *id = (static_cast<quint64>(info.nFileIndexHigh) << 32)
                | info.nFileIndexLow;
        }
        return ok;
    };

    quint64 idA = 0;
    quint64 idB = 0;
    if (!fileId(a, &idA) || !fileId(b, &idB))
    {
        return LinkKind::Unknown;
    }
    // Hardlinks share a file id but keep separate directory entries, so a
    // different name with the same id is the deployed-vs-staged pair Vortex
    // produces with its hardlink_activator method.
    if (idA == idB)
    {
        return LinkKind::Hardlink;
    }
    return LinkKind::Distinct;
#else
    Q_UNUSED(a)
    Q_UNUSED(b)
    return LinkKind::Unknown;
#endif
}

QString ModDeploymentResolver::linkKindName(LinkKind kind)
{
    switch (kind)
    {
    case LinkKind::SameFile:
        return QStringLiteral("SameFile");
    case LinkKind::Hardlink:
        return QStringLiteral("Hardlink");
    case LinkKind::Symlink:
        return QStringLiteral("Symlink");
    case LinkKind::Distinct:
        return QStringLiteral("Distinct");
    case LinkKind::Unknown:
        return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

bool ModDeploymentResolver::parseMo2ModList(const QString& content,
                                            QVector<ModEntry>& out)
{
    out.clear();
    const QStringList lines = content.split(QLatin1Char('\n'));
    for (const QString& raw : lines)
    {
        const QString line = raw.trimmed();
        if (line.isEmpty())
        {
            continue;
        }
        ModEntry entry;
        if (line.startsWith(QLatin1Char('+')))
        {
            entry.name = line.mid(1).trimmed();
            entry.enabled = true;
        }
        else if (line.startsWith(QLatin1Char('-')))
        {
            entry.name = line.mid(1).trimmed();
            entry.enabled = false;
        }
        else if (line.startsWith(QLatin1Char('*')))
        {
            // MO2 separator line; carries no mod name.
            continue;
        }
        else
        {
            continue;
        }
        if (entry.name.isEmpty())
        {
            continue;
        }
        out.append(entry);
    }
    return !out.isEmpty();
}

bool ModDeploymentResolver::readMo2ModList(const QString& path,
                                           QVector<ModEntry>& out)
{
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("ModDeploymentResolver: cannot open %1").arg(path));
        return false;
    }
    const QString content = QString::fromUtf8(file.readAll());
    file.close();
    return parseMo2ModList(content, out);
}

QStringList ModDeploymentResolver::enabledMods(const QVector<ModEntry>& entries)
{
    QStringList names;
    for (const ModEntry& entry : entries)
    {
        if (entry.enabled)
        {
            names.append(entry.name);
        }
    }
    return names;
}

QString ModDeploymentResolver::winningMod(const QVector<ModEntry>& entries,
                                          const QString& modsDir,
                                          const QString& relPath,
                                          QString* fullPath)
{
    const QString wanted = normalizeRelPath(relPath);

    // Last enabled line wins in MO2, so walk the list backwards and stop at
    // the first mod that actually ships the file.
    for (int i = entries.size() - 1; i >= 0; --i)
    {
        const ModEntry& entry = entries.at(i);
        if (!entry.enabled)
        {
            continue;
        }
        const QString candidate =
            QDir(QDir(modsDir).filePath(entry.name)).filePath(wanted);
        if (QFile::exists(candidate))
        {
            if (fullPath)
            {
                *fullPath = QDir::toNativeSeparators(candidate);
            }
            return entry.name;
        }
    }
    return QString();
}

QVector<QPair<QString, QString>>
ModDeploymentResolver::overrideChain(const QVector<ModEntry>& entries,
                                     const QString& modsDir,
                                     const QString& relPath)
{
    QVector<QPair<QString, QString>> chain;
    const QString wanted = normalizeRelPath(relPath);
    for (int i = entries.size() - 1; i >= 0; --i)
    {
        const ModEntry& entry = entries.at(i);
        if (!entry.enabled)
        {
            continue;
        }
        const QString candidate =
            QDir(QDir(modsDir).filePath(entry.name)).filePath(wanted);
        if (QFile::exists(candidate))
        {
            chain.append({ entry.name, QDir::toNativeSeparators(candidate) });
        }
    }
    return chain;
}

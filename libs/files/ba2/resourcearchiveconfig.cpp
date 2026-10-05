#include "resourcearchiveconfig.hpp"

#include "ini/inifile.hpp"

#include <QDir>
#include <QFileInfo>

ResourceArchiveConfig ResourceArchiveConfig::fromIni(const QString& iniPath)
{
    ResourceArchiveConfig config;
    IniFile ini;
    if (!ini.load(iniPath))
        return config;

    // Key and section names, not user-facing text: these have to match the file
    // we are reading for interoperability.
    config.mResourceArchives = ini.valueList(QStringLiteral("Archive"),
                                             QStringLiteral("SResourceArchiveList"));
    config.mResourceIndexArchives = ini.valueList(QStringLiteral("Archive"),
                                                  QStringLiteral("sResourceIndexFileList"));
    config.mDefaultExternalCodecId = ini.hasKey(QStringLiteral("Wwise"),
                                                QStringLiteral("iDefaultExternalCodecID"))
        ? ini.intValue(QStringLiteral("Wwise"), QStringLiteral("iDefaultExternalCodecID"), -1)
        : -1;
    return config;
}

QStringList ResourceArchiveConfig::allNamedArchives() const
{
    QStringList out = mResourceArchives;
    for (const QString& name : mResourceIndexArchives) {
        if (!out.contains(name, Qt::CaseInsensitive))
            out.append(name);
    }
    return out;
}

ResourceArchiveConfig::Resolution ResourceArchiveConfig::resolve(const QStringList& names,
                                                                 const QString& dataDir)
{
    Resolution result;
    if (names.isEmpty() || dataDir.isEmpty())
        return result;

    // Case-insensitive lookup of every archive actually present, so a configured
    // name matches regardless of how the file is cased on disk.
    QMap<QString, QString> byName;
    const QDir dir(dataDir);
    const QFileInfoList entries = dir.entryInfoList({ QStringLiteral("*.bsa"),
                                                      QStringLiteral("*.BA2") },
                                                   QDir::Files, QDir::Name);
    for (const QFileInfo& info : entries)
        byName.insert(info.fileName().toLower(), info.absoluteFilePath());
    // DLC archives live one level down, and a configured list routinely names them.
    const QFileInfoList nested = dir.entryInfoList(QDir::Dirs, QDir::Name);
    for (const QFileInfo& sub : nested) {
        const QDir subDir(sub.absoluteFilePath());
        for (const QFileInfo& info : subDir.entryInfoList({ QStringLiteral("*.bsa"),
                                                            QStringLiteral("*.BA2") },
                                                         QDir::Files, QDir::Name)) {
            if (!byName.contains(info.fileName().toLower()))
                byName.insert(info.fileName().toLower(), info.absoluteFilePath());
        }
    }

    for (const QString& name : names) {
        const QString key = name.trimmed().toLower();
        if (key.isEmpty())
            continue;
        const auto it = byName.constFind(key);
        if (it == byName.constEnd())
            result.missing.append(name);
        else
            result.found.insert(name, it.value());
    }
    return result;
}

bool ResourceArchiveConfig::namesArchive(const QString& archiveFileName) const
{
    const QString key = QFileInfo(archiveFileName).fileName().toLower();
    if (key.isEmpty())
        return false;
    for (const QString& name : allNamedArchives()) {
        if (QFileInfo(name).fileName().toLower() == key)
            return true;
    }
    return false;
}
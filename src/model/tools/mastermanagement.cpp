#include "mastermanagement.hpp"

#include <QFile>
#include <QTextStream>
#include <QSet>

#include "libs/files/log/logger.hpp"

QString MasterManagement::updateSourceToString(UpdateSource source)
{
    switch (source)
    {
    case UpdateSource::MastersOnly: return QStringLiteral("Masters Only");
    case UpdateSource::MastersAndMods: return QStringLiteral("Masters and Mods");
    case UpdateSource::LocalOnly: return QStringLiteral("Local Only");
    }
    return QStringLiteral("Unknown");
}

MasterManagement::UpdateSource MasterManagement::stringToUpdateSource(
    const QString& text)
{
    const QString upper = text.trimmed().toUpper();
    if (upper == QStringLiteral("MASTERSONLY") || upper == QStringLiteral("1"))
        return UpdateSource::MastersOnly;
    if (upper == QStringLiteral("LOCALONLY") || upper == QStringLiteral("2"))
        return UpdateSource::LocalOnly;
    return UpdateSource::MastersAndMods;
}

QStringList MasterManagement::knownKeys()
{
    return {
        QStringLiteral("MMS_UpdateMasterFromFile"),
        QStringLiteral("MMS_UseLocalFileForUpdate"),
        QStringLiteral("MMS_UpdateFile"),
        QStringLiteral("MMS_ReuseDeletedRecordIDs"),
        QStringLiteral("MMS_StartLocalID"),
    };
}

bool MasterManagement::applyIniLine(const QString& key, const QString& value)
{
    const QString k = key.trimmed().toLower();
    const QString v = value.trimmed();
    rawKeys.insert(key.trimmed(), v);

    if (k == QStringLiteral("mms_updatemasterfromfile"))
    {
        updateSource = stringToUpdateSource(v);
        return true;
    }
    if (k == QStringLiteral("mms_uselocalfileforupdate"))
    {
        useLocalFileForUpdate = (v.toLower() == QStringLiteral("1")
                                 || v.toLower() == QStringLiteral("true"));
        return true;
    }
    if (k == QStringLiteral("mms_updatefile"))
    {
        updateFile = v;
        return true;
    }
    if (k == QStringLiteral("mms_reusedeletedrecordids"))
    {
        allocation.reuseDeleted = (v.toLower() == QStringLiteral("1")
                                   || v.toLower() == QStringLiteral("true"));
        return true;
    }
    if (k == QStringLiteral("mms_startlocalid"))
    {
        bool ok = false;
        const int start = v.toInt(&ok);
        if (ok && start >= 1)
        {
            allocation.startLocalId = static_cast<quint32>(start);
            allocation.nextLocalId = static_cast<quint32>(start);
        }
        return true;
    }
    return false;
}

bool MasterManagement::loadIni(const QString& iniPath)
{
    QFile file(iniPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG_WARNING(QString("MasterManagement: cannot open %1").arg(iniPath));
        return false;
    }

    bool inMmsSection = false;
    bool sawMms = false;
    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(';') || trimmed.startsWith('#'))
            continue;

        if (trimmed.startsWith('['))
        {
            const QString section = trimmed.mid(1, trimmed.length() - 2).trimmed();
            inMmsSection = section.compare(QStringLiteral("MMS"), Qt::CaseInsensitive) == 0;
            if (inMmsSection)
                sawMms = true;
            continue;
        }

        if (!inMmsSection)
            continue;

        const int eq = trimmed.indexOf('=');
        if (eq <= 0)
            continue;
        const QString key = trimmed.left(eq).trimmed();
        const QString value = trimmed.mid(eq + 1).trimmed();
        applyIniLine(key, value);
    }

    if (!sawMms)
        LOG_DEBUG(QString("MasterManagement: no [MMS] section in %1").arg(iniPath));
    return true;
}

quint32 MasterManagement::allocateLocalId(const QVector<quint32>& usedLocalIds)
{
    QSet<quint32> used(usedLocalIds.begin(), usedLocalIds.end());

    quint32 candidate = allocation.nextLocalId;
    if (candidate < allocation.startLocalId)
        candidate = allocation.startLocalId;

    if (allocation.reuseDeleted)
    {
        // Scan forward for the first free ID (may reuse a hole).
        while (used.contains(candidate) && candidate < 0x00FFFFFFu)
            ++candidate;
    }
    else
    {
        // Only skip the next gapThreshold occupied IDs, then jump.
        int skipped = 0;
        while (used.contains(candidate) && skipped < allocation.gapThreshold
               && candidate < 0x00FFFFFFu)
        {
            ++candidate;
            ++skipped;
        }
    }

    allocation.nextLocalId = candidate + 1;
    return candidate;
}

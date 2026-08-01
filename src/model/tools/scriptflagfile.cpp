#include "scriptflagfile.hpp"

#include <QFile>
#include <QSet>

#include "../../files/log/logger.hpp"

QVector<ScriptFlagFile::Entry> ScriptFlagFile::parse(const QString& content)
{
    QVector<Entry> entries;
    const QStringList lines = content.split('\n');

    for (QString line : lines)
    {
        line = line.trimmed();
        if (line.isEmpty())
            continue;
        if (line.startsWith(';') || line.startsWith('#'))
            continue;

        // "ScriptName = FlagA|FlagB"
        const int eq = line.indexOf('=');
        const QString scriptName = (eq >= 0 ? line.left(eq) : line).trimmed();
        const QString flagPart = (eq >= 0 ? line.mid(eq + 1) : QString()).trimmed();

        if (scriptName.isEmpty())
            continue;

        Entry entry;
        entry.scriptName = scriptName;
        if (!flagPart.isEmpty())
        {
            const QStringList flags = flagPart.split('|');
            for (const QString& f : flags)
            {
                const QString trimmed = f.trimmed();
                if (!trimmed.isEmpty())
                    entry.flags.append(trimmed);
            }
        }
        entries.append(entry);
    }
    return entries;
}

bool ScriptFlagFile::loadFile(const QString& path, QVector<Entry>& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG_WARNING(QString("ScriptFlagFile::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    out = parse(content);
    LOG_DEBUG(QString("ScriptFlagFile: parsed %1 flag mappings from %2")
        .arg(out.size()).arg(path));
    return true;
}

QStringList ScriptFlagFile::knownFlags()
{
    return { QStringLiteral("Hidden"),
             QStringLiteral("Conditional"),
             QStringLiteral("Default"),
             QStringLiteral("CollapsedOnRef"),
             QStringLiteral("CollapsedOnBase"),
             QStringLiteral("Mandatory") };
}

QStringList ScriptFlagFile::unknownFlags(const QVector<Entry>& entries)
{
    QStringList unknown;
    QSet<QString> known;
    for (const QString& f : knownFlags())
    {
        known.insert(f);
    }
    for (const Entry& entry : entries)
    {
        for (const QString& flag : entry.flags)
        {
            if (!known.contains(flag))
            {
                unknown.append(entry.scriptName + QStringLiteral(": ") + flag);
            }
        }
    }
    return unknown;
}

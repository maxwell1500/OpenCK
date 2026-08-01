#include "csvsnippet.hpp"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonValue>
#include <QRegularExpression>
#include <QDebug>

#include "../log/logger.hpp"

QVariantMap CsvSnippet::flattenRecord(const QJsonObject& record)
{
    QVariantMap flat;
    for (auto it = record.begin(); it != record.end(); ++it)
    {
        flat.insert(it.key(), it.value().toVariant());
    }
    return flat;
}

QVector<CsvSnippet::Row> CsvSnippet::render(const QString& snippetPath,
                                            const QJsonObject& record,
                                            int maxDepth)
{
    QVector<Row> rows;
    const QVariantMap fields = flattenRecord(record);

    QFile file(snippetPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG_WARNING(QString("CsvSnippet::render: cannot open %1").arg(snippetPath));
        return rows;
    }

    const QDir dir(QFileInfo(snippetPath).absolutePath());

    while (!file.atEnd())
    {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        if (line.startsWith(QStringLiteral(".Import=")))
        {
            const QString importPath = line.mid(QStringLiteral(".Import=").size()).trimmed();
            inlineImport(dir.absolutePath(), importPath, record, rows, maxDepth);
            continue;
        }

        Row row;
        row.text = substituteLine(line, fields);
        rows.append(row);
    }
    file.close();
    return rows;
}

bool CsvSnippet::inlineImport(const QString& snippetDir,
                              const QString& importPath,
                              const QJsonObject& record,
                              QVector<Row>& out,
                              int depth)
{
    if (depth <= 0)
    {
        LOG_WARNING(QString("CsvSnippet: import depth exceeded for %1").arg(importPath));
        return false;
    }

    QString resolved = importPath;
    if (!QFileInfo::exists(resolved))
    {
        resolved = QDir(snippetDir).filePath(importPath);
    }

    QFile file(resolved);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG_WARNING(QString("CsvSnippet::inlineImport: cannot open %1").arg(resolved));
        return false;
    }

    const QVariantMap fields = flattenRecord(record);
    const QDir dir(QFileInfo(resolved).absolutePath());

    while (!file.atEnd())
    {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        if (line.startsWith(QStringLiteral(".Import=")))
        {
            const QString nested = line.mid(QStringLiteral(".Import=").size()).trimmed();
            inlineImport(dir.absolutePath(), nested, record, out, depth - 1);
            continue;
        }

        Row row;
        row.text = substituteLine(line, fields);
        out.append(row);
    }
    file.close();
    return true;
}

QString CsvSnippet::substituteLine(const QString& line, const QVariantMap& fields)
{
    QString result = line;
    static const QRegularExpression accessorRe(QStringLiteral("<([^>]+)>"));
    QRegularExpressionMatchIterator it = accessorRe.globalMatch(line);
    while (it.hasNext())
    {
        const QRegularExpressionMatch match = it.next();
        const QString accessor = match.captured(1);
        const QVariant value = resolveAccessor(accessor, fields);
        result.replace(match.captured(0), value.toString());
    }
    return result;
}

QVariant CsvSnippet::resolveAccessor(const QString& accessor, const QVariantMap& fields)
{
    // Optional leading "<Type>:" is dropped; we look up the field directly.
    QString key = accessor;
    const int colon = key.indexOf(':');
    if (colon >= 0)
    {
        key = key.mid(colon + 1);
    }

    // Strip a trailing .Count suffix: "Field.List.Count" -> array length.
    const QString countSuffix = QStringLiteral(".Count");
    if (key.endsWith(countSuffix))
    {
        const QString baseKey = key.left(key.size() - countSuffix.size());
        const QVariant base = fields.value(baseKey);
        if (base.canConvert<QList<QVariant>>())
        {
            return base.toList().size();
        }
        if (base.type() == QVariant::List || base.type() == QVariant::StringList)
        {
            return base.toList().size();
        }
        return 0;
    }

    // Support dotted paths for nested maps, e.g. "Position.X".
    QString currentKey = key;
    const int dot = key.indexOf('.');
    if (dot >= 0)
    {
        const QString head = key.left(dot);
        const QString rest = key.mid(dot + 1);
        const QVariant headValue = fields.value(head);
        if (headValue.canConvert<QMap<QString, QVariant>>())
        {
            return headValue.toMap().value(rest);
        }
        return QVariant();
    }

    return fields.value(currentKey);
}

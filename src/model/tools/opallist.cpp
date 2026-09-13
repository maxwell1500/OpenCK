#include "opallist.hpp"

#include <QFile>

#include "../../files/log/logger.hpp"

namespace {

// Splits a CSV line, respecting double-quoted fields ("" escapes a quote).
QStringList splitCsvLine(const QString& line)
{
    QStringList fields;
    QString current;
    bool inQuotes = false;
    const QChar quote('"');

    for (int i = 0; i < line.size(); ++i)
    {
        const QChar c = line.at(i);
        if (inQuotes)
        {
            if (c == quote)
            {
                if (i + 1 < line.size() && line.at(i + 1) == quote)
                {
                    current += quote;
                    ++i;
                }
                else
                {
                    inQuotes = false;
                }
            }
            else
            {
                current += c;
            }
        }
        else if (c == quote)
        {
            inQuotes = true;
        }
        else if (c == ',')
        {
            fields.append(current.trimmed());
            current.clear();
        }
        else
        {
            current += c;
        }
    }
    fields.append(current.trimmed());
    return fields;
}

} // namespace

OpalList OpalList::parse(const QString& content)
{
    OpalList list;
    const QStringList lines = content.split('\n');

    bool firstLine = true;
    for (QString line : lines)
    {
        line = line.trimmed();
        if (line.isEmpty())
            continue;
        if (line.startsWith('#'))
            continue;

        const QStringList fields = splitCsvLine(line);
        if (firstLine)
        {
            list.headers = fields;
            firstLine = false;
            continue;
        }

        QVector<QString> row = fields.toVector();
        // Pad short rows to match the header width.
        while (row.size() < list.headers.size())
            row.append(QString());
        list.rows.append(row);
    }
    return list;
}

bool OpalList::loadFile(const QString& path, OpalList& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG_WARNING(QString("OpalList::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    out = parse(content);
    LOG_DEBUG(QString("OpalList: parsed %1 placement rows from %2")
        .arg(out.rowCount()).arg(path));
    return true;
}

QString OpalList::value(int row, const QString& columnName) const
{
    if (row < 0 || row >= rows.size())
        return QString();
    const int col = headers.indexOf(columnName);
    if (col < 0 || col >= rows[row].size())
        return QString();
    return rows[row][col];
}

namespace {

QString escapeCsvField(const QString& field)
{
    if (field.contains(',') || field.contains('"') || field.contains('\n'))
    {
        QString escaped = field;
        escaped.replace('"', QStringLiteral("\"\""));
        return QStringLiteral("\"") + escaped + QStringLiteral("\"");
    }
    return field;
}

} // namespace

QString OpalList::toCsv() const
{
    QStringList lines;

    QStringList headerFields;
    for (const QString& h : headers)
        headerFields.append(escapeCsvField(h));
    lines.append(headerFields.join(','));

    for (const QVector<QString>& row : rows)
    {
        QStringList fields;
        for (const QString& f : row)
            fields.append(escapeCsvField(f));
        lines.append(fields.join(','));
    }

    return lines.join('\n') + '\n';
}

bool OpalList::saveFile(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        LOG_WARNING(QString("OpalList::saveFile: cannot write %1").arg(path));
        return false;
    }
    file.write(toCsv().toUtf8());
    file.close();
    return true;
}

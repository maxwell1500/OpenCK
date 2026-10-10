#include "plugintextexport.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStringList>

namespace openck {

namespace {

QString recordLabel(NAME type, quint32 formId)
{
    const QString code = snapshotName(type);
    return QStringLiteral("%1:%2")
        .arg(code, QString::number(formId, 16).rightJustified(8, QChar('0')));
}

} // namespace

QJsonObject PluginTextExport::toJson(const QVector<RecordSnapshot>& records)
{
    // Group under "TYPE/EDID" so two modders adding different records land
    // in different JSON object keys — a text diff stays local instead of
    // rewriting the whole record list.
    struct Row { QString key; QJsonObject value; };
    QVector<Row> rows;

    for (const RecordSnapshot& r : records)
    {
        QJsonArray subs;
        for (const SubSnapshot& s : r.subs)
        {
            QJsonObject sub;
            sub["name"] = QString::fromLatin1(snapshotName(s.name).toUtf8());
            sub["data"] = QString::fromLatin1(s.payload.toHex());
            subs.append(sub);
        }

        QJsonObject entry;
        entry["formId"] = QString::number(r.formId, 16);
        entry["flags"] = QString::number(r.flags, 16);
        entry["subs"] = subs;
        rows.append({ recordLabel(r.type, r.formId), entry });
    }

    std::sort(rows.begin(), rows.end(),
        [](const Row& a, const Row& b) { return a.key < b.key; });

    QJsonObject out;
    for (const Row& row : rows)
        out[row.key] = row.value;
    return out;
}

bool PluginTextExport::exportJson(const QString& pluginPath, const QString& outputPath,
                                   QString* error)
{
    const QVector<RecordSnapshot> records = collectRecordSnapshots(pluginPath);
    if (records.isEmpty() && error)
        *error = QStringLiteral("No records could be read from %1").arg(pluginPath);

    QJsonObject root;
    root["plugin"] = QFileInfo(pluginPath).fileName();
    root["format"] = QStringLiteral("openck-plugin-text/1");
    root["records"] = toJson(records);

    QSaveFile out(outputPath);
    if (!out.open(QIODevice::WriteOnly))
    {
        if (error)
            *error = QStringLiteral("Cannot write %1").arg(outputPath);
        return false;
    }
    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return out.commit();
}

} // namespace openck

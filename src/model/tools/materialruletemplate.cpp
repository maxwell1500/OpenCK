#include "materialruletemplate.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

#include "../../files/log/logger.hpp"

MaterialRuleTemplate MaterialRuleTemplate::fromJson(const QJsonObject& obj)
{
    MaterialRuleTemplate tpl;
    tpl.name = obj.value(QStringLiteral("name")).toString();
    tpl.shaderModel = obj.value(QStringLiteral("shaderModel")).toString(
        obj.value(QStringLiteral("shader")).toString());
    tpl.layerCount = obj.value(QStringLiteral("layerCount")).toInt(1);

    const QJsonValue opsValue = obj.value(QStringLiteral("operations"));
    if (opsValue.isArray())
    {
        for (const QJsonValue& ov : opsValue.toArray())
        {
            if (!ov.isObject()) continue;
            const QJsonObject oo = ov.toObject();
            LayerOp op;
            op.op = oo.value(QStringLiteral("op")).toString();
            op.target = oo.value(QStringLiteral("target")).toString();
            if (!op.op.isEmpty())
                tpl.operations.append(op);
        }
    }
    return tpl;
}

bool MaterialRuleTemplate::loadFile(const QString& path, QVector<MaterialRuleTemplate>& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("MaterialRuleTemplate::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError)
    {
        LOG_WARNING(QString("MaterialRuleTemplate: JSON error: %1").arg(err.errorString()));
        return false;
    }

    QJsonArray arr;
    if (doc.isArray())
    {
        arr = doc.array();
    }
    else if (doc.isObject())
    {
        arr = doc.object().value(QStringLiteral("templates")).toArray();
    }

    for (const QJsonValue& v : arr)
    {
        if (!v.isObject()) continue;
        const MaterialRuleTemplate tpl = fromJson(v.toObject());
        if (tpl.name.isEmpty()) continue;
        out.append(tpl);
    }
    LOG_DEBUG(QString("MaterialRuleTemplate: parsed %1 templates from %2")
        .arg(out.size()).arg(path));
    return !out.isEmpty();
}

QStringList MaterialRuleTemplate::builtinNames()
{
    return { QStringLiteral("1LayerStandard"),
             QStringLiteral("2LayerStandard"),
             QStringLiteral("3LayerStandard"),
             QStringLiteral("4LayerStandard"),
             QStringLiteral("Terrain"),
             QStringLiteral("Skin"),
             QStringLiteral("Hair"),
             QStringLiteral("Eye"),
             QStringLiteral("Water"),
             QStringLiteral("Vegetation") };
}

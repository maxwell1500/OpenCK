#include "particleslodpresets.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "../../files/log/logger.hpp"

QVector<ParticleLodPresets::Category> ParticleLodPresets::parseJson(const QByteArray& json)
{
    QVector<Category> result;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError)
    {
        LOG_WARNING(QString("ParticleLodPresets: JSON error: %1").arg(err.errorString()));
        return result;
    }

    QJsonArray arr;
    if (doc.isArray())
    {
        arr = doc.array();
    }
    else if (doc.isObject())
    {
        arr = doc.object().value(QStringLiteral("presets")).toArray();
    }

    for (const QJsonValue& v : arr)
    {
        if (!v.isObject()) continue;
        const QJsonObject obj = v.toObject();
        Category cat;
        cat.name = obj.value(QStringLiteral("category")).toString();
        if (cat.name.isEmpty())
            cat.name = obj.value(QStringLiteral("Category")).toString();
        if (cat.name.isEmpty())
            continue;
        cat.nearBudget = obj.value(QStringLiteral("near")).toInt(obj.value(QStringLiteral("Near")).toInt(0));
        cat.middleBudget = obj.value(QStringLiteral("middle")).toInt(obj.value(QStringLiteral("Middle")).toInt(0));
        cat.farBudget = obj.value(QStringLiteral("far")).toInt(obj.value(QStringLiteral("Far")).toInt(0));
        result.append(cat);
    }
    return result;
}

bool ParticleLodPresets::loadFile(const QString& path, QVector<Category>& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("ParticleLodPresets::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    out = parseJson(data);
    LOG_DEBUG(QString("ParticleLodPresets: parsed %1 categories from %2")
        .arg(out.size()).arg(path));
    return true;
}

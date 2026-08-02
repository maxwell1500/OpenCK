#include "creatureattachpoints.hpp"

#include <QJsonArray>

QJsonObject CreatureAttachPoints::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("editorId"), editorId);
    obj.insert(QStringLiteral("diet"), diet);
    obj.insert(QStringLiteral("size"), size);
    obj.insert(QStringLiteral("temperament"), temperament);
    obj.insert(QStringLiteral("speed"), speed);

    QJsonArray points;
    for (const AttachPoint& p : attachPoints)
    {
        QJsonObject pj;
        pj.insert(QStringLiteral("aspect"), p.aspect);
        pj.insert(QStringLiteral("bone"), p.boneName);
        pj.insert(QStringLiteral("enabled"), p.enabled);
        points.append(pj);
    }
    obj.insert(QStringLiteral("attachPoints"), points);
    return obj;
}

CreatureAttachPoints CreatureAttachPoints::fromJson(const QJsonObject& obj)
{
    CreatureAttachPoints def;
    def.editorId = obj.value(QStringLiteral("editorId")).toString();
    def.diet = obj.value(QStringLiteral("diet")).toString();
    def.size = obj.value(QStringLiteral("size")).toString();
    def.temperament = obj.value(QStringLiteral("temperament")).toString();
    def.speed = obj.value(QStringLiteral("speed")).toString();

    const QJsonArray points = obj.value(QStringLiteral("attachPoints")).toArray();
    for (const QJsonValue& v : points)
    {
        if (!v.isObject())
            continue;
        const QJsonObject pj = v.toObject();
        AttachPoint p;
        p.aspect = pj.value(QStringLiteral("aspect")).toString();
        p.boneName = pj.value(QStringLiteral("bone")).toString();
        p.enabled = pj.value(QStringLiteral("enabled")).toBool(true);
        def.attachPoints.append(p);
    }
    return def;
}

QStringList CreatureAttachPoints::standardAspects()
{
    return {
        QStringLiteral("Attack"),
        QStringLiteral("Defense"),
        QStringLiteral("Faction"),
        QStringLiteral("Diet"),
        QStringLiteral("Size"),
        QStringLiteral("Skin"),
        QStringLiteral("Speed"),
        QStringLiteral("Temperament"),
    };
}

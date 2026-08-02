#include "planetdefinition.hpp"

#include <QJsonArray>

QJsonObject PlanetDefinition::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("editorId"), editorId);
    obj.insert(QStringLiteral("starSystem"), starSystem);
    obj.insert(QStringLiteral("dayLengthHours"), dayLengthHours);
    obj.insert(QStringLiteral("gravity"), gravity);
    obj.insert(QStringLiteral("temperature"), temperature);

    QJsonArray biomesArr;
    for (const Biome& b : biomes)
    {
        QJsonObject bj;
        bj.insert(QStringLiteral("name"), b.name);
        bj.insert(QStringLiteral("color"), b.colorHex);
        bj.insert(QStringLiteral("coverage"), b.coverage);
        biomesArr.append(bj);
    }
    obj.insert(QStringLiteral("biomes"), biomesArr);

    QJsonArray traitsArr;
    for (const QString& t : traits)
        traitsArr.append(t);
    obj.insert(QStringLiteral("traits"), traitsArr);

    QJsonArray resourcesArr;
    for (const Resource& r : resources)
    {
        QJsonObject rj;
        rj.insert(QStringLiteral("name"), r.name);
        rj.insert(QStringLiteral("count"), r.count);
        resourcesArr.append(rj);
    }
    obj.insert(QStringLiteral("resources"), resourcesArr);
    return obj;
}

PlanetDefinition PlanetDefinition::fromJson(const QJsonObject& obj)
{
    PlanetDefinition def;
    def.editorId = obj.value(QStringLiteral("editorId")).toString();
    def.starSystem = obj.value(QStringLiteral("starSystem")).toString();
    def.dayLengthHours = obj.value(QStringLiteral("dayLengthHours")).toDouble(24.0);
    def.gravity = obj.value(QStringLiteral("gravity")).toString();
    def.temperature = obj.value(QStringLiteral("temperature")).toString();

    const QJsonArray biomesArr = obj.value(QStringLiteral("biomes")).toArray();
    for (const QJsonValue& v : biomesArr)
    {
        if (!v.isObject())
            continue;
        const QJsonObject bj = v.toObject();
        Biome b;
        b.name = bj.value(QStringLiteral("name")).toString();
        b.colorHex = bj.value(QStringLiteral("color")).toString();
        b.coverage = bj.value(QStringLiteral("coverage")).toDouble(0.0);
        def.biomes.append(b);
    }

    const QJsonArray traitsArr = obj.value(QStringLiteral("traits")).toArray();
    for (const QJsonValue& v : traitsArr)
        def.traits.append(v.toString());

    const QJsonArray resourcesArr = obj.value(QStringLiteral("resources")).toArray();
    for (const QJsonValue& v : resourcesArr)
    {
        if (!v.isObject())
            continue;
        const QJsonObject rj = v.toObject();
        Resource r;
        r.name = rj.value(QStringLiteral("name")).toString();
        r.count = rj.value(QStringLiteral("count")).toInt(0);
        def.resources.append(r);
    }
    return def;
}

QStringList PlanetDefinition::commonTraits()
{
    return {
        QStringLiteral("Extreme Cold"),
        QStringLiteral("Extreme Heat"),
        QStringLiteral("Thin Atmosphere"),
        QStringLiteral("Thick Atmosphere"),
        QStringLiteral("No Atmosphere"),
        QStringLiteral("Toxic Atmosphere"),
        QStringLiteral("Corrosive Atmosphere"),
        QStringLiteral("High Gravity"),
        QStringLiteral("Low Gravity"),
        QStringLiteral("Abundant Flora"),
        QStringLiteral("Abundant Fauna"),
        QStringLiteral("Barren"),
        QStringLiteral("Frozen"),
        QStringLiteral("Volcanic"),
        QStringLiteral("Radioactive"),
        QStringLiteral("Water World"),
        QStringLiteral("Ice Cap"),
        QStringLiteral("Magnetic Field"),
    };
}

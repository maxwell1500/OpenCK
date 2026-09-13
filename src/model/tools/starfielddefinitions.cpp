#include "starfielddefinitions.hpp"

#include <QJsonArray>

// ===========================================================================
// SpaceshipDefinition
// ===========================================================================
QJsonObject SpaceshipDefinition::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("editorId"), editorId);
    obj.insert(QStringLiteral("name"), name);
    obj.insert(QStringLiteral("shipClass"), shipClass);
    obj.insert(QStringLiteral("reactorId"), reactorId);
    obj.insert(QStringLiteral("gravDriveId"), gravDriveId);
    obj.insert(QStringLiteral("shieldId"), shieldId);
    obj.insert(QStringLiteral("engineId"), engineId);
    obj.insert(QStringLiteral("engineCount"), engineCount);
    obj.insert(QStringLiteral("cargoCapacity"), cargoCapacity);
    obj.insert(QStringLiteral("crewCapacity"), crewCapacity);
    obj.insert(QStringLiteral("mass"), mass);
    obj.insert(QStringLiteral("hull"), hull);

    QJsonArray modulesArr;
    for (const Module& m : modules)
    {
        QJsonObject mj;
        mj.insert(QStringLiteral("slot"), m.slot);
        mj.insert(QStringLiteral("id"), m.id);
        mj.insert(QStringLiteral("count"), m.count);
        modulesArr.append(mj);
    }
    obj.insert(QStringLiteral("modules"), modulesArr);
    return obj;
}

SpaceshipDefinition SpaceshipDefinition::fromJson(const QJsonObject& obj)
{
    SpaceshipDefinition def;
    def.editorId = obj.value(QStringLiteral("editorId")).toString();
    def.name = obj.value(QStringLiteral("name")).toString();
    def.shipClass = obj.value(QStringLiteral("shipClass")).toString();
    def.reactorId = obj.value(QStringLiteral("reactorId")).toString();
    def.gravDriveId = obj.value(QStringLiteral("gravDriveId")).toString();
    def.shieldId = obj.value(QStringLiteral("shieldId")).toString();
    def.engineId = obj.value(QStringLiteral("engineId")).toString();
    def.engineCount = obj.value(QStringLiteral("engineCount")).toInt(1);
    def.cargoCapacity = obj.value(QStringLiteral("cargoCapacity")).toInt(0);
    def.crewCapacity = obj.value(QStringLiteral("crewCapacity")).toInt(0);
    def.mass = obj.value(QStringLiteral("mass")).toDouble(0.0);
    def.hull = obj.value(QStringLiteral("hull")).toDouble(0.0);

    const QJsonArray arr = obj.value(QStringLiteral("modules")).toArray();
    for (const QJsonValue& v : arr)
    {
        if (!v.isObject())
            continue;
        const QJsonObject mj = v.toObject();
        Module m;
        m.slot = mj.value(QStringLiteral("slot")).toString();
        m.id = mj.value(QStringLiteral("id")).toString();
        m.count = mj.value(QStringLiteral("count")).toInt(1);
        def.modules.append(m);
    }
    return def;
}

// ===========================================================================
// ReflectionProbeDefinition
// ===========================================================================
QJsonObject ReflectionProbeDefinition::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("editorId"), editorId);
    obj.insert(QStringLiteral("x"), x);
    obj.insert(QStringLiteral("y"), y);
    obj.insert(QStringLiteral("z"), z);
    obj.insert(QStringLiteral("radius"), radius);
    obj.insert(QStringLiteral("resolution"), resolution);
    obj.insert(QStringLiteral("boxProjection"), boxProjection);
    obj.insert(QStringLiteral("brightness"), brightness);
    obj.insert(QStringLiteral("shape"), shape);
    return obj;
}

ReflectionProbeDefinition ReflectionProbeDefinition::fromJson(const QJsonObject& obj)
{
    ReflectionProbeDefinition def;
    def.editorId = obj.value(QStringLiteral("editorId")).toString();
    def.x = obj.value(QStringLiteral("x")).toDouble(0.0);
    def.y = obj.value(QStringLiteral("y")).toDouble(0.0);
    def.z = obj.value(QStringLiteral("z")).toDouble(0.0);
    def.radius = obj.value(QStringLiteral("radius")).toDouble(512.0);
    def.resolution = obj.value(QStringLiteral("resolution")).toInt(128);
    def.boxProjection = obj.value(QStringLiteral("boxProjection")).toBool(false);
    def.brightness = obj.value(QStringLiteral("brightness")).toDouble(1.0);
    def.shape = obj.value(QStringLiteral("shape")).toString(QStringLiteral("Sphere"));
    return def;
}

// ===========================================================================
// CrowdRegionDefinition
// ===========================================================================
QJsonObject CrowdRegionDefinition::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("editorId"), editorId);
    obj.insert(QStringLiteral("behavior"), behavior);
    obj.insert(QStringLiteral("x"), x);
    obj.insert(QStringLiteral("y"), y);
    obj.insert(QStringLiteral("z"), z);
    obj.insert(QStringLiteral("radius"), radius);
    obj.insert(QStringLiteral("density"), density);
    obj.insert(QStringLiteral("maxMembers"), maxMembers);

    QJsonArray membersArr;
    for (const Member& m : members)
    {
        QJsonObject mj;
        mj.insert(QStringLiteral("id"), m.id);
        mj.insert(QStringLiteral("weight"), m.weight);
        membersArr.append(mj);
    }
    obj.insert(QStringLiteral("members"), membersArr);
    return obj;
}

CrowdRegionDefinition CrowdRegionDefinition::fromJson(const QJsonObject& obj)
{
    CrowdRegionDefinition def;
    def.editorId = obj.value(QStringLiteral("editorId")).toString();
    def.behavior = obj.value(QStringLiteral("behavior")).toString();
    def.x = obj.value(QStringLiteral("x")).toDouble(0.0);
    def.y = obj.value(QStringLiteral("y")).toDouble(0.0);
    def.z = obj.value(QStringLiteral("z")).toDouble(0.0);
    def.radius = obj.value(QStringLiteral("radius")).toDouble(1024.0);
    def.density = obj.value(QStringLiteral("density")).toDouble(0.1);
    def.maxMembers = obj.value(QStringLiteral("maxMembers")).toInt(20);

    const QJsonArray arr = obj.value(QStringLiteral("members")).toArray();
    for (const QJsonValue& v : arr)
    {
        if (!v.isObject())
            continue;
        const QJsonObject mj = v.toObject();
        Member m;
        m.id = mj.value(QStringLiteral("id")).toString();
        m.weight = mj.value(QStringLiteral("weight")).toDouble(1.0);
        def.members.append(m);
    }
    return def;
}

// ===========================================================================
// MorphDefinition
// ===========================================================================
QStringList MorphDefinition::commonChannels()
{
    return {
        QStringLiteral("Brow Height"),
        QStringLiteral("Brow Angle"),
        QStringLiteral("Eye Shape"),
        QStringLiteral("Eye Spacing"),
        QStringLiteral("Nose Width"),
        QStringLiteral("Nose Height"),
        QStringLiteral("Cheekbones"),
        QStringLiteral("Jaw Width"),
        QStringLiteral("Chin Shape"),
        QStringLiteral("Mouth Width"),
        QStringLiteral("Lip Fullness"),
        QStringLiteral("Ear Shape"),
    };
}

QJsonObject MorphDefinition::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("editorId"), editorId);
    obj.insert(QStringLiteral("raceId"), raceId);

    QJsonArray channelsArr;
    for (const Channel& c : channels)
    {
        QJsonObject cj;
        cj.insert(QStringLiteral("name"), c.name);
        cj.insert(QStringLiteral("value"), c.value);
        cj.insert(QStringLiteral("min"), c.minimum);
        cj.insert(QStringLiteral("max"), c.maximum);
        channelsArr.append(cj);
    }
    obj.insert(QStringLiteral("channels"), channelsArr);
    return obj;
}

MorphDefinition MorphDefinition::fromJson(const QJsonObject& obj)
{
    MorphDefinition def;
    def.editorId = obj.value(QStringLiteral("editorId")).toString();
    def.raceId = obj.value(QStringLiteral("raceId")).toString();

    const QJsonArray arr = obj.value(QStringLiteral("channels")).toArray();
    for (const QJsonValue& v : arr)
    {
        if (!v.isObject())
            continue;
        const QJsonObject cj = v.toObject();
        Channel c;
        c.name = cj.value(QStringLiteral("name")).toString();
        c.value = cj.value(QStringLiteral("value")).toDouble(0.0);
        c.minimum = cj.value(QStringLiteral("min")).toDouble(-1.0);
        c.maximum = cj.value(QStringLiteral("max")).toDouble(1.0);
        def.channels.append(c);
    }
    return def;
}

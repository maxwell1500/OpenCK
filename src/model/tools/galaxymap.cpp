#include "galaxymap.hpp"

#include <QJsonArray>

int GalaxyMap::totalPlanets() const
{
    int total = 0;
    for (const StarSystem& s : systems)
        total += s.planets.size();
    return total;
}

int GalaxyMap::findSystem(const QString& name) const
{
    for (int i = 0; i < systems.size(); ++i)
    {
        if (systems[i].name.compare(name, Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

QJsonObject GalaxyMap::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("name"), name);

    QJsonArray systemsArr;
    for (const StarSystem& s : systems)
    {
        QJsonObject sj;
        sj.insert(QStringLiteral("name"), s.name);
        sj.insert(QStringLiteral("x"), s.x);
        sj.insert(QStringLiteral("y"), s.y);

        QJsonArray planetsArr;
        for (const Planet& p : s.planets)
        {
            QJsonObject pj;
            pj.insert(QStringLiteral("name"), p.name);
            pj.insert(QStringLiteral("type"), p.type);
            pj.insert(QStringLiteral("planetEditorId"), p.planetEditorId);
            pj.insert(QStringLiteral("moons"), p.moons);
            planetsArr.append(pj);
        }
        sj.insert(QStringLiteral("planets"), planetsArr);
        systemsArr.append(sj);
    }
    obj.insert(QStringLiteral("systems"), systemsArr);
    return obj;
}

GalaxyMap GalaxyMap::fromJson(const QJsonObject& obj)
{
    GalaxyMap map;
    map.name = obj.value(QStringLiteral("name")).toString();

    const QJsonArray systemsArr = obj.value(QStringLiteral("systems")).toArray();
    for (const QJsonValue& sv : systemsArr)
    {
        if (!sv.isObject())
            continue;
        const QJsonObject sj = sv.toObject();
        StarSystem s;
        s.name = sj.value(QStringLiteral("name")).toString();
        s.x = sj.value(QStringLiteral("x")).toDouble(0.0);
        s.y = sj.value(QStringLiteral("y")).toDouble(0.0);

        const QJsonArray planetsArr = sj.value(QStringLiteral("planets")).toArray();
        for (const QJsonValue& pv : planetsArr)
        {
            if (!pv.isObject())
                continue;
            const QJsonObject pj = pv.toObject();
            Planet p;
            p.name = pj.value(QStringLiteral("name")).toString();
            p.type = pj.value(QStringLiteral("type")).toString();
            p.planetEditorId = pj.value(QStringLiteral("planetEditorId")).toString();
            p.moons = pj.value(QStringLiteral("moons")).toInt(0);
            s.planets.append(p);
        }
        map.systems.append(s);
    }
    return map;
}

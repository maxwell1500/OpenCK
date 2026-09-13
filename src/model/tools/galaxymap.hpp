#ifndef GALAXYMAP_HPP
#define GALAXYMAP_HPP

#include <QJsonObject>
#include <QString>
#include <QVector>

// Starfield galaxy view data model (REMAINING.md §3.8). A galaxy is a set of
// star systems positioned in 2D galaxy space, each holding planets that can
// reference a PlanetDefinition by editor id. Round-trips through JSON; the
// galaxy view widget renders the model without needing a real record.
struct GalaxyMap
{
    struct Planet
    {
        QString name;
        QString type;               // e.g. "Rock", "Gas Giant", "Ice"
        QString planetEditorId;     // -> PlanetDefinition.editorId
        int moons = 0;
    };

    struct StarSystem
    {
        QString name;
        double x = 0.0;             // galaxy-space position
        double y = 0.0;
        QVector<Planet> planets;
    };

    QString name;
    QVector<StarSystem> systems;

    int totalPlanets() const;
    int findSystem(const QString& name) const;   // -1 if absent

    QJsonObject toJson() const;
    static GalaxyMap fromJson(const QJsonObject& obj);
};

#endif // GALAXYMAP_HPP

#ifndef PLANETDEFINITION_H
#define PLANETDEFINITION_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>

// PlanetDefinition models the Starfield planet (PNDT) record the real CK's
// planet editor edits. Since no on-disk binary sample is available locally
// (Starfield ESM records live inside .ba2 archives), the model captures the
// documented field structure — star system, biomes, traits, day length,
// resources — and serializes it as JSON so the editor is usable now and the
// binary encoder can be added once a real record is validated.
struct PlanetDefinition
{
    QString editorId;
    QString starSystem;
    double dayLengthHours = 24.0;
    QString gravity;               // e.g. "0.5G"
    QString temperature;           // e.g. "Temperate"

    struct Biome
    {
        QString name;
        QString colorHex;          // e.g. "#a0c060"
        double coverage = 0.0;     // fraction of the planet surface
    };
    QVector<Biome> biomes;

    QStringList traits;            // e.g. "Extreme Cold", "Thin Atmosphere"

    struct Resource
    {
        QString name;
        int count = 0;
    };
    QVector<Resource> resources;

    // Serializes the definition to JSON (for the editor's save).
    QJsonObject toJson() const;

    // Loads from JSON (for tests / editor load).
    static PlanetDefinition fromJson(const QJsonObject& obj);

    // The common planet traits the editor offers.
    static QStringList commonTraits();
};

#endif // PLANETDEFINITION_H

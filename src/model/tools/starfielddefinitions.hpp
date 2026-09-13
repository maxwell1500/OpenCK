#ifndef STARFIELDDEFINITIONS_HPP
#define STARFIELDDEFINITIONS_HPP

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

// JSON-backed authoring models for the Starfield-specific feature slots
// (REMAINING.md §3.8). As with PlanetDefinition, no on-disk binary sample is
// available locally for these records, so each model captures the documented
// field structure and round-trips through JSON; a binary encoder can be added
// once a real record is validated against a shipped game file.

// ---------------------------------------------------------------------------
// Spaceship editor — a player/NPC ship made of modules and derived stats.
// ---------------------------------------------------------------------------
struct SpaceshipDefinition
{
    QString editorId;
    QString name;
    QString shipClass;             // "A", "B", "C"
    QString reactorId;
    QString gravDriveId;
    QString shieldId;
    QString engineId;
    int engineCount = 1;
    int cargoCapacity = 0;
    int crewCapacity = 0;
    double mass = 0.0;
    double hull = 0.0;

    struct Module
    {
        QString slot;              // e.g. "Reactor", "Engine", "Weapon"
        QString id;
        int count = 1;
    };
    QVector<Module> modules;

    QJsonObject toJson() const;
    static SpaceshipDefinition fromJson(const QJsonObject& obj);
};

// ---------------------------------------------------------------------------
// Reflection probe — a baked cube/box reflection capture volume.
// ---------------------------------------------------------------------------
struct ReflectionProbeDefinition
{
    QString editorId;
    double x = 0.0, y = 0.0, z = 0.0;
    double radius = 512.0;
    int resolution = 128;          // cube face size in texels
    bool boxProjection = false;
    double brightness = 1.0;
    QString shape = QStringLiteral("Sphere"); // "Sphere" | "Box"

    QJsonObject toJson() const;
    static ReflectionProbeDefinition fromJson(const QJsonObject& obj);
};

// ---------------------------------------------------------------------------
// Crowd-region authoring — a volume that spawns weighted crowd members.
// ---------------------------------------------------------------------------
struct CrowdRegionDefinition
{
    QString editorId;
    QString behavior;              // e.g. "Idle", "Wander", "Work"
    double x = 0.0, y = 0.0, z = 0.0;
    double radius = 1024.0;
    double density = 0.1;          // members per area unit
    int maxMembers = 20;

    struct Member
    {
        QString id;                // actor base / template form editor id
        double weight = 1.0;
    };
    QVector<Member> members;

    QJsonObject toJson() const;
    static CrowdRegionDefinition fromJson(const QJsonObject& obj);
};

// ---------------------------------------------------------------------------
// Morph / face-gen editor — named blend-shape channels with ranges.
// ---------------------------------------------------------------------------
struct MorphDefinition
{
    QString editorId;
    QString raceId;

    struct Channel
    {
        QString name;              // e.g. "Brow Height"
        double value = 0.0;
        double minimum = -1.0;
        double maximum = 1.0;
    };
    QVector<Channel> channels;

    // The common face-region channels the editor offers when starting fresh.
    static QStringList commonChannels();

    QJsonObject toJson() const;
    static MorphDefinition fromJson(const QJsonObject& obj);
};

#endif // STARFIELDDEFINITIONS_HPP

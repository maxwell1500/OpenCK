#ifndef MESHLODCONFIG_H
#define MESHLODCONFIG_H

#include <QString>
#include <QVector>
#include <QJsonObject>

// MeshLodConfig parses the Simplygon-style GenerationConfig.json the real
// Creation Kit uses for its -GenerateMeshLODAssociations pipeline. Each LOD
// level is defined by a screen-size threshold and reduction settings; the
// config also names the output association file and the LOD naming scheme.
// OpenCK uses these rules to drive its in-process NIF decimation.
struct MeshLodConfig
{
    struct LodLevel
    {
        int level = 1;              // 1-based LOD index
        float screenSize = 0.05f;   // coverage threshold
        float reductionPercent = 0.5f;
        int maxTriangleCount = 0;   // 0 = no limit
        bool preserveUVs = true;
        bool preserveNormals = true;
        bool generateCollision = false;
    };

    QString name;                   // pipeline name
    QString outputAssociation;      // .json association file path
    QString lodNamePattern;         // e.g. "mesh_LOD%1.nif"
    QVector<LodLevel> levels;
    bool enabled = true;

    // Loads a GenerationConfig.json file (array of LOD levels, or an object
    // with "levels"/"lodLevels" plus optional "name"/"outputAssociation").
    static bool loadFile(const QString& path, MeshLodConfig& out);

    // Parses from JSON text.
    static bool parse(const QString& json, MeshLodConfig& out);

    // Returns the LOD level for a given screen-size coverage, or nullptr if
    // no level should trigger.
    const LodLevel* levelForScreenSize(float screenSize) const;

    // Returns the default 3-level LOD pipeline.
    static MeshLodConfig builtin();
};

#endif // MESHLODCONFIG_H

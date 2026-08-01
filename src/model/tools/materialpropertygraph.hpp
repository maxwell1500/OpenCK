#ifndef MATERIALPROPERTYGRAPH_H
#define MATERIALPROPERTYGRAPH_H

#include <QString>
#include <QVector>
#include <QJsonObject>

// MaterialPropertyGraph models the BSMaterial shader property graph the real
// Creation Kit exposes in its material editor. A material is built from a
// TextureSet (named texture slots), a shader model, optional blender nodes,
// and subsurface-scattering / translucency settings. The graph is described
// by the JSON files under RuleTemplates\ShaderModels\ in a real install;
// OpenCK reads the same shape so its material editor can offer the standard
// texture slots and toggles.

// One texture slot on the material.
struct MaterialTextureSlot
{
    QString name;        // e.g. "Albedo", "Normal", "Roughness"
    QString textureKey;  // the BSM attribute (e.g. "Diffuse", "Normal")
    bool mandatory = false;
    bool optional = false;
};

// A shader-model definition (1LayerStandard, 4LayerStandard, Terrain...).
struct MaterialShaderModel
{
    QString name;
    QString displayName;
    QVector<MaterialTextureSlot> textureSlots;
    int blenderCount = 0;        // Blenders 1..5 wired in the UI
    bool hasSubsurfaceScattering = false;
    bool hasTranslucency = false;
    bool isWater = false;
    bool isSkin = false;
};

struct MaterialPropertyGraph
{
    QString name;                    // graph name
    QVector<MaterialShaderModel> models;
    QVector<MaterialTextureSlot> commonSlots;

    // The full set of texture slots the real editor lists.
    static QVector<MaterialTextureSlot> standardSlots();

    // Parses a ShaderModels JSON file (array of shader-model objects, or an
    // object with a "shaderModels"/"models" array).
    static bool loadFile(const QString& path, MaterialPropertyGraph& out);

    // Parses from JSON text.
    static bool parse(const QString& json, MaterialPropertyGraph& out);

    // Returns the shader model with the given name, or nullptr.
    const MaterialShaderModel* findModel(const QString& name) const;

    // Default graph with the standard shader models (1..4 Layer Standard,
    // Terrain, Skin, Hair, Eye, Water, Vegetation).
    static MaterialPropertyGraph builtin();
};

#endif // MATERIALPROPERTYGRAPH_H

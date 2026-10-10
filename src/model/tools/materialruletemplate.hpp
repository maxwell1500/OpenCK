#ifndef MATERIALRULETEMPLATE_HPP
#define MATERIALRULETEMPLATE_HPP

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QRegularExpression>

// Parses the material rule templates the real Creation Kit ships as JSON
// files under Data/EditorFiles/RuleTemplates/ (the ShaderModels/ and
// Bundles/ sub-directories). Each file describes one template:
//
//   {
//     "Category": "ShaderModels",
//     "MetaData": { "RootMaterial": "1LayerStandard",
//                   "DisplayName": "Standard1Layer",
//                   "ComplexityCost": 10 },
//     "Name": "1LayerStandard",
//     "TemplateRules": [
//       { "Class": "BSMaterial::LayeredMaterialID",
//         "Rules": [
//           { "From": "*", "Op": "Remove" },
//           { "From": "Layer1", "Op": "Add" },
//           { "From": "Layer1/Material/TextureSet/Emissive", "Op": "Remove" },
//           { "From": "A/...", "To": "B/...", "Op": "Move" }
//         ] }
//     ],
//     "Version": "1"
//   }
//
// Rules rewrite the material property tree (Add / Remove / Move /
// MakeConst). OpenCK simulates the texture-slot-relevant subset so the
// material editor can determine which PBR texture slots (Diffuse, Normal,
// Roughness, AO, ...) a compiled material must fill.

struct MaterialRuleTemplate
{
    // One property-tree rule. `from` is a wildcard path pattern, `to` the
    // destination of a Move, `op` one of Add | Remove | Move | MakeConst.
    struct Rule
    {
        QString from;
        QString to;
        QString op;
    };

    struct RuleClass
    {
        QString className;        // e.g. "BSMaterial::LayeredMaterialID", "null"
        QVector<Rule> rules;
    };

    QString category;             // "ShaderModels" | "Bundles"
    QString name;
    QString displayName;
    QString rootMaterial;
    int complexityCost = 0;
    QString version;
    QVector<RuleClass> ruleClasses;

    // Parses one template JSON object (the on-disk shape above; a
    // top-level "Name"/"name" plus "TemplateRules" array).
    static MaterialRuleTemplate fromJson(const QJsonObject& obj);

    // Loads one template from a JSON file. A top-level array of template
    // objects also loads (first element wins).
    static bool loadFile(const QString& path, MaterialRuleTemplate& out);

    // Scans a RuleTemplates sub-directory (ShaderModels/ or Bundles/) and
    // loads every *.json file as a template (sorted by file name).
    // Returns the number of templates loaded.
    static int loadDirectory(const QString& dir, QVector<MaterialRuleTemplate>& out);

    // The standard texture slots a LayerN group carries when a template
    // re-adds the whole layer (the CK base property set for one layer).
    static QStringList builtinLayerSlots();

    // CK wildcard pattern to an anchored QRegularExpression:
    //   "*"            -> matches any path (any depth)
    //   "*" in a segment -> matches any characters within that segment
    //   trailing "/*"   -> matches any remaining depth
    // Everything else matches literally (case-sensitive).
    static QRegularExpression patternToRegex(const QString& pattern);
    static bool patternMatches(const QString& pattern, const QString& path);

    // Texture slot names referenced by this template's rules (the
    // TextureSet/<slot> segment of From/To paths), in first-appearance
    // order, deduplicated.
    QStringList textureSlots() const;

    // Simulates the rules against a texture-slot name set, in rule order:
    //   Remove "*"        -> clears the set
    //   Add "LayerN"      -> adds every builtin layer slot
    //   Add/Remove <slot> -> inserts / deletes the slot
    //   MakeConst <slot>  -> drops the slot (property becomes a constant)
    //   Move <A> -> <B>   -> renames slot A to slot B
    struct ApplyResult
    {
        QStringList slotNames;
        QStringList operations;   // log of slot-affecting rules, in order
    };
    ApplyResult applyToSlots(const QStringList& slotNames) const;

    // Same simulation over a slot -> texture-path map (Move renames keys,
    // preserving their paths).
    struct ApplyMapResult
    {
        QMap<QString, QString> slotPaths;
        QStringList operations;
    };
    ApplyMapResult applyToSlotMap(const QMap<QString, QString>& slotMap) const;

    // The texture slots a compiled material must fill when this template
    // is applied to an empty base material.
    QStringList requiredSlots() const;

    // The built-in template names the real CK ships (fallback when no
    // RuleTemplates directory is available).
    static QStringList builtinNames();

    // A minimal fallback template (Remove * + Add Layer1) used when the
    // real RuleTemplates files are unavailable.
    static MaterialRuleTemplate builtinTemplate(const QString& name);
};

#endif // MATERIALRULETEMPLATE_HPP

#ifndef MATERIALRULETEMPLATE_HPP
#define MATERIALRULETEMPLATE_HPP

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>

// Parses BSMaterial layered material rule templates (the ShaderModels/*.json
// files). Each template describes a layered material with a name, the
// shader model, the number of layers, and the layer operations used to
// build it (Add/Remove/Move/MakeConst).
struct MaterialRuleTemplate
{
    struct LayerOp
    {
        QString op;        // Add | Remove | Move | MakeConst
        QString target;    // e.g. "Albedo", "Normal", "Layer1"
    };

    QString name;
    QString shaderModel;
    int layerCount = 1;
    QVector<LayerOp> operations;

    // Parses a template JSON object with fields name / shaderModel /
    // layerCount / operations[ { op, target } ].
    static MaterialRuleTemplate fromJson(const QJsonObject& obj);

    // Loads an array of templates from a JSON file.
    static bool loadFile(const QString& path, QVector<MaterialRuleTemplate>& out);

    // The built-in template names the real CK ships.
    static QStringList builtinNames();
};

#endif // MATERIALRULETEMPLATE_HPP

#ifndef BRUSHDEFINITION_H
#define BRUSHDEFINITION_H

#include <QString>
#include <QVector>
#include <QJsonObject>

// BrushDefinition describes a landscape brush loaded from a JSON `.lbr`
// file. The real CreationKit drives its landscape sculpt/paint system with
// JSON brush definitions plus alpha mask textures; OpenCK reimplements the
// observable behavior with its own JSON schema and wording.
struct BrushDefinition
{
    enum class Operation
    {
        Sculpt,       // raise or lower terrain with a smooth falloff
        Flatten,      // pull terrain toward a target height
        Smooth,       // average heights with neighbors
        Stamp,        // apply a height profile from an alpha mask / circle
        BuildUp,      // additive build-up inside the brush footprint
        Subtractive   // carve away terrain inside the brush footprint
    };

    static QString operationToString(Operation op);
    static Operation stringToOperation(const QString& text, bool* ok = nullptr);

    QString name;
    Operation operation = Operation::Sculpt;
    double radius = 5.0;
    double strength = 10.0;
    double falloff = 0.5;       // 0 = hard edge, 1 = very soft edge
    bool invert = false;        // for Sculpt: lower instead of raise
    double targetHeight = 0.0;  // for Flatten

    // Parses a single brush object from JSON.
    static BrushDefinition fromJson(const QJsonObject& obj);

    // Loads every brush defined in the given JSON file.
    // Returns true if the file parsed and contained at least one brush.
    static bool loadFile(const QString& path, QVector<BrushDefinition>& out);

    // Returns the built-in default brush set used when no .lbr file is
    // available (Sculpt/Flatten/Smooth/Stamp/BuildUp/Subtractive).
    static QVector<BrushDefinition> builtin();
};

#endif // BRUSHDEFINITION_H

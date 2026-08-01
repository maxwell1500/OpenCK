#ifndef BLENDTREEMODEL_H
#define BLENDTREEMODEL_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>

// BlendTreeModel implements the blend-tree editing semantics and the
// variable-assignment nodes of the animation graph editor:
//
//   - A blend tree blends between child animation nodes using per-child
//     blend weights. Weights are normalized so they sum to 1.0, and a node
//     can be pinned so its weight is fixed while the others re-normalize.
//   - Variable nodes (Assign_Variable, State_Variable_Control,
//     Dampen_Variable, Linear_Variable, Rotation_Variable) assign values to
//     named graph variables; the model tracks those variables and exposes
//     the operations each node type performs.
struct BlendTreeModel
{
    struct BlendChild
    {
        QString nodeName;       // child animation node
        double weight = 0.0;    // raw weight (normalized on use)
        bool pinned = false;    // keep this weight when others change
    };

    // One named variable and its last assigned value (string form).
    struct GraphVariable
    {
        QString name;
        QString value;
        QString assignedBy;     // last variable node that set it
    };

    // The variable-node operations.
    enum class VariableOp
    {
        Assign,          // Assign_Variable: set value directly
        StateControl,    // State_Variable_Control
        Dampen,          // Dampen_Variable: move value toward target
        Linear,          // Linear_Variable
        Rotation         // Rotation_Variable
    };

    static QString variableOpName(VariableOp op);
    static VariableOp variableOpFromName(const QString& text);

    // Normalizes the given weights in place so they sum to 1.0. Pinned
    // children keep their weight; the remainder is distributed over the
    // unpinned children proportionally. Returns the normalized weights.
    static QVector<double> normalizeWeights(const QVector<BlendChild>& children);

    // Re-weights a child: sets its weight, keeping pinned children fixed and
    // scaling the others to restore a total of 1.0.
    static void setChildWeight(QVector<BlendChild>& children, int index,
                               double weight);

    // Applies a variable-node operation to the variable set, returning true
    // when the variable changed. Dampen moves a numeric value a fraction of
    // the way to its target.
    static bool applyVariableOp(QVector<GraphVariable>& variables,
                                const QString& name, VariableOp op,
                                const QString& value);

    // Returns the current value of a variable, or empty.
    static QString variableValue(const QVector<GraphVariable>& variables,
                                 const QString& name);

    // The variable-node type names the palette offers.
    static QStringList variableNodeTypes();
};

#endif // BLENDTREEMODEL_H

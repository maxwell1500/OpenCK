#include "blendtreemodel.hpp"

#include <cmath>

QString BlendTreeModel::variableOpName(VariableOp op)
{
    switch (op)
    {
    case VariableOp::Assign: return QStringLiteral("Assign_Variable");
    case VariableOp::StateControl: return QStringLiteral("State_Variable_Control");
    case VariableOp::Dampen: return QStringLiteral("Dampen_Variable");
    case VariableOp::Linear: return QStringLiteral("Linear_Variable");
    case VariableOp::Rotation: return QStringLiteral("Rotation_Variable");
    }
    return QStringLiteral("Unknown");
}

BlendTreeModel::VariableOp BlendTreeModel::variableOpFromName(const QString& text)
{
    const QString t = text.trimmed();
    if (t == QStringLiteral("Assign_Variable")) return VariableOp::Assign;
    if (t == QStringLiteral("State_Variable_Control")) return VariableOp::StateControl;
    if (t == QStringLiteral("Dampen_Variable")) return VariableOp::Dampen;
    if (t == QStringLiteral("Linear_Variable")) return VariableOp::Linear;
    if (t == QStringLiteral("Rotation_Variable")) return VariableOp::Rotation;
    return VariableOp::Assign;
}

QStringList BlendTreeModel::variableNodeTypes()
{
    return {
        QStringLiteral("Assign_Variable"),
        QStringLiteral("State_Variable_Control"),
        QStringLiteral("Dampen_Variable"),
        QStringLiteral("Linear_Variable"),
        QStringLiteral("Rotation_Variable"),
    };
}

QVector<double> BlendTreeModel::normalizeWeights(const QVector<BlendChild>& children)
{
    QVector<double> out;
    out.reserve(children.size());
    double pinnedSum = 0.0;
    bool anyPinned = false;
    for (const BlendChild& c : children)
    {
        if (c.pinned)
        {
            pinnedSum += qMax(0.0, c.weight);
            anyPinned = true;
        }
    }

    double unpinnedSum = 0.0;
    for (const BlendChild& c : children)
        if (!c.pinned)
            unpinnedSum += qMax(0.0, c.weight);

    const double remaining = qMax(0.0, 1.0 - pinnedSum);
    const double scale = (unpinnedSum > 1e-9 && !children.isEmpty()) ? remaining / unpinnedSum : 0.0;

    for (const BlendChild& c : children)
    {
        if (c.pinned)
            out.append(qMax(0.0, c.weight));
        else
            out.append(qMax(0.0, c.weight) * scale);
    }
    Q_UNUSED(anyPinned);
    return out;
}

void BlendTreeModel::setChildWeight(QVector<BlendChild>& children, int index,
                                    double weight)
{
    if (index < 0 || index >= children.size())
        return;
    children[index].weight = qMax(0.0, weight);

    // Keep the target (and any pinned) children as-is; distribute the rest
    // of the 1.0 budget over the unpinned children proportionally.
    double fixed = children[index].weight;
    int movableCount = 0;
    double movableSum = 0.0;
    for (int i = 0; i < children.size(); ++i)
    {
        if (i == index)
            continue;
        if (children[i].pinned)
            fixed += qMax(0.0, children[i].weight);
        else
        {
            ++movableCount;
            movableSum += qMax(0.0, children[i].weight);
        }
    }

    const double remaining = qMax(0.0, 1.0 - fixed);
    const double scale = (movableSum > 1e-9) ? remaining / movableSum : 0.0;
    for (int i = 0; i < children.size(); ++i)
    {
        if (i == index || children[i].pinned)
            continue;
        children[i].weight = qMax(0.0, children[i].weight) * scale;
    }
}

bool BlendTreeModel::applyVariableOp(QVector<GraphVariable>& variables,
                                     const QString& name, VariableOp op,
                                     const QString& value)
{
    for (GraphVariable& v : variables)
    {
        if (v.name != name)
            continue;
        const QString previous = v.value;
        if (op == VariableOp::Dampen)
        {
            // Move a fraction of the way toward the target.
            const double from = previous.toDouble();
            const double to = value.toDouble();
            v.value = QString::number(from + (to - from) * 0.25);
        }
        else
        {
            v.value = value;
        }
        v.assignedBy = variableOpName(op);
        return v.value != previous;
    }

    GraphVariable v;
    v.name = name;
    v.value = value;
    v.assignedBy = variableOpName(op);
    variables.append(v);
    return true;
}

QString BlendTreeModel::variableValue(const QVector<GraphVariable>& variables,
                                      const QString& name)
{
    for (const GraphVariable& v : variables)
        if (v.name == name)
            return v.value;
    return QString();
}

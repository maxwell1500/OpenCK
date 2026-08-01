#include <QTest>

#include "../../src/model/tools/blendtreemodel.hpp"

class TestBlendTreeModel : public QObject
{
    Q_OBJECT

private slots:
    void testVariableOpNames();
    void testNormalizeWeights();
    void testNormalizeWithPinned();
    void testSetChildWeightRenormalizes();
    void testApplyVariableOp();
    void testDampen();
};

void TestBlendTreeModel::testVariableOpNames()
{
    QCOMPARE(BlendTreeModel::variableOpName(BlendTreeModel::VariableOp::Assign),
             QStringLiteral("Assign_Variable"));
    QCOMPARE(BlendTreeModel::variableOpName(BlendTreeModel::VariableOp::Rotation),
             QStringLiteral("Rotation_Variable"));
    QCOMPARE(BlendTreeModel::variableOpFromName(QStringLiteral("Dampen_Variable")),
             BlendTreeModel::VariableOp::Dampen);

    const QStringList types = BlendTreeModel::variableNodeTypes();
    QCOMPARE(types.size(), 5);
    QVERIFY(types.contains(QStringLiteral("Assign_Variable")));
    QVERIFY(types.contains(QStringLiteral("State_Variable_Control")));
    QVERIFY(types.contains(QStringLiteral("Linear_Variable")));
}

void TestBlendTreeModel::testNormalizeWeights()
{
    QVector<BlendTreeModel::BlendChild> children(2);
    children[0].nodeName = "A"; children[0].weight = 1.0;
    children[1].nodeName = "B"; children[1].weight = 1.0;

    const QVector<double> normalized = BlendTreeModel::normalizeWeights(children);
    QCOMPARE(normalized.size(), 2);
    QVERIFY(qFuzzyCompare(normalized[0] + normalized[1], 1.0));
    QVERIFY(qFuzzyCompare(normalized[0], 0.5));
}

void TestBlendTreeModel::testNormalizeWithPinned()
{
    QVector<BlendTreeModel::BlendChild> children(3);
    children[0].nodeName = "A"; children[0].weight = 0.6; children[0].pinned = true;
    children[1].nodeName = "B"; children[1].weight = 0.5;
    children[2].nodeName = "C"; children[2].weight = 0.5;

    const QVector<double> normalized = BlendTreeModel::normalizeWeights(children);
    QVERIFY(qFuzzyCompare(normalized[0], 0.6));       // pinned stays
    QVERIFY(qFuzzyCompare(normalized[1] + normalized[2], 0.4));  // rest split
    QVERIFY(qFuzzyCompare(normalized[1], 0.2));
}

void TestBlendTreeModel::testSetChildWeightRenormalizes()
{
    QVector<BlendTreeModel::BlendChild> children(2);
    children[0].nodeName = "A"; children[0].weight = 0.5;
    children[1].nodeName = "B"; children[1].weight = 0.5;

    BlendTreeModel::setChildWeight(children, 0, 0.8);
    QVERIFY(qFuzzyCompare(children[0].weight, 0.8));
    QVERIFY(qFuzzyCompare(children[1].weight, 0.2));
}

void TestBlendTreeModel::testApplyVariableOp()
{
    QVector<BlendTreeModel::GraphVariable> vars;
    QVERIFY(BlendTreeModel::applyVariableOp(
        vars, QStringLiteral("Speed"), BlendTreeModel::VariableOp::Assign,
        QStringLiteral("4.0")));
    QCOMPARE(vars.size(), 1);
    QCOMPARE(BlendTreeModel::variableValue(vars, QStringLiteral("Speed")),
             QStringLiteral("4.0"));
    QCOMPARE(vars[0].assignedBy, QStringLiteral("Assign_Variable"));

    // Overwrite an existing variable.
    QVERIFY(BlendTreeModel::applyVariableOp(
        vars, QStringLiteral("Speed"), BlendTreeModel::VariableOp::Assign,
        QStringLiteral("2.5")));
    QCOMPARE(vars.size(), 1);
    QCOMPARE(BlendTreeModel::variableValue(vars, QStringLiteral("Speed")),
             QStringLiteral("2.5"));

    // Unknown variable returns empty.
    QVERIFY(BlendTreeModel::variableValue(vars, QStringLiteral("Missing")).isEmpty());
}

void TestBlendTreeModel::testDampen()
{
    QVector<BlendTreeModel::GraphVariable> vars;
    BlendTreeModel::applyVariableOp(
        vars, QStringLiteral("Heading"), BlendTreeModel::VariableOp::Assign,
        QStringLiteral("0"));
    BlendTreeModel::applyVariableOp(
        vars, QStringLiteral("Heading"), BlendTreeModel::VariableOp::Dampen,
        QStringLiteral("100"));

    const double value = BlendTreeModel::variableValue(vars, QStringLiteral("Heading"))
                             .toDouble();
    // 0 + (100 - 0) * 0.25 = 25.
    QVERIFY(qFuzzyCompare(value, 25.0));
}

QTEST_MAIN(TestBlendTreeModel)
#include "test_blendtreemodel.moc"

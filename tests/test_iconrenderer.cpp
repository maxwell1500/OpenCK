#include <QTest>

#include "../../src/model/tools/iconrenderer.hpp"

class TestIconRenderer : public QObject
{
    Q_OBJECT

private slots:
    void testContextSizes();
    void testContextNames();
    void testDefaultRig();
    void testBlenderArguments();
};

void TestIconRenderer::testContextSizes()
{
    QCOMPARE(IconRenderer::contextSize(IconRenderer::Context::Inventory), 128);
    QCOMPARE(IconRenderer::contextSize(IconRenderer::Context::Workshop), 512);
    QCOMPARE(IconRenderer::contextSize(IconRenderer::Context::ShipBuilder), 512);
}

void TestIconRenderer::testContextNames()
{
    QCOMPARE(IconRenderer::contextName(IconRenderer::Context::Inventory),
             QStringLiteral("Inventory"));
    QCOMPARE(IconRenderer::contextName(IconRenderer::Context::Workshop),
             QStringLiteral("Workshop"));
    QCOMPARE(IconRenderer::contextName(IconRenderer::Context::ShipBuilder),
             QStringLiteral("Ship Builder"));
}

void TestIconRenderer::testDefaultRig()
{
    const QVector<IconRenderer::Light> rig = IconRenderer::defaultRig();
    QCOMPARE(rig.size(), 3);

    // Order: warm fill, cool rim, key light.
    QCOMPARE(rig[0].name, QStringLiteral("FillWarm"));
    QVERIFY(rig[0].color.red() > 200);          // warm/orange
    QVERIFY(rig[0].color.blue() < rig[0].color.red());

    QCOMPARE(rig[1].name, QStringLiteral("RimCool"));
    QVERIFY(rig[1].color.blue() > 200);         // cool/blue
    QVERIFY(rig[1].color.blue() > rig[1].color.red());

    QCOMPARE(rig[2].name, QStringLiteral("KeyLight"));
    QVERIFY(rig[2].energy >= rig[0].energy && rig[2].energy >= rig[1].energy);

    // Warm fill is front-left (negative x), rim back-right (positive x).
    QVERIFY(rig[0].x < 0 && rig[1].x > 0);
}

void TestIconRenderer::testBlenderArguments()
{
    const QString blender = QStringLiteral("C:/blender/blender.exe");
    const QStringList args = IconRenderer::blenderArguments(
        blender,
        QStringLiteral("C:/models/foo.nif"),
        QStringLiteral("C:/out/foo_inventory.png"),
        IconRenderer::Context::Inventory);

    QCOMPARE(args.size(), 8);
    QCOMPARE(args[0], blender);
    QCOMPARE(args[1], QStringLiteral("--background"));
    QCOMPARE(args[2], QStringLiteral("--python"));
    QVERIFY(args[3].endsWith(QStringLiteral("icon_generator.py")));
    QCOMPARE(args[4], QStringLiteral("--"));
    QCOMPARE(args[5], QStringLiteral("C:/models/foo.nif"));
    QCOMPARE(args[6], QStringLiteral("C:/out/foo_inventory.png"));
    QCOMPARE(args[7], QStringLiteral("128"));

    // Ship-builder context renders at 512.
    const QStringList args512 = IconRenderer::blenderArguments(
        blender, QStringLiteral("a.nif"), QStringLiteral("b.png"),
        IconRenderer::Context::ShipBuilder);
    QCOMPARE(args512.last(), QStringLiteral("512"));
}

QTEST_MAIN(TestIconRenderer)
#include "test_iconrenderer.moc"

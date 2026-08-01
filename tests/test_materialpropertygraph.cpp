#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../src/model/tools/materialpropertygraph.hpp"

class TestMaterialPropertyGraph : public QObject
{
    Q_OBJECT

private slots:
    void testStandardSlots();
    void testParseArray();
    void testParseObjectWithModels();
    void testParseSlotListString();
    void testFindModel();
    void testBuiltin();
    void testInvalid();
};

void TestMaterialPropertyGraph::testStandardSlots()
{
    const QVector<MaterialTextureSlot> standard =
        MaterialPropertyGraph::standardSlots();
    QCOMPARE(standard.size(), 10);

    QCOMPARE(standard[0].name, QStringLiteral("Albedo"));
    QCOMPARE(standard[0].textureKey, QStringLiteral("Diffuse"));
    QVERIFY(standard[0].mandatory);
    QVERIFY(!standard[0].optional);

    QCOMPARE(standard[1].name, QStringLiteral("Normal"));
    QVERIFY(standard[1].optional);

    // Frost is the last standard slot.
    QCOMPARE(standard[9].name, QStringLiteral("Frost"));
}

void TestMaterialPropertyGraph::testParseArray()
{
    const QString json = R"([
        {
            "name": "1LayerStandard",
            "displayName": "1 Layer Standard",
            "blenders": 1,
            "subsurfaceScattering": false,
            "textureSlots": [
                { "name": "Albedo", "key": "Diffuse", "mandatory": true },
                { "name": "Normal", "key": "Normal", "optional": true }
            ]
        }
    ])";

    MaterialPropertyGraph graph;
    QVERIFY(MaterialPropertyGraph::parse(json, graph));
    QCOMPARE(graph.models.size(), 1);
    QCOMPARE(graph.models[0].name, QStringLiteral("1LayerStandard"));
    QCOMPARE(graph.models[0].displayName, QStringLiteral("1 Layer Standard"));
    QCOMPARE(graph.models[0].blenderCount, 1);
    QVERIFY(!graph.models[0].hasSubsurfaceScattering);
    QCOMPARE(graph.models[0].textureSlots.size(), 2);
    QCOMPARE(graph.models[0].textureSlots[0].name, QStringLiteral("Albedo"));
    QVERIFY(graph.models[0].textureSlots[0].mandatory);
}

void TestMaterialPropertyGraph::testParseObjectWithModels()
{
    const QString json = R"({
        "name": "Graph",
        "shaderModels": [
            { "name": "Skin", "skin": true, "subsurfaceScattering": true, "translucency": true },
            { "name": "Water", "water": true }
        ]
    })";

    MaterialPropertyGraph graph;
    QVERIFY(MaterialPropertyGraph::parse(json, graph));
    QCOMPARE(graph.name, QStringLiteral("Graph"));
    QCOMPARE(graph.models.size(), 2);
    QVERIFY(graph.models[0].isSkin);
    QVERIFY(graph.models[0].hasSubsurfaceScattering);
    QVERIFY(graph.models[0].hasTranslucency);
    QVERIFY(graph.models[1].isWater);
}

void TestMaterialPropertyGraph::testParseSlotListString()
{
    const QString json = R"({
        "models": [
            { "name": "Simple", "textureSlots": "Albedo,Normal,Roughness" }
        ]
    })";

    MaterialPropertyGraph graph;
    QVERIFY(MaterialPropertyGraph::parse(json, graph));
    QCOMPARE(graph.models.size(), 1);
    QCOMPARE(graph.models[0].textureSlots.size(), 3);
    QCOMPARE(graph.models[0].textureSlots[0].name, QStringLiteral("Albedo"));
    QCOMPARE(graph.models[0].textureSlots[2].name, QStringLiteral("Roughness"));
}

void TestMaterialPropertyGraph::testFindModel()
{
    MaterialPropertyGraph graph = MaterialPropertyGraph::builtin();
    const auto* skin = graph.findModel(QStringLiteral("skin"));
    QVERIFY(skin != nullptr);
    QVERIFY(skin->isSkin);
    QVERIFY(skin->hasSubsurfaceScattering);

    QVERIFY(graph.findModel(QStringLiteral("nope")) == nullptr);
}

void TestMaterialPropertyGraph::testBuiltin()
{
    const MaterialPropertyGraph graph = MaterialPropertyGraph::builtin();
    QCOMPARE(graph.name, QStringLiteral("Default"));
    QCOMPARE(graph.models.size(), 10);
    QCOMPARE(graph.commonSlots.size(), 10);

    const auto* terrain = graph.findModel(QStringLiteral("Terrain"));
    QVERIFY(terrain != nullptr);
    QCOMPARE(terrain->blenderCount, 1);

    const auto* water = graph.findModel(QStringLiteral("Water"));
    QVERIFY(water != nullptr);
    QVERIFY(water->isWater);
}

void TestMaterialPropertyGraph::testInvalid()
{
    MaterialPropertyGraph graph;
    QVERIFY(!MaterialPropertyGraph::parse(QStringLiteral("not json"), graph));
    QVERIFY(graph.models.isEmpty());

    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    tmp.write("not json");
    const QString path = tmp.fileName();
    tmp.close();
    QVERIFY(!MaterialPropertyGraph::loadFile(path, graph));
}

QTEST_MAIN(TestMaterialPropertyGraph)
#include "test_materialpropertygraph.moc"

#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../src/model/tools/meshlodconfig.hpp"

class TestMeshLodConfig : public QObject
{
    Q_OBJECT

private slots:
    void testParseArray();
    void testParseObjectWithLevels();
    void testLevelForScreenSize();
    void testBuiltin();
    void testInvalid();
};

void TestMeshLodConfig::testParseArray()
{
    const QString json = R"([
        { "level": 1, "screenSize": 0.2, "reductionPercent": 0.3 },
        { "level": 2, "screenSize": 0.06, "reductionPercent": 0.55, "generateCollision": true },
        { "level": 3, "screenSize": 0.02, "reductionPercent": 0.8, "maxTriangles": 5000 }
    ])";

    MeshLodConfig config;
    QVERIFY(MeshLodConfig::parse(json, config));
    QCOMPARE(config.levels.size(), 3);
    QCOMPARE(config.levels[0].level, 1);
    QVERIFY(qFuzzyCompare(config.levels[0].screenSize, 0.2f));
    QVERIFY(qFuzzyCompare(config.levels[0].reductionPercent, 0.3f));
    QVERIFY(config.levels[1].generateCollision);
    QCOMPARE(config.levels[2].maxTriangleCount, 5000);
}

void TestMeshLodConfig::testParseObjectWithLevels()
{
    const QString json = R"({
        "name": "Default",
        "outputAssociation": "assoc.json",
        "lodNamePattern": "%1_LOD%2.nif",
        "enabled": false,
        "lodLevels": [
            { "level": 1, "screenSize": 0.1 },
            { "level": 2, "screenSize": 0.02 }
        ]
    })";

    MeshLodConfig config;
    QVERIFY(MeshLodConfig::parse(json, config));
    QCOMPARE(config.name, QStringLiteral("Default"));
    QCOMPARE(config.outputAssociation, QStringLiteral("assoc.json"));
    QCOMPARE(config.lodNamePattern, QStringLiteral("%1_LOD%2.nif"));
    QVERIFY(!config.enabled);
    QCOMPARE(config.levels.size(), 2);
}

void TestMeshLodConfig::testLevelForScreenSize()
{
    MeshLodConfig config = MeshLodConfig::builtin();
    QCOMPARE(config.levels.size(), 3);

    // Large on-screen coverage picks LOD 1.
    const auto* l1 = config.levelForScreenSize(0.9f);
    QVERIFY(l1 != nullptr);
    QCOMPARE(l1->level, 1);

    // Mid coverage picks LOD 2.
    const auto* l2 = config.levelForScreenSize(0.1f);
    QVERIFY(l2 != nullptr);
    QCOMPARE(l2->level, 2);

    // Tiny coverage picks LOD 3.
    const auto* l3 = config.levelForScreenSize(0.005f);
    QVERIFY(l3 != nullptr);
    QCOMPARE(l3->level, 3);
}

void TestMeshLodConfig::testBuiltin()
{
    const MeshLodConfig config = MeshLodConfig::builtin();
    QCOMPARE(config.name, QStringLiteral("Default"));
    QCOMPARE(config.levels.size(), 3);
    QVERIFY(config.enabled);
    QVERIFY(config.levels[2].generateCollision);
}

void TestMeshLodConfig::testInvalid()
{
    MeshLodConfig config;
    QVERIFY(!MeshLodConfig::parse(QStringLiteral("not json"), config));
    QVERIFY(config.levels.isEmpty());

    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    tmp.write("not json");
    const QString path = tmp.fileName();
    tmp.close();
    QVERIFY(!MeshLodConfig::loadFile(path, config));
}

QTEST_MAIN(TestMeshLodConfig)
#include "test_meshlodconfig.moc"

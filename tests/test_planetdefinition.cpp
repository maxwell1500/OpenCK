#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../../src/model/tools/planetdefinition.hpp"

class TestPlanetDefinition : public QObject
{
    Q_OBJECT

private slots:
    void testRoundTrip();
    void testDefaults();
    void testCommonTraits();
};

void TestPlanetDefinition::testRoundTrip()
{
    PlanetDefinition def;
    def.editorId = QStringLiteral("Jemison");
    def.starSystem = QStringLiteral("Alpha Centauri");
    def.dayLengthHours = 24.0;
    def.gravity = QStringLiteral("1G");
    def.temperature = QStringLiteral("Temperate");

    PlanetDefinition::Biome forest;
    forest.name = QStringLiteral("Forest");
    forest.colorHex = QStringLiteral("#3a7d44");
    forest.coverage = 0.6;
    def.biomes.append(forest);

    PlanetDefinition::Biome ocean;
    ocean.name = QStringLiteral("Ocean");
    ocean.colorHex = QStringLiteral("#1b4f8a");
    ocean.coverage = 0.4;
    def.biomes.append(ocean);

    def.traits = { QStringLiteral("Abundant Flora"), QStringLiteral("Water World") };

    PlanetDefinition::Resource iron;
    iron.name = QStringLiteral("Iron");
    iron.count = 3;
    def.resources.append(iron);

    const QJsonObject json = def.toJson();
    const PlanetDefinition loaded = PlanetDefinition::fromJson(json);

    QCOMPARE(loaded.editorId, QStringLiteral("Jemison"));
    QCOMPARE(loaded.starSystem, QStringLiteral("Alpha Centauri"));
    QCOMPARE(loaded.dayLengthHours, 24.0);
    QCOMPARE(loaded.gravity, QStringLiteral("1G"));
    QCOMPARE(loaded.biomes.size(), 2);
    QCOMPARE(loaded.biomes[0].name, QStringLiteral("Forest"));
    QCOMPARE(loaded.biomes[0].colorHex, QStringLiteral("#3a7d44"));
    QVERIFY(qFuzzyCompare(loaded.biomes[0].coverage, 0.6));
    QCOMPARE(loaded.traits.size(), 2);
    QCOMPARE(loaded.traits[1], QStringLiteral("Water World"));
    QCOMPARE(loaded.resources.size(), 1);
    QCOMPARE(loaded.resources[0].name, QStringLiteral("Iron"));
    QCOMPARE(loaded.resources[0].count, 3);
}

void TestPlanetDefinition::testDefaults()
{
    const PlanetDefinition def = PlanetDefinition::fromJson(QJsonObject());
    QCOMPARE(def.dayLengthHours, 24.0);
    QVERIFY(def.biomes.isEmpty());
    QVERIFY(def.traits.isEmpty());
    QVERIFY(def.resources.isEmpty());
}

void TestPlanetDefinition::testCommonTraits()
{
    const QStringList traits = PlanetDefinition::commonTraits();
    QVERIFY(traits.size() >= 10);
    QVERIFY(traits.contains(QStringLiteral("Extreme Cold")));
    QVERIFY(traits.contains(QStringLiteral("Water World")));
}

QTEST_MAIN(TestPlanetDefinition)
#include "test_planetdefinition.moc"

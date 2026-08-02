#include <QTest>
#include <QJsonObject>
#include <QJsonArray>

#include "../../src/model/tools/creatureattachpoints.hpp"

class TestCreatureAttachPoints : public QObject
{
    Q_OBJECT

private slots:
    void testRoundTrip();
    void testStandardAspects();
    void testDefaults();
};

void TestCreatureAttachPoints::testRoundTrip()
{
    CreatureAttachPoints def;
    def.editorId = QStringLiteral("CaveCrawler");
    def.diet = QStringLiteral("Carnivore");
    def.size = QStringLiteral("Medium");
    def.temperament = QStringLiteral("Aggressive");
    def.speed = QStringLiteral("Fast");

    CreatureAttachPoints::AttachPoint attack;
    attack.aspect = QStringLiteral("Attack");
    attack.boneName = QStringLiteral("ap_CCT_Attack");
    attack.enabled = true;
    def.attachPoints.append(attack);

    CreatureAttachPoints::AttachPoint skin;
    skin.aspect = QStringLiteral("Skin");
    skin.boneName = QStringLiteral("ap_CCT_Skin");
    skin.enabled = false;
    def.attachPoints.append(skin);

    const QJsonObject json = def.toJson();
    const CreatureAttachPoints loaded = CreatureAttachPoints::fromJson(json);

    QCOMPARE(loaded.editorId, QStringLiteral("CaveCrawler"));
    QCOMPARE(loaded.diet, QStringLiteral("Carnivore"));
    QCOMPARE(loaded.size, QStringLiteral("Medium"));
    QCOMPARE(loaded.temperament, QStringLiteral("Aggressive"));
    QCOMPARE(loaded.speed, QStringLiteral("Fast"));
    QCOMPARE(loaded.attachPoints.size(), 2);
    QCOMPARE(loaded.attachPoints[0].aspect, QStringLiteral("Attack"));
    QCOMPARE(loaded.attachPoints[0].boneName, QStringLiteral("ap_CCT_Attack"));
    QVERIFY(loaded.attachPoints[0].enabled);
    QCOMPARE(loaded.attachPoints[1].aspect, QStringLiteral("Skin"));
    QVERIFY(!loaded.attachPoints[1].enabled);
}

void TestCreatureAttachPoints::testStandardAspects()
{
    const QStringList aspects = CreatureAttachPoints::standardAspects();
    QCOMPARE(aspects.size(), 8);
    QVERIFY(aspects.contains(QStringLiteral("Attack")));
    QVERIFY(aspects.contains(QStringLiteral("Defense")));
    QVERIFY(aspects.contains(QStringLiteral("Faction")));
    QVERIFY(aspects.contains(QStringLiteral("Diet")));
    QVERIFY(aspects.contains(QStringLiteral("Size")));
    QVERIFY(aspects.contains(QStringLiteral("Skin")));
    QVERIFY(aspects.contains(QStringLiteral("Speed")));
    QVERIFY(aspects.contains(QStringLiteral("Temperament")));
}

void TestCreatureAttachPoints::testDefaults()
{
    const CreatureAttachPoints def = CreatureAttachPoints::fromJson(QJsonObject());
    QVERIFY(def.attachPoints.isEmpty());
    QVERIFY(def.diet.isEmpty());
    QVERIFY(def.speed.isEmpty());
}

QTEST_MAIN(TestCreatureAttachPoints)
#include "test_creatureattachpoints.moc"

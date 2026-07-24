#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDataStream>
#include <QByteArray>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/tes4.hpp"
#include "../../libs/files/esm/common.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/esm/weaprecord.hpp"
#include "../../libs/files/esm/armorrecord.hpp"
#include "../../libs/files/esm/spellrecord.hpp"
#include "../../libs/files/esm/questrecord.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/record.hpp"

class TestDataModel : public QObject
{
    Q_OBJECT

private slots:
    void testNpcRecord_DefaultValues();
    void testNpcRecord_SetProperties();
    void testWeaponRecord_DefaultValues();
    void testWeaponRecord_SetProperties();
    void testArmorRecord_DefaultValues();
    void testArmorRecord_SetProperties();
    void testSpellRecord_DefaultValues();
    void testQuestRecord_DefaultValues();
    void testCollection_AppendRecord();
    void testCollection_SearchById();
    void testCollection_RemoveRecord();
    void testRecord_State_Transitions();
};

void TestDataModel::testNpcRecord_DefaultValues()
{
    NpcRecord npc;
    
    // Verify default values
    QVERIFY(npc.editorId.isEmpty());
    QVERIFY(npc.fullName.isEmpty());
    QCOMPARE(npc.formId, 0u);
    QCOMPARE(npc.level, 0);
    QCOMPARE(npc.race, 0);
    QCOMPARE(npc.faction, 0);
}

void TestDataModel::testNpcRecord_SetProperties()
{
    NpcRecord npc;
    
    npc.editorId = "TestNPC";
    npc.fullName = "Test Character";
    npc.formId = 0x00012345;
    npc.level = 25;
    npc.race = 7;
    npc.faction = 3;
    
    QCOMPARE(npc.editorId, QString("TestNPC"));
    QCOMPARE(npc.fullName, QString("Test Character"));
    QCOMPARE(npc.formId, 0x00012345u);
    QCOMPARE(npc.level, 25);
    QCOMPARE(npc.race, 7);
    QCOMPARE(npc.faction, 3);
}

void TestDataModel::testWeaponRecord_DefaultValues()
{
    WeaponRecord weapon;
    
    QVERIFY(weapon.editorId.isEmpty());
    QCOMPARE(weapon.formId, 0u);
    QCOMPARE(weapon.damage, 0.0f);
    QCOMPARE(weapon.speed, 0.0f);
    QCOMPARE(weapon.weight, 0.0f);
    QCOMPARE(weapon.value, 0);
}

void TestDataModel::testWeaponRecord_SetProperties()
{
    WeaponRecord weapon;
    
    weapon.editorId = "TestSword";
    weapon.formId = 0x0001ABCD;
    weapon.damage = 15.5f;
    weapon.speed = 1.2f;
    weapon.weight = 20.0f;
    weapon.value = 150;
    
    QCOMPARE(weapon.editorId, QString("TestSword"));
    QCOMPARE(weapon.formId, 0x0001ABCDu);
    QVERIFY(qFuzzyCompare(weapon.damage, 15.5f));
    QVERIFY(qFuzzyCompare(weapon.speed, 1.2f));
    QVERIFY(qFuzzyCompare(weapon.weight, 20.0f));
    QCOMPARE(weapon.value, 150);
}

void TestDataModel::testArmorRecord_DefaultValues()
{
    ArmorRecord armor;
    
    QVERIFY(armor.editorId.isEmpty());
    QCOMPARE(armor.formId, 0u);
    QCOMPARE(armor.armorRating, 0);
    QCOMPARE(armor.weight, 0.0f);
    QCOMPARE(armor.value, 0);
}

void TestDataModel::testArmorRecord_SetProperties()
{
    ArmorRecord armor;
    
    armor.editorId = "TestHelmet";
    armor.formId = 0x0002FFFF;
    armor.armorRating = 45;
    armor.weight = 12.5f;
    armor.value = 300;
    
    QCOMPARE(armor.editorId, QString("TestHelmet"));
    QCOMPARE(armor.formId, 0x0002FFFFu);
    QCOMPARE(armor.armorRating, 45);
    QVERIFY(qFuzzyCompare(armor.weight, 12.5f));
    QCOMPARE(armor.value, 300);
}

void TestDataModel::testSpellRecord_DefaultValues()
{
    SpellRecord spell;
    
    QVERIFY(spell.editorId.isEmpty());
    QCOMPARE(spell.formId, 0u);
    QCOMPARE(spell.cost, 0);
}

void TestDataModel::testQuestRecord_DefaultValues()
{
    QuestRecord quest;
    
    QVERIFY(quest.editorId.isEmpty());
    QCOMPARE(quest.formId, 0u);
    QVERIFY(quest.questName.isEmpty());
    QCOMPARE(quest.questType, 0);
}

void TestDataModel::testCollection_AppendRecord()
{
    IdCollection<NpcRecord> collection;
    
    NpcRecord npc1;
    npc1.editorId = "NPC_One";
    npc1.formId = 0x00012345;
    
    NpcRecord npc2;
    npc2.editorId = "NPC_Two";
    npc2.formId = 0x00012346;
    
    collection.add(npc1);
    QCOMPARE(collection.size(), 1);
    
    collection.add(npc2);
    QCOMPARE(collection.size(), 2);
    
    QCOMPARE(collection.getId(0), QString("NPC_One"));
    QCOMPARE(collection.getId(1), QString("NPC_Two"));
}

void TestDataModel::testCollection_SearchById()
{
    IdCollection<NpcRecord> collection;
    
    NpcRecord npc1;
    npc1.editorId = "dragon";
    npc1.formId = 0x00012345;
    
    NpcRecord npc2;
    npc2.editorId = "dragonknight";
    npc2.formId = 0x00012346;
    
    NpcRecord npc3;
    npc3.editorId = "merchant";
    npc3.formId = 0x00012347;
    
    collection.add(npc1);
    collection.add(npc2);
    collection.add(npc3);
    
    QCOMPARE(collection.searchId("dragon"), 0);
    QCOMPARE(collection.searchId("dragonknight"), 1);
    QCOMPARE(collection.searchId("merchant"), 2);
    QCOMPARE(collection.searchId("nonexistent"), -1);
}

void TestDataModel::testCollection_RemoveRecord()
{
    IdCollection<NpcRecord> collection;
    
    for (int i = 0; i < 5; i++)
    {
        NpcRecord npc;
        npc.editorId = QString("NPC_%1").arg(i);
        npc.formId = 0x00010000 + i;
        collection.add(npc);
    }
    
    QCOMPARE(collection.size(), 5);
    
    // Remove middle record
    collection.removeRows(2, 1);
    QCOMPARE(collection.size(), 4);
    
    // Verify remaining records
    QCOMPARE(collection.getId(0), QString("NPC_0"));
    QCOMPARE(collection.getId(1), QString("NPC_1"));
    QCOMPARE(collection.getId(2), QString("NPC_3"));
    QCOMPARE(collection.getId(3), QString("NPC_4"));
}

void TestDataModel::testRecord_State_Transitions()
{
    Record<NpcRecord> record;
    
    // Default state should be State_Base
    QCOMPARE(record.state, State_Base);
    
    // Test state transitions
    NpcRecord npc;
    npc.editorId = "TestNPC";
    
    record.setModified(npc);
    QCOMPARE(record.state, State_Modified);
    
    // Test merge
    record.merge();
    QCOMPARE(record.state, State_Base);
}

QTEST_MAIN(TestDataModel)
#include "test_datamodel.moc"

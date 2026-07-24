#include <QtTest>

#include "../../src/model/tools/columnvalidator.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/esm/weaprecord.hpp"
#include "../../libs/files/esm/spellrecord.hpp"
#include "../../libs/files/esm/armorrecord.hpp"
#include "../../libs/files/esm/questrecord.hpp"

class TestColumnValidator : public QObject
{
    Q_OBJECT

private slots:
    void testValidateNpc_EmptyEditorId();
    void testValidateNpc_ValidRecord();
    void testValidateNpc_LevelOutOfRange();
    void testValidateWeapon_ValidRecord();
    void testValidateWeapon_DamageOutOfRange();
    void testValidateSpell_EmptyEditorId();
    void testValidateArmor_EmptyEditorId();
    void testValidateQuest_ValidRecord();
};

void TestColumnValidator::testValidateNpc_EmptyEditorId()
{
    NpcRecord npc;
    npc.editorId = "";
    npc.level = 10;

    auto results = ColumnValidator::validateNpc(npc, nullptr);

    bool hasEditorIdError = false;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error && r.field == "EditorID")
        {
            hasEditorIdError = true;
            break;
        }
    }
    QVERIFY(hasEditorIdError);
}

void TestColumnValidator::testValidateNpc_ValidRecord()
{
    NpcRecord npc;
    npc.editorId = "TestNPC";
    npc.level = 10;
    npc.health = 100;
    npc.magicka = 50;
    npc.stamina = 80;

    auto results = ColumnValidator::validateNpc(npc, nullptr);

    bool hasError = false;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error)
        {
            hasError = true;
            break;
        }
    }
    QVERIFY(!hasError);
}

void TestColumnValidator::testValidateNpc_LevelOutOfRange()
{
    NpcRecord npc;
    npc.editorId = "TestNPC";
    npc.level = 65536;

    auto results = ColumnValidator::validateNpc(npc, nullptr);

    bool hasLevelError = false;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error && r.field == "Level")
        {
            hasLevelError = true;
            break;
        }
    }
    QVERIFY(hasLevelError);
}

void TestColumnValidator::testValidateWeapon_ValidRecord()
{
    WeaponRecord weapon;
    weapon.editorId = "TestWeapon";
    weapon.damage = 25.5f;
    weapon.speed = 1.2f;
    weapon.reach = 1.0f;
    weapon.weight = 15.0f;
    weapon.value = 100;

    auto results = ColumnValidator::validateWeapon(weapon, nullptr);

    bool hasError = false;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error)
        {
            hasError = true;
            break;
        }
    }
    QVERIFY(!hasError);
}

void TestColumnValidator::testValidateWeapon_DamageOutOfRange()
{
    WeaponRecord weapon;
    weapon.editorId = "TestWeapon";
    weapon.damage = 100000.0f;

    auto results = ColumnValidator::validateWeapon(weapon, nullptr);

    bool hasDamageError = false;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error && r.field == "Damage")
        {
            hasDamageError = true;
            break;
        }
    }
    QVERIFY(hasDamageError);
}

void TestColumnValidator::testValidateSpell_EmptyEditorId()
{
    SpellRecord spell;
    spell.editorId = "";

    auto results = ColumnValidator::validateSpell(spell, nullptr);

    bool hasEditorIdError = false;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error && r.field == "EditorID")
        {
            hasEditorIdError = true;
            break;
        }
    }
    QVERIFY(hasEditorIdError);
}

void TestColumnValidator::testValidateArmor_EmptyEditorId()
{
    ArmorRecord armor;
    armor.editorId = "";

    auto results = ColumnValidator::validateArmor(armor, nullptr);

    bool hasEditorIdError = false;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error && r.field == "EditorID")
        {
            hasEditorIdError = true;
            break;
        }
    }
    QVERIFY(hasEditorIdError);
}

void TestColumnValidator::testValidateQuest_ValidRecord()
{
    QuestRecord quest;
    quest.editorId = "TestQuest";
    quest.questName = "Test Quest";

    auto results = ColumnValidator::validateQuest(quest, nullptr);

    bool hasError = false;
    for (const auto& r : results)
    {
        if (r.severity == ColumnValidator::Severity::Error)
        {
            hasError = true;
            break;
        }
    }
    QVERIFY(!hasError);
}

QTEST_MAIN(TestColumnValidator)
#include "test_columnvalidator.moc"

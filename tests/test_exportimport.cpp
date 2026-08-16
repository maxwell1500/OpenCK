#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../../src/model/world/record.hpp"
#include "../../libs/files/data/dataexporter.hpp"
#include "../../libs/files/data/dataimporter.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/esm/weaprecord.hpp"
#include "../../libs/files/esm/armorrecord.hpp"
#include "../../libs/files/esm/spellrecord.hpp"
#include "../../libs/files/esm/questrecord.hpp"
#include "../../libs/files/esm/cellrecord.hpp"
#include "../../libs/files/esm/Actirecord.hpp"
#include "../../libs/files/esm/Bookrecord.hpp"
#include "../../libs/files/esm/landrecord.hpp"
#include "../../libs/files/esm/statrecord.hpp"
#include "../../libs/files/esm/treerecord.hpp"
#include "../../libs/files/esm/sounrecord.hpp"
#include "../../libs/files/esm/wthrrecord.hpp"
#include "../../libs/files/esm/ltexrecord.hpp"

class TestExportImport : public QObject
{
    Q_OBJECT

private slots:
    void testJsonRoundTrip_NPC();
    void testJsonRoundTrip_Weapon();
    void testJsonRoundTrip_Armor();
    void testJsonRoundTrip_Spell();
    void testJsonRoundTrip_Quest();
    void testJsonRoundTrip_Cell();
    void testJsonRoundTrip_AllTypes();
    void testXmlRoundTrip_NPC();
    void testCsvRoundTrip_NPC();
    void testJsonRoundTrip_Land();
    void testJsonRoundTrip_Stat();
    void testJsonRoundTrip_Tree();
    void testJsonRoundTrip_Sound();
    void testJsonRoundTrip_Weather();
    void testJsonRoundTrip_LandTexture();
};

void TestExportImport::testJsonRoundTrip_NPC()
{
    NpcRecord npc;
    npc.editorId = "TestNPC";
    npc.fullName = "Test Character";
    npc.level = 25;
    npc.health = 100;
    npc.magicka = 80;
    npc.stamina = 120;
    npc.attack = 15;
    npc.defense = 10;
    npc.personality = 50;
    npc.intelligence = 60;
    npc.willpower = 45;
    npc.agility = 55;
    npc.luck = 70;
    npc.disposition = 75;
    npc.reputation = 10;
    npc.race = 7;
    npc.sex = 1;
    npc.class_ = 3;
    npc.faction = 5;
    npc.formId = 0x00012345;
    npc.flags = 0;
    npc.spells = {100, 200, 300};
    npc.inventoryItems = {1000, 2000};
    npc.relationships = {500};

    Record<NpcRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = npc;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Npc_);
    QVERIFY(!json.isEmpty());
    QCOMPARE(json["editorId"].toString(), QString("TestNPC"));

    NpcRecord imported;
    QVERIFY(DataImporter::importNpcRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestNPC"));
    QCOMPARE(imported.fullName, QString("Test Character"));
    QCOMPARE(imported.level, 25u);
    QCOMPARE(imported.health, 100u);
    QCOMPARE(imported.magicka, 80u);
    QCOMPARE(imported.stamina, 120u);
    QCOMPARE(imported.attack, 15u);
    QCOMPARE(imported.defense, 10u);
    QCOMPARE(imported.personality, 50u);
    QCOMPARE(imported.intelligence, 60u);
    QCOMPARE(imported.willpower, 45u);
    QCOMPARE(imported.agility, 55u);
    QCOMPARE(imported.luck, 70u);
    QCOMPARE(imported.disposition, 75u);
    QCOMPARE(imported.reputation, 10u);
    QCOMPARE(imported.formId, 0x00012345u);
    QCOMPARE(imported.race, 7u);
    QCOMPARE(imported.sex, 1u);
    QCOMPARE(imported.class_, 3u);
    QCOMPARE(imported.faction, 5u);
    QCOMPARE(imported.spells, QVector<quint32>({100, 200, 300}));
    QCOMPARE(imported.inventoryItems, QVector<quint32>({1000, 2000}));
    QCOMPARE(imported.relationships, QVector<quint32>({500}));
}

void TestExportImport::testJsonRoundTrip_Weapon()
{
    WeaponRecord weapon;
    weapon.editorId = "TestSword";
    weapon.fullName = "Steel Sword";
    weapon.formId = 0x0002ABCD;
    weapon.flags = 0;
    weapon.weaponType = 1;
    weapon.damage = 15.5f;
    weapon.speed = 1.2f;
    weapon.reach = 1.0f;
    weapon.weight = 12.0f;
    weapon.value = 200;
    weapon.enchantment = 0;
    weapon.iconPath = "icons/sword.dds";
    weapon.modelPath = "meshes/sword.nif";
    weapon.magicSchool = 0;
    weapon.enchantLimit = 50;

    Record<WeaponRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = weapon;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Weap_);
    QVERIFY(!json.isEmpty());

    WeaponRecord imported;
    QVERIFY(DataImporter::importWeaponRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestSword"));
    QCOMPARE(imported.fullName, QString("Steel Sword"));
    QCOMPARE(imported.formId, 0x0002ABCDu);
    QCOMPARE(imported.weaponType, 1u);
    QVERIFY(qFuzzyCompare(imported.damage, 15.5f));
    QVERIFY(qFuzzyCompare(imported.speed, 1.2f));
    QVERIFY(qFuzzyCompare(imported.reach, 1.0f));
    QVERIFY(qFuzzyCompare(imported.weight, 12.0f));
    QCOMPARE(imported.value, 200u);
    QCOMPARE(imported.iconPath, QString("icons/sword.dds"));
    QCOMPARE(imported.modelPath, QString("meshes/sword.nif"));
    QCOMPARE(imported.enchantLimit, 50u);
}

void TestExportImport::testJsonRoundTrip_Armor()
{
    ArmorRecord armor;
    armor.editorId = "TestArmor";
    armor.fullName = "Iron Shield";
    armor.formId = 0x0003EF01;
    armor.flags = 0;
    armor.armorRating = 45;
    armor.weight = 12.5f;
    armor.value = 300;
    armor.iconPath = "icons/shield.dds";
    armor.modelPath = "meshes/shield.nif";
    armor.health = 200.0f;

    Record<ArmorRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = armor;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Armor_);
    QVERIFY(!json.isEmpty());

    ArmorRecord imported;
    QVERIFY(DataImporter::importArmorRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestArmor"));
    QCOMPARE(imported.fullName, QString("Iron Shield"));
    QCOMPARE(imported.formId, 0x0003EF01u);
    QCOMPARE(imported.armorRating, 45u);
    QVERIFY(qFuzzyCompare(imported.weight, 12.5f));
    QCOMPARE(imported.value, 300u);
    QCOMPARE(imported.iconPath, QString("icons/shield.dds"));
    QCOMPARE(imported.modelPath, QString("meshes/shield.nif"));
    QVERIFY(qFuzzyCompare(imported.health, 200.0f));
}

void TestExportImport::testJsonRoundTrip_Spell()
{
    SpellRecord spell;
    spell.editorId = "TestSpell";
    spell.fullName = "Fireball";
    spell.formId = 0x00045678;
    spell.flags = 0;
    spell.cost = 50;
    spell.castingSound = 1;
    spell.effects = {1000, 2000, 3000};
    spell.enchantment = 0;

    Record<SpellRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = spell;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Spel_);
    QVERIFY(!json.isEmpty());

    SpellRecord imported;
    QVERIFY(DataImporter::importSpellRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestSpell"));
    QCOMPARE(imported.fullName, QString("Fireball"));
    QCOMPARE(imported.formId, 0x00045678u);
    QCOMPARE(imported.cost, 50u);
    QCOMPARE(imported.castingSound, 1u);
    QCOMPARE(imported.effects, QVector<quint32>({1000, 2000, 3000}));
}

void TestExportImport::testJsonRoundTrip_Quest()
{
    QuestRecord quest;
    quest.editorId = "TestQuest";
    quest.formId = 0x00059ABC;
    quest.flags = 0;
    quest.questName = "Main Quest";
    quest.questDesc = "Save the world";
    quest.questType = 1;
    quest.stageIds = {10, 20, 30};
    quest.stageDescriptions = {"Start", "Middle", "End"};
    quest.objectiveIds = {100, 200};
    quest.aliasIds = {50};
    quest.dialogueView = "default";
    quest.scriptIds = {1000};

    Record<QuestRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = quest;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Quest_);
    QVERIFY(!json.isEmpty());

    QuestRecord imported;
    QVERIFY(DataImporter::importQuestRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestQuest"));
    QCOMPARE(imported.formId, 0x00059ABCu);
    QCOMPARE(imported.questName, QString("Main Quest"));
    QCOMPARE(imported.questDesc, QString("Save the world"));
    QCOMPARE(imported.questType, 1u);
    QCOMPARE(imported.stageIds, QVector<quint32>({10, 20, 30}));
    QCOMPARE(imported.stageDescriptions, QVector<QString>({"Start", "Middle", "End"}));
    QCOMPARE(imported.objectiveIds, QVector<quint32>({100, 200}));
    QCOMPARE(imported.aliasIds, QVector<quint32>({50}));
    QCOMPARE(imported.dialogueView, QString("default"));
    QCOMPARE(imported.scriptIds, QVector<quint32>({1000}));
}

void TestExportImport::testJsonRoundTrip_Cell()
{
    CellRecord cell;
    cell.editorId = "TestCell";
    cell.formId = 0x0006DEF0;
    cell.flags = 0;
    cell.cellX = 5;
    cell.cellY = -3;
    cell.owner = 0;
    cell.lockLevel = 100;
    cell.cellName = "Test Cell Name";

    Record<CellRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = cell;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Cel_);
    QVERIFY(!json.isEmpty());

    CellRecord imported;
    QVERIFY(DataImporter::importCellRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestCell"));
    QCOMPARE(imported.formId, 0x0006DEF0u);
    QCOMPARE(imported.cellX, 5);
    QCOMPARE(imported.cellY, -3);
    QCOMPARE(imported.owner, 0u);
    QCOMPARE(imported.lockLevel, 100u);
    QCOMPARE(imported.cellName, QString("Test Cell Name"));
}

void TestExportImport::testJsonRoundTrip_AllTypes()
{
    QJsonObject result;

    {
        NpcRecord npc;
        npc.editorId = "MultiNPC";
        npc.formId = 0x000A0001;
        npc.fullName = "NPC One";
        npc.level = 10;
        Record<NpcRecord> rec;
        rec.state = State_ModifiedOnly;
        rec.modifiedRecord = npc;
        result["npc"] = DataExporter::recordToJSON(rec, CkId::Type_Npc_);
    }
    {
        WeaponRecord weapon;
        weapon.editorId = "MultiWeap";
        weapon.formId = 0x000A0002;
        weapon.fullName = "Weapon One";
        weapon.damage = 5.0f;
        Record<WeaponRecord> rec;
        rec.state = State_ModifiedOnly;
        rec.modifiedRecord = weapon;
        result["weapon"] = DataExporter::recordToJSON(rec, CkId::Type_Weap_);
    }
    {
        ArmorRecord armor;
        armor.editorId = "MultiArmo";
        armor.formId = 0x000A0003;
        armor.fullName = "Armor One";
        armor.armorRating = 20;
        Record<ArmorRecord> rec;
        rec.state = State_ModifiedOnly;
        rec.modifiedRecord = armor;
        result["armor"] = DataExporter::recordToJSON(rec, CkId::Type_Armor_);
    }
    {
        SpellRecord spell;
        spell.editorId = "MultiSpel";
        spell.formId = 0x000A0004;
        spell.fullName = "Spell One";
        spell.cost = 25;
        Record<SpellRecord> rec;
        rec.state = State_ModifiedOnly;
        rec.modifiedRecord = spell;
        result["spell"] = DataExporter::recordToJSON(rec, CkId::Type_Spel_);
    }
    {
        QuestRecord quest;
        quest.editorId = "MultiQuest";
        quest.formId = 0x000A0005;
        quest.questName = "Quest One";
        Record<QuestRecord> rec;
        rec.state = State_ModifiedOnly;
        rec.modifiedRecord = quest;
        result["quest"] = DataExporter::recordToJSON(rec, CkId::Type_Quest_);
    }
    {
        ActiRecord acti;
        acti.editorId = "MultiActi";
        acti.formId = 0x000A0006;
        Record<ActiRecord> rec;
        rec.state = State_ModifiedOnly;
        rec.modifiedRecord = acti;
        result["acti"] = DataExporter::recordToJSON(rec, CkId::Type_Acti_);
    }
    {
        BookRecord book;
        book.editorId = "MultiBook";
        book.formId = 0x000A0007;
        book.pageCount = 10;
        Record<BookRecord> rec;
        rec.state = State_ModifiedOnly;
        rec.modifiedRecord = book;
        result["book"] = DataExporter::recordToJSON(rec, CkId::Type_Book_);
    }

    NpcRecord npcImported;
    QVERIFY(DataImporter::importNpcRecord(result["npc"].toObject(), npcImported));
    QCOMPARE(npcImported.editorId, QString("MultiNPC"));
    QCOMPARE(npcImported.formId, 0x000A0001u);

    WeaponRecord weapImported;
    QVERIFY(DataImporter::importWeaponRecord(result["weapon"].toObject(), weapImported));
    QCOMPARE(weapImported.editorId, QString("MultiWeap"));
    QCOMPARE(weapImported.formId, 0x000A0002u);

    ArmorRecord armorImported;
    QVERIFY(DataImporter::importArmorRecord(result["armor"].toObject(), armorImported));
    QCOMPARE(armorImported.editorId, QString("MultiArmo"));
    QCOMPARE(armorImported.formId, 0x000A0003u);

    SpellRecord spelImported;
    QVERIFY(DataImporter::importSpellRecord(result["spell"].toObject(), spelImported));
    QCOMPARE(spelImported.editorId, QString("MultiSpel"));
    QCOMPARE(spelImported.formId, 0x000A0004u);

    QuestRecord questImported;
    QVERIFY(DataImporter::importQuestRecord(result["quest"].toObject(), questImported));
    QCOMPARE(questImported.editorId, QString("MultiQuest"));
    QCOMPARE(questImported.formId, 0x000A0005u);

    ActiRecord actiImported;
    QVERIFY(DataImporter::importActiRecord(result["acti"].toObject(), actiImported));
    QCOMPARE(actiImported.editorId, QString("MultiActi"));
    QCOMPARE(actiImported.formId, 0x000A0006u);

    BookRecord bookImported;
    QVERIFY(DataImporter::importBookRecord(result["book"].toObject(), bookImported));
    QCOMPARE(bookImported.editorId, QString("MultiBook"));
    QCOMPARE(bookImported.formId, 0x000A0007u);
}

void TestExportImport::testXmlRoundTrip_NPC()
{
    NpcRecord npc;
    npc.editorId = "XmlNPC";
    npc.fullName = "XML Test";
    npc.level = 42;
    npc.health = 200;
    npc.race = 3;
    npc.sex = 0;
    npc.class_ = 2;
    npc.formId = 0x000B1234;

    Record<NpcRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = npc;

    QDomDocument doc;
    QDomElement root = doc.createElement("OpenCKExport");
    doc.appendChild(root);

    QDomElement recordElem = DataExporter::recordToXML(rec, CkId::Type_Npc_, doc);
    root.appendChild(recordElem);

    QCOMPARE(recordElem.attribute("editorId"), QString("XmlNPC"));
    QCOMPARE(recordElem.attribute("formId"), QString("0x000b1234"));
    QCOMPARE(recordElem.firstChildElement("level").attribute("value"), QString("42"));
    QCOMPARE(recordElem.firstChildElement("health").attribute("value"), QString("200"));
    QCOMPARE(recordElem.firstChildElement("race").attribute("value"), QString("3"));
    QCOMPARE(recordElem.firstChildElement("sex").attribute("value"), QString("0"));
    QCOMPARE(recordElem.firstChildElement("classId").attribute("value"), QString("2"));
}

void TestExportImport::testCsvRoundTrip_NPC()
{
    NpcRecord npc;
    npc.editorId = "CsvNPC";
    npc.fullName = "CSV Test";
    npc.level = 8;
    npc.health = 150;
    npc.race = 2;
    npc.sex = 1;
    npc.class_ = 4;
    npc.formId = 0x000C5678;

    Record<NpcRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = npc;

    QStringList headers;
    QStringList fields = DataExporter::recordToCSVFields(rec, CkId::Type_Npc_, headers);

    QVERIFY(!headers.isEmpty());
    QVERIFY(!fields.isEmpty());
    QCOMPARE(headers.size(), fields.size());

    QCOMPARE(fields[0], QString("0x000c5678"));
    QCOMPARE(fields[1], QString("CsvNPC"));
}

void TestExportImport::testJsonRoundTrip_Land()
{
    LandRecord land;
    land.editorId = "TestLand";
    land.formId = 0x000D1234;
    land.flags = 0;
    land.cellX = 5;
    land.cellY = -3;
    land.baseHeight = 128.5f;
    land.hasHeightData = true;
    land.hasNormalData = true;
    land.hasColorData = true;
    for (int x = 0; x < 33; ++x)
        for (int y = 0; y < 33; ++y) {
            land.heightData[x][y] = static_cast<qint8>((x + y) & 0xFF);
            land.normalData[x][y].nx = static_cast<qint8>(x);
            land.normalData[x][y].ny = static_cast<qint8>(y);
            land.normalData[x][y].nz = 127;
            land.colorData[x][y].r = static_cast<quint8>(x * 8);
            land.colorData[x][y].g = static_cast<quint8>(y * 8);
            land.colorData[x][y].b = 128;
            land.colorData[x][y].a = 255;
        }
    land.numTextureLayers = 2;
    land.textureLayers[0].textureFormId = 0xAA;
    land.textureLayers[0].opacity = 200;
    land.textureLayers[1].textureFormId = 0xBB;
    land.textureLayers[1].opacity = 150;

    Record<LandRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = land;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Land_);
    QVERIFY(!json.isEmpty());
    QCOMPARE(json["editorId"].toString(), QString("TestLand"));
    QCOMPARE(json["cellX"].toInt(), 5);
    QCOMPARE(json["cellY"].toInt(), -3);
    QVERIFY(qFuzzyCompare(static_cast<float>(json["baseHeight"].toDouble()), 128.5f));
    QCOMPARE(json["hasHeightData"].toBool(), true);
    QCOMPARE(json["hasNormalData"].toBool(), true);
    QCOMPARE(json["hasColorData"].toBool(), true);
    QCOMPARE(json["numTextureLayers"].toInt(), 2);
    QVERIFY(json["heightData"].toString().length() > 0);
    QVERIFY(json["normalData"].toString().length() > 0);
    QVERIFY(json["colorData"].toString().length() > 0);

    LandRecord imported;
    QVERIFY(DataImporter::importLandRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestLand"));
    QCOMPARE(imported.formId, 0x000D1234u);
    QCOMPARE(imported.cellX, 5);
    QCOMPARE(imported.cellY, -3);
    QVERIFY(qFuzzyCompare(imported.baseHeight, 128.5f));
    QCOMPARE(imported.hasHeightData, true);
    QCOMPARE(imported.hasNormalData, true);
    QCOMPARE(imported.hasColorData, true);
    QCOMPARE(imported.numTextureLayers, 2);
    QCOMPARE(imported.textureLayers[0].textureFormId, 0xAAu);
    QCOMPARE(imported.textureLayers[0].opacity, static_cast<quint8>(200));
    QCOMPARE(imported.textureLayers[1].textureFormId, 0xBBu);
    QCOMPARE(imported.textureLayers[1].opacity, static_cast<quint8>(150));

    for (int x = 0; x < 33; ++x)
        for (int y = 0; y < 33; ++y) {
            QCOMPARE(imported.heightData[x][y], land.heightData[x][y]);
            QCOMPARE(imported.normalData[x][y].nx, land.normalData[x][y].nx);
            QCOMPARE(imported.normalData[x][y].ny, land.normalData[x][y].ny);
            QCOMPARE(imported.normalData[x][y].nz, land.normalData[x][y].nz);
            QCOMPARE(imported.colorData[x][y].r, land.colorData[x][y].r);
            QCOMPARE(imported.colorData[x][y].g, land.colorData[x][y].g);
            QCOMPARE(imported.colorData[x][y].b, land.colorData[x][y].b);
            QCOMPARE(imported.colorData[x][y].a, land.colorData[x][y].a);
        }
}

void TestExportImport::testJsonRoundTrip_Stat()
{
    StatRecord stat;
    stat.editorId = "TestStat";
    stat.formId = 0x000E5678;
    stat.flags = 0x10;
    stat.iconPath = "icons/stat.dds";
    stat.modelPath = "meshes/stat.nif";
    stat.lodModelPath = "meshes/stat_lod.nif";
    stat.lodFlags = 0x05;

    Record<StatRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = stat;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Stat_);
    QVERIFY(!json.isEmpty());

    StatRecord imported;
    QVERIFY(DataImporter::importStatRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestStat"));
    QCOMPARE(imported.formId, 0x000E5678u);
    QCOMPARE(imported.flags, 0x10u);
    QCOMPARE(imported.iconPath, QString("icons/stat.dds"));
    QCOMPARE(imported.modelPath, QString("meshes/stat.nif"));
    QCOMPARE(imported.lodModelPath, QString("meshes/stat_lod.nif"));
    QCOMPARE(imported.lodFlags, 0x05u);
}

void TestExportImport::testJsonRoundTrip_Tree()
{
    TreeRecord tree;
    tree.editorId = "TestTree";
    tree.formId = 0x000F9ABC;
    tree.flags = 0x20;
    tree.iconPath = "icons/tree.dds";
    tree.modelPath = "meshes/tree.nif";
    tree.leafCurvature = 1.5f;
    tree.leafAmplitude = 0.8f;
    tree.lodModelPath = "meshes/tree_lod.nif";
    tree.lodFlags = 0x03;

    Record<TreeRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = tree;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Tree_);
    QVERIFY(!json.isEmpty());

    TreeRecord imported;
    QVERIFY(DataImporter::importTreeRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestTree"));
    QCOMPARE(imported.formId, 0x000F9ABCu);
    QCOMPARE(imported.flags, 0x20u);
    QCOMPARE(imported.iconPath, QString("icons/tree.dds"));
    QCOMPARE(imported.modelPath, QString("meshes/tree.nif"));
    QVERIFY(qFuzzyCompare(imported.leafCurvature, 1.5f));
    QVERIFY(qFuzzyCompare(imported.leafAmplitude, 0.8f));
    QCOMPARE(imported.lodModelPath, QString("meshes/tree_lod.nif"));
    QCOMPARE(imported.lodFlags, 0x03u);
}

void TestExportImport::testJsonRoundTrip_Sound()
{
    SounRecord sound;
    sound.editorId = "TestSound";
    sound.formId = 0x0010ABCD;
    sound.flags = 0;
    sound.soundFile = "sounds/test.wav";

    Record<SounRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = sound;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Soun_);
    QVERIFY(!json.isEmpty());
    QCOMPARE(json["editorId"].toString(), QString("TestSound"));
    QCOMPARE(json["formId"].toString(), QString("0x0010abcd"));
    QCOMPARE(json["soundFile"].toString(), QString("sounds/test.wav"));

    SounRecord imported;
    QVERIFY(DataImporter::importSounRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestSound"));
    QCOMPARE(imported.formId, 0x0010ABCDu);
    QCOMPARE(imported.flags, 0u);
    QCOMPARE(imported.soundFile, QString("sounds/test.wav"));
}

void TestExportImport::testJsonRoundTrip_Weather()
{
    WthrRecord weather;
    weather.editorId = "TestWeather";
    weather.formId = 0x0011EF01;
    weather.flags = 1;
    weather.sunTexture = "textures/sun.dds";

    Record<WthrRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = weather;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Wthr_);
    QVERIFY(!json.isEmpty());
    QCOMPARE(json["editorId"].toString(), QString("TestWeather"));
    QCOMPARE(json["formId"].toString(), QString("0x0011ef01"));
    QCOMPARE(json["flags"].toInt(), 1);
    QCOMPARE(json["sunTexture"].toString(), QString("textures/sun.dds"));

    WthrRecord imported;
    QVERIFY(DataImporter::importWthrRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestWeather"));
    QCOMPARE(imported.formId, 0x0011EF01u);
    QCOMPARE(imported.flags, 1u);
    QCOMPARE(imported.sunTexture, QString("textures/sun.dds"));
}

void TestExportImport::testJsonRoundTrip_LandTexture()
{
    LtexRecord ltex;
    ltex.editorId = "TestLtex";
    ltex.formId = 0x00125678;
    ltex.flags = 0;
    ltex.iconPath = "icons/landtex.dds";
    ltex.havokMaterial = 42;
    ltex.grassFormIds = {0x00200001, 0x00200002, 0x00200003};

    Record<LtexRecord> rec;
    rec.state = State_ModifiedOnly;
    rec.modifiedRecord = ltex;

    QJsonObject json = DataExporter::recordToJSON(rec, CkId::Type_Ltex_);
    QVERIFY(!json.isEmpty());
    QCOMPARE(json["editorId"].toString(), QString("TestLtex"));
    QCOMPARE(json["formId"].toString(), QString("0x00125678"));
    QCOMPARE(json["iconPath"].toString(), QString("icons/landtex.dds"));
    QCOMPARE(json["havokMaterial"].toInt(), 42);
    QVERIFY(json["grassFormIds"].toArray().size() == 3);

    LtexRecord imported;
    QVERIFY(DataImporter::importLtexRecord(json, imported));

    QCOMPARE(imported.editorId, QString("TestLtex"));
    QCOMPARE(imported.formId, 0x00125678u);
    QCOMPARE(imported.flags, 0u);
    QCOMPARE(imported.iconPath, QString("icons/landtex.dds"));
    QCOMPARE(imported.havokMaterial, 42u);
    QCOMPARE(imported.grassFormIds, QVector<quint32>({0x00200001, 0x00200002, 0x00200003}));
}

QTEST_MAIN(TestExportImport)
#include "test_exportimport.moc"

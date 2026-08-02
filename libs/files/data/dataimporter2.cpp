#include "dataimporter.hpp"
#include "../esm/npcrecord.hpp"
#include "../esm/weaprecord.hpp"
#include "../esm/armorrecord.hpp"
#include "../esm/spellrecord.hpp"
#include "../esm/questrecord.hpp"
#include "../esm/cellrecord.hpp"
#include "../esm/Actirecord.hpp"
#include "../esm/Bookrecord.hpp"
#include "../esm/Miscrecord.hpp"
#include "../esm/Ingrrecord.hpp"
#include "../esm/Alchrecord.hpp"
#include "../esm/Enchrecord.hpp"
#include "../esm/Contrecord.hpp"
#include "../esm/Racerecord.hpp"
#include "../esm/Perkrecord.hpp"
#include "../esm/Magicrecord.hpp"
#include "../esm/Packagerecord.hpp"
#include "../esm/Classrecord.hpp"
#include "../esm/Factrecord.hpp"
#include "../esm/glob.hpp"
#include "../esm/Treerecord.hpp"
#include "../esm/Statrecord.hpp"
#include "../esm/lcrt.hpp"
#include "../esm/worldspacerecord.hpp"
#include "../esm/locationrecord.hpp"
#include "../esm/refrecord.hpp"
#include "../esm/materialrecord.hpp"
#include "../esm/Dialrecord.hpp"
#include "../esm/Inforecord.hpp"
#include "../esm/landrecord.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>
#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QByteArray>

quint32 DataImporter::parseFormId(const QString& formIdStr)
{
    bool ok = false;
    quint32 value = formIdStr.toUInt(&ok, 16);
    return ok ? value : 0;
}

State DataImporter::parseState(const QString& stateStr)
{
    if (stateStr == "Modified") return State_Modified;
    if (stateStr == "ModifiedOnly") return State_ModifiedOnly;
    if (stateStr == "Deleted") return State_Deleted;
    return State_Base;
}

QString DataImporter::extractField(const QJsonObject& obj, const QString& key)
{
    auto it = obj.find(key);
    if (it != obj.end())
    {
        return it.value().toString();
    }
    return QString();
}

int DataImporter::extractIntField(const QJsonObject& obj, const QString& key, int defaultValue)
{
    auto it = obj.find(key);
    if (it != obj.end())
    {
        return it.value().toInt(defaultValue);
    }
    return defaultValue;
}

quint32 DataImporter::extractUInt32Field(const QJsonObject& obj, const QString& key, quint32 defaultValue)
{
    auto it = obj.find(key);
    if (it != obj.end())
    {
        QJsonValue v = it.value();
        if (v.isDouble())
        {
            return static_cast<quint32>(static_cast<qint64>(v.toDouble()));
        }
        return static_cast<quint32>(v.toVariant().toULongLong());
    }
    return defaultValue;
}

float DataImporter::extractFloatField(const QJsonObject& obj, const QString& key, float defaultValue)
{
    auto it = obj.find(key);
    if (it != obj.end())
    {
        return static_cast<float>(it.value().toDouble(defaultValue));
    }
    return defaultValue;
}

bool DataImporter::extractBoolField(const QJsonObject& obj, const QString& key, bool defaultValue)
{
    auto it = obj.find(key);
    if (it != obj.end())
    {
        return it.value().toBool(defaultValue);
    }
    return defaultValue;
}

QVector<quint32> DataImporter::extractIntVector(const QJsonObject& obj, const QString& key)
{
    QVector<quint32> result;
    auto it = obj.find(key);
    if (it != obj.end() && it.value().isArray())
    {
        QJsonArray arr = it.value().toArray();
        for (const QJsonValue& v : arr)
        {
            result.append(static_cast<quint32>(v.toInt()));
        }
    }
    return result;
}

QVector<QString> DataImporter::extractStringVector(const QJsonObject& obj, const QString& key)
{
    QVector<QString> result;
    auto it = obj.find(key);
    if (it != obj.end() && it.value().isArray())
    {
        QJsonArray arr = it.value().toArray();
        for (const QJsonValue& v : arr)
        {
            result.append(v.toString());
        }
    }
    return result;
}

bool DataImporter::importNpcRecord(const QJsonObject& json, NpcRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.fullName = extractField(json, "fullName");
    record.level = extractIntField(json, "level", 0);
    record.health = extractIntField(json, "health", 0);
    record.magicka = extractIntField(json, "magicka", 0);
    record.stamina = extractIntField(json, "stamina", 0);
    record.attack = extractIntField(json, "attack", 0);
    record.defense = extractIntField(json, "defense", 0);
    record.personality = extractIntField(json, "personality", 0);
    record.intelligence = extractIntField(json, "intelligence", 0);
    record.willpower = extractIntField(json, "willpower", 0);
    record.agility = extractIntField(json, "agility", 0);
    record.luck = extractIntField(json, "luck", 0);
    record.disposition = extractIntField(json, "disposition", 0);
    record.reputation = extractIntField(json, "reputation", 0);
    record.aiIndex = extractIntField(json, "aiIndex", 0);
    record.aiGlobal = extractIntField(json, "aiGlobal", 0);
    record.aiFacet = extractIntField(json, "aiFacet", 0);
    record.aiRank = extractIntField(json, "aiRank", 0);
    record.aiFaction = extractIntField(json, "aiFaction", 0);
    record.aiSound = extractIntField(json, "aiSound", 0);
    record.aiAlert = extractIntField(json, "aiAlert", 0);
    record.aiCombat = extractIntField(json, "aiCombat", 0);
    record.aiHazard = extractIntField(json, "aiHazard", 0);
    record.aiClass = extractIntField(json, "aiClass", 0);
    record.aiRace = extractIntField(json, "aiRace", 0);
    record.aiCompany = extractIntField(json, "aiCompany", 0);
    record.aiFactionRank = extractIntField(json, "aiFactionRank", 0);
    record.aiFactionBase = extractIntField(json, "aiFactionBase", 0);
    record.aiFactionMember = extractIntField(json, "aiFactionMember", 0);
    record.aiFactionTarget = extractIntField(json, "aiFactionTarget", 0);
    record.aiFactionTargetRank = extractIntField(json, "aiFactionTargetRank", 0);
    record.aiFactionTargetBase = extractIntField(json, "aiFactionTargetBase", 0);
    record.aiFactionTargetMember = extractIntField(json, "aiFactionTargetMember", 0);
    record.aiFactionTargetClass = extractIntField(json, "aiFactionTargetClass", 0);
    record.aiFactionTargetRace = extractIntField(json, "aiFactionTargetRace", 0);
    record.aiFactionTargetCompany = extractIntField(json, "aiFactionTargetCompany", 0);
    record.aiFactionTargetFacet = extractIntField(json, "aiFactionTargetFacet", 0);
    record.aiFactionTargetSound = extractIntField(json, "aiFactionTargetSound", 0);
    record.aiFactionTargetAlert = extractIntField(json, "aiFactionTargetAlert", 0);
    record.aiFactionTargetCombat = extractIntField(json, "aiFactionTargetCombat", 0);
    record.aiFactionTargetHazard = extractIntField(json, "aiFactionTargetHazard", 0);
    record.aiFactionTargetClassRank = extractIntField(json, "aiFactionTargetClassRank", 0);
    record.aiFactionTargetClassBase = extractIntField(json, "aiFactionTargetClassBase", 0);
    record.aiFactionTargetClassMember = extractIntField(json, "aiFactionTargetClassMember", 0);
    record.aiFactionTargetClassFacet = extractIntField(json, "aiFactionTargetClassFacet", 0);
    record.aiFactionTargetClassSound = extractIntField(json, "aiFactionTargetClassSound", 0);
    record.aiFactionTargetClassAlert = extractIntField(json, "aiFactionTargetClassAlert", 0);
    record.aiFactionTargetClassCombat = extractIntField(json, "aiFactionTargetClassCombat", 0);
    record.aiFactionTargetClassHazard = extractIntField(json, "aiFactionTargetClassHazard", 0);
    record.aiFactionTargetClassTarget = extractIntField(json, "aiFactionTargetClassTarget", 0);
    record.aiFactionTargetClassTargetRank = extractIntField(json, "aiFactionTargetClassTargetRank", 0);
    record.aiFactionTargetClassTargetBase = extractIntField(json, "aiFactionTargetClassTargetBase", 0);
    record.aiFactionTargetClassTargetMember = extractIntField(json, "aiFactionTargetClassTargetMember", 0);
    record.aiFactionTargetClassTargetFacet = extractIntField(json, "aiFactionTargetClassTargetFacet", 0);
    record.aiFactionTargetClassTargetSound = extractIntField(json, "aiFactionTargetClassTargetSound", 0);
    record.aiFactionTargetClassTargetAlert = extractIntField(json, "aiFactionTargetClassTargetAlert", 0);
    record.aiFactionTargetClassTargetCombat = extractIntField(json, "aiFactionTargetClassTargetCombat", 0);
    record.aiFactionTargetClassTargetHazard = extractIntField(json, "aiFactionTargetClassTargetHazard", 0);
    record.race = extractIntField(json, "race", 0);
    record.sex = extractIntField(json, "sex", 0);
    record.class_ = extractIntField(json, "classId", 0);
    record.faction = extractIntField(json, "faction", 0);
    record.aiAggroRadius = extractIntField(json, "aiAggroRadius", 0);
    record.spells = extractIntVector(json, "spells");
    record.inventoryItems = extractIntVector(json, "inventoryItems");
    record.relationships = extractIntVector(json, "relationships");
    
    return true;
}

bool DataImporter::importWeaponRecord(const QJsonObject& json, WeaponRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.fullName = extractField(json, "fullName");
    record.weaponType = extractIntField(json, "weaponType", 0);
    record.damage = extractFloatField(json, "damage", 0.0f);
    record.speed = extractFloatField(json, "speed", 1.0f);
    record.reach = extractFloatField(json, "reach", 0.0f);
    record.weight = extractFloatField(json, "weight", 0.0f);
    record.value = extractIntField(json, "value", 0);
    record.enchantment = extractIntField(json, "enchantment", 0);
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    record.magicSchool = extractIntField(json, "magicSchool", 0);
    record.enchantLimit = extractIntField(json, "enchantLimit", 0);
    
    return true;
}

bool DataImporter::importArmorRecord(const QJsonObject& json, ArmorRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.fullName = extractField(json, "fullName");
    record.armorRating = extractIntField(json, "armorRating", 0);
    record.weight = extractFloatField(json, "weight", 0.0f);
    record.value = extractIntField(json, "value", 0);
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    record.health = extractFloatField(json, "health", 0.0f);
    
    return true;
}

bool DataImporter::importSpellRecord(const QJsonObject& json, SpellRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.fullName = extractField(json, "fullName");
    record.cost = extractIntField(json, "cost", 0);
    record.castingSound = extractIntField(json, "castingSound", 0);
    record.effects = extractIntVector(json, "effects");
    record.enchantment = extractIntField(json, "enchantment", 0);
    
    return true;
}

bool DataImporter::importQuestRecord(const QJsonObject& json, QuestRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.questName = extractField(json, "questName");
    record.questDesc = extractField(json, "questDesc");
    record.questType = extractIntField(json, "questType", 0);
    record.stageIds = extractIntVector(json, "stageIds");
    record.stageDescriptions = extractStringVector(json, "stageDescriptions");
    record.objectiveIds = extractIntVector(json, "objectiveIds");
    record.aliasIds = extractIntVector(json, "aliasIds");
    record.dialogueView = extractField(json, "dialogueView");
    record.scriptIds = extractIntVector(json, "scriptIds");
    
    return true;
}

bool DataImporter::importCellRecord(const QJsonObject& json, CellRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.cellX = extractUInt32Field(json, "cellX", 0);
    record.cellY = extractUInt32Field(json, "cellY", 0);
    record.owner = extractIntField(json, "owner", 0);
    record.lockLevel = extractIntField(json, "lockLevel", 0);
    record.cellName = extractField(json, "cellName");
    
    return true;
}

bool DataImporter::importActiRecord(const QJsonObject& json, ActiRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    
    return true;
}

bool DataImporter::importBookRecord(const QJsonObject& json, BookRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.pageCount = extractIntField(json, "pageCount", 0);
    record.pages = extractField(json, "pages");
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    
    return true;
}

bool DataImporter::importMiscRecord(const QJsonObject& json, MiscRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    record.weight = extractFloatField(json, "weight", 0.0f);
    record.value = extractIntField(json, "value", 0);
    
    return true;
}

bool DataImporter::importIngrRecord(const QJsonObject& json, IngrRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    record.weight = extractFloatField(json, "weight", 0.0f);
    record.value = extractIntField(json, "value", 0);
    
    return true;
}

bool DataImporter::importAlchRecord(const QJsonObject& json, AlchRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    record.weight = extractFloatField(json, "weight", 0.0f);
    record.value = extractIntField(json, "value", 0);
    
    return true;
}

bool DataImporter::importEnchRecord(const QJsonObject& json, EnchRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.name = extractField(json, "name");
    record.costLimit = extractIntField(json, "costLimit", 0);
    record.charges = extractIntField(json, "charges", 0);
    record.enchantmentData = extractIntField(json, "enchantmentData", 0);
    record.charge = extractFloatField(json, "charge", 0.0f);
    record.duration = extractIntField(json, "duration", 0);
    record.magnitude = extractFloatField(json, "magnitude", 0.0f);
    record.type = extractIntField(json, "type", 0);
    record.soulGem = extractIntField(json, "soulGem", 0);
    
    return true;
}

bool DataImporter::importContRecord(const QJsonObject& json, ContRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    record.contents = extractIntField(json, "contents", 0);
    record.inventoryControl = extractIntField(json, "inventoryControl", 0);
    record.weight = extractFloatField(json, "weight", 0.0f);
    record.value = extractIntField(json, "value", 0);
    
    return true;
}

bool DataImporter::importRaceRecord(const QJsonObject& json, RaceRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.raceFlags = extractIntField(json, "raceFlags", 0);
    record.npcVariables = extractIntVector(json, "npcVariables");
    record.faceData = extractIntVector(json, "faceData");
    record.headData = extractIntVector(json, "headData");
    
    return true;
}

bool DataImporter::importPerkRecord(const QJsonObject& json, PerkRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.description = extractField(json, "description");
    record.requirements = extractField(json, "requirements");
    record.iconPath = extractField(json, "iconPath");
    record.conditions = extractIntVector(json, "conditions");
    
    return true;
}

bool DataImporter::importMagicRecord(const QJsonObject& json, MagicRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.schools = extractIntField(json, "schools", 0);
    record.damageType = extractIntField(json, "damageType", 0);
    record.castingSound = extractIntField(json, "castingSound", 0);
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    record.effects = extractIntVector(json, "effects");
    
    return true;
}

bool DataImporter::importPackageRecord(const QJsonObject& json, PackageRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.packageType = extractIntField(json, "packageType", 0);
    record.targetType = extractIntField(json, "targetType", 0);
    record.targetIds = extractIntVector(json, "targetIds");
    record.parameters = extractIntVector(json, "parameters");
    
    return true;
}

bool DataImporter::importClassRecord(const QJsonObject& json, ClassRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.className = extractField(json, "className");
    record.description = extractField(json, "description");
    record.serviceFlags = extractIntField(json, "serviceFlags", 0);
    record.iconPath = extractField(json, "iconPath");
    
    return true;
}

bool DataImporter::importFactRecord(const QJsonObject& json, FactRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.factionName = extractField(json, "factionName");
    record.description = extractField(json, "description");
    record.iconPath = extractField(json, "iconPath");
    record.ranks = extractStringVector(json, "ranks");
    record.relations = extractIntVector(json, "relations");
    
    return true;
}

bool DataImporter::importGlobRecord(const QJsonObject& json, GlobalVariable& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.constant = extractBoolField(json, "constant", false);
    
    QString valStr = extractField(json, "value");
    if (!valStr.isEmpty())
    {
        bool ok;
        int intVal = valStr.toInt(&ok);
        if (ok)
        {
            record.value.setInt(static_cast<quint32>(intVal));
        }
        else
        {
            float floatVal = valStr.toFloat(&ok);
            if (ok)
            {
                record.value.setFloat(floatVal);
            }
            else
            {
                record.value.setString(valStr);
            }
        }
    }
    
    return true;
}

bool DataImporter::importTreeRecord(const QJsonObject& json, TreeRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    record.leafCurvature = extractFloatField(json, "leafCurvature", 0.0f);
    record.leafAmplitude = extractFloatField(json, "leafAmplitude", 0.0f);
    record.lodModelPath = extractField(json, "lodModelPath");
    record.lodFlags = extractIntField(json, "lodFlags", 0);
    
    return true;
}

bool DataImporter::importStatRecord(const QJsonObject& json, StatRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    record.lodModelPath = extractField(json, "lodModelPath");
    record.lodFlags = extractIntField(json, "lodFlags", 0);
    
    return true;
}

bool DataImporter::importLcrtRecord(const QJsonObject& json, LocationRefType& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.color = extractIntField(json, "color", 0);
    
    return true;
}

bool DataImporter::importWorldspaceRecord(const QJsonObject& json, WorldspaceRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.name = extractField(json, "name");
    record.waterType = extractIntField(json, "waterType", 0);
    record.climateId = extractIntField(json, "climateId", 0);
    record.lightingId = extractIntField(json, "lightingId", 0);
    record.mapWidth = extractIntField(json, "mapWidth", 0);
    record.mapHeight = extractIntField(json, "mapHeight", 0);
    record.mapNwX = extractIntField(json, "mapNwX", 0);
    record.mapNwY = extractIntField(json, "mapNwY", 0);
    record.mapSeX = extractIntField(json, "mapSeX", 0);
    record.mapSeY = extractIntField(json, "mapSeY", 0);
    record.setMapScale(extractFloatField(json, "mapScale", 1.0f));
    record.mapLodBias = extractFloatField(json, "mapLodBias", 1.0f);
    record.mapSize = extractIntField(json, "mapSize", 0);
    record.templ = extractIntField(json, "template", 0);
    record.terrain = extractIntField(json, "terrain", 0);
    record.mapImage = extractField(json, "mapImage");
    record.lodNoise = extractField(json, "lodNoise");
    record.billboardTexture = extractField(json, "billboardTexture");
    record.music = extractIntField(json, "music", 0);
    record.dnam = extractIntField(json, "dnam", 0);
    record.dataMinX = extractIntField(json, "dataMinX", 0);
    record.dataMinY = extractIntField(json, "dataMinY", 0);
    record.cellIds = extractIntVector(json, "cellIds");
    record.navPointIds = extractIntVector(json, "navPointIds");
    
    return true;
}

bool DataImporter::importLocationRecord(const QJsonObject& json, LocationRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.locationName = extractField(json, "locationName");
    record.parentId = extractIntField(json, "parentId", 0);
    record.x = extractIntField(json, "x", 0);
    record.y = extractIntField(json, "y", 0);
    record.z = extractIntField(json, "z", 0);
    
    return true;
}

bool DataImporter::importRefrRecord(const QJsonObject& json, RefrRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.formId = parseFormId(extractField(json, "formId"));
    record.baseId = extractIntField(json, "baseId", 0);
    record.posX = extractFloatField(json, "posX", 0.0f);
    record.posY = extractFloatField(json, "posY", 0.0f);
    record.posZ = extractFloatField(json, "posZ", 0.0f);
    record.rotX = extractFloatField(json, "rotX", 0.0f);
    record.rotY = extractFloatField(json, "rotY", 0.0f);
    record.rotZ = extractFloatField(json, "rotZ", 0.0f);
    record.scale = extractFloatField(json, "scale", 1.0f);
    record.owner = extractIntField(json, "owner", 0);
    record.lockLevel = extractIntField(json, "lockLevel", 0);
    record.initiallyDisabled = extractBoolField(json, "initiallyDisabled", false);
    record.scriptIds = extractIntVector(json, "scriptIds");
    
    return true;
}

bool DataImporter::importMaterialRecord(const QJsonObject& json, MaterialRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.materialName = extractField(json, "materialName");
    record.name = extractField(json, "name");
    record.description = extractField(json, "description");
    record.iconPath = extractField(json, "iconPath");
    record.modelPath = extractField(json, "modelPath");
    record.bnam = extractField(json, "bnam");
    record.cnam = extractField(json, "cnam");
    record.texturePath = extractField(json, "texturePath");
    record.materialType = extractIntField(json, "materialType", 0);
    record.value = extractIntField(json, "value", 0);
    record.weight = extractIntField(json, "weight", 0);
    record.health = extractIntField(json, "health", 0);
    record.magicka = extractIntField(json, "magicka", 0);
    record.stamina = extractIntField(json, "stamina", 0);
    record.level = extractIntField(json, "level", 0);
    record.race = extractIntField(json, "race", 0);
    record.faction = extractIntField(json, "faction", 0);
    record.stage = extractIntField(json, "stage", 0);
    record.difficulty = extractIntField(json, "difficulty", 0);
    
    return true;
}

bool DataImporter::importDialRecord(const QJsonObject& json, DialRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.topicName = extractField(json, "topicName");
    record.responseIds = extractIntVector(json, "responseIds");
    record.conditionIds = extractIntVector(json, "conditionIds");
    record.animationIds = extractIntVector(json, "animationIds");
    record.emotionIds = extractIntVector(json, "emotionIds");
    
    return true;
}

bool DataImporter::importInfoRecord(const QJsonObject& json, InfoRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.responseText = extractField(json, "responseText");
    record.conditionIds = extractIntVector(json, "conditionIds");
    record.targetId = extractIntField(json, "targetId", 0);
    record.scriptIds = extractIntVector(json, "scriptIds");
    
    return true;
}

bool DataImporter::importLandRecord(const QJsonObject& json, LandRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;
    
    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractIntField(json, "flags", 0);
    record.cellX = extractIntField(json, "cellX", 0);
    record.cellY = extractIntField(json, "cellY", 0);
    record.baseHeight = extractFloatField(json, "baseHeight", 0.0f);
    record.hasHeightData = extractBoolField(json, "hasHeightData", false);
    record.hasNormalData = extractBoolField(json, "hasNormalData", false);
    record.hasColorData = extractBoolField(json, "hasColorData", false);
    
    if (record.hasHeightData) {
        QByteArray hd = QByteArray::fromBase64(extractField(json, "heightData").toUtf8());
        if (hd.size() >= static_cast<int>(sizeof(record.heightData)))
            memcpy(record.heightData, hd.data(), sizeof(record.heightData));
    }
    if (record.hasNormalData) {
        QByteArray nd = QByteArray::fromBase64(extractField(json, "normalData").toUtf8());
        if (nd.size() >= static_cast<int>(sizeof(record.normalData)))
            memcpy(record.normalData, nd.data(), sizeof(record.normalData));
    }
    if (record.hasColorData) {
        QByteArray cd = QByteArray::fromBase64(extractField(json, "colorData").toUtf8());
        if (cd.size() >= static_cast<int>(sizeof(record.colorData)))
            memcpy(record.colorData, cd.data(), sizeof(record.colorData));
    }
    
    record.numTextureLayers = extractIntField(json, "numTextureLayers", 0);
    auto texArr = json.find("textureLayers");
    if (texArr != json.end() && texArr.value().isArray()) {
        QJsonArray arr = texArr.value().toArray();
        for (int i = 0; i < arr.size() && i < 4; ++i) {
            QJsonObject texObj = arr[i].toObject();
            record.textureLayers[i].textureFormId = static_cast<quint32>(texObj["textureFormId"].toInt());
            record.textureLayers[i].opacity = static_cast<quint8>(texObj["opacity"].toInt());
        }
    }
    
    return true;
}

bool DataImporter::importSounRecord(const QJsonObject& json, SounRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;

    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractUInt32Field(json, "flags", 0);
    record.soundFile = extractField(json, "soundFile");
    return true;
}

bool DataImporter::importWthrRecord(const QJsonObject& json, WthrRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;

    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractUInt32Field(json, "flags", 0);
    record.sunTexture = extractField(json, "sunTexture");
    return true;
}

bool DataImporter::importLtexRecord(const QJsonObject& json, LtexRecord& record)
{
    QString editorId = extractField(json, "editorId");
    if (editorId.isEmpty()) return false;

    record.editorId = editorId;
    record.formId = parseFormId(extractField(json, "formId"));
    record.flags = extractUInt32Field(json, "flags", 0);
    record.iconPath = extractField(json, "iconPath");
    record.havokMaterial = extractUInt32Field(json, "havokMaterial", 0);
    record.grassFormIds = extractIntVector(json, "grassFormIds");
    return true;
}

DataImporter::ImportResult DataImporter::importFromJSON(Data& data, const QString& filePath)
{
    ImportResult result;
    result.recordsImported = 0;
    result.recordsSkipped = 0;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        result.error = QString("Cannot open file: %1").arg(filePath);
        return result;
    }
    
    QByteArray dataBytes = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(dataBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        result.error = QString("JSON parse error: %1").arg(parseError.errorString());
        return result;
    }
    
    if (!doc.isObject())
    {
        result.error = "Invalid JSON structure";
        return result;
    }
    
    QJsonObject root = doc.object();
    QJsonArray typeArray = root["records"].toArray();
    
    for (const QJsonValue& typeValue : typeArray)
    {
        QJsonObject typeObj = typeValue.toObject();
        QString typeName = typeObj["typeName"].toString();
        QJsonArray records = typeObj["records"].toArray();
        
        CkId::Type type = CkId::Type_None;
        if (typeName == "NPC") type = CkId::Type_Npc_;
        else if (typeName == "Weapon") type = CkId::Type_Weap_;
        else if (typeName == "Armor") type = CkId::Type_Armor_;
        else if (typeName == "Spell") type = CkId::Type_Spel_;
        else if (typeName == "Quest") type = CkId::Type_Quest_;
        else if (typeName == "Activator") type = CkId::Type_Acti_;
        else if (typeName == "Book") type = CkId::Type_Book_;
        else if (typeName == "Misc") type = CkId::Type_Misc_;
        else if (typeName == "Ingredient") type = CkId::Type_Ingr_;
        else if (typeName == "Alchemy") type = CkId::Type_Alch_;
        else if (typeName == "Enchantment") type = CkId::Type_Ench_;
        else if (typeName == "Container") type = CkId::Type_Cont_;
        else if (typeName == "Race") type = CkId::Type_Race_;
        else if (typeName == "Perk") type = CkId::Type_PerK_;
        else if (typeName == "Magic Effect") type = CkId::Type_Magic_;
        else if (typeName == "Package") type = CkId::Type_Pack_;
        else if (typeName == "Class") type = CkId::Type_Class_;
        else if (typeName == "Faction") type = CkId::Type_Fact_;
        else if (typeName == "Global") type = CkId::Type_Glob_;
        else if (typeName == "Leveled Creature Group") type = CkId::Type_Tree_;
        else if (typeName == "Static") type = CkId::Type_Stat_;
        else if (typeName == "Leveled Creature") type = CkId::Type_Lcrt_;
        else if (typeName == "Worldspace") type = CkId::Type_WRLD_;
        else if (typeName == "Location") type = CkId::Type_LOCT_;
        else if (typeName == "Refr") type = CkId::Type_Refr_;
        else if (typeName == "Material") type = CkId::Type_Material_;
        else if (typeName == "Dialogue") type = CkId::Type_Dial_;
        else if (typeName == "Info") type = CkId::Type_Info_;
        else if (typeName == "Cell") type = CkId::Type_Cel_;
        else if (typeName == "Landscape") type = CkId::Type_Land_;
        else continue;
        
        for (const QJsonValue& recordValue : records)
        {
            QJsonObject recordJson = recordValue.toObject();
            
            switch (type)
            {
            case CkId::Type_Npc_:
            {
                NpcRecord npc;
                if (importNpcRecord(recordJson, npc))
                {
                    data.addNpc(npc);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Weap_:
            {
                WeaponRecord weap;
                if (importWeaponRecord(recordJson, weap))
                {
                    data.addWeapon(weap);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Armor_:
            {
                ArmorRecord armor;
                if (importArmorRecord(recordJson, armor))
                {
                    data.addArmor(armor);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Spel_:
            {
                SpellRecord spell;
                if (importSpellRecord(recordJson, spell))
                {
                    data.addSpell(spell);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Quest_:
            {
                QuestRecord quest;
                if (importQuestRecord(recordJson, quest))
                {
                    data.addQuest(quest);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Cel_:
            {
                CellRecord cell;
                if (importCellRecord(recordJson, cell))
                {
                    data.addCell(cell);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Acti_:
            {
                ActiRecord acti;
                if (importActiRecord(recordJson, acti))
                {
                    data.addActi(acti);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Book_:
            {
                BookRecord book;
                if (importBookRecord(recordJson, book))
                {
                    data.addBook(book);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Misc_:
            {
                MiscRecord misc;
                if (importMiscRecord(recordJson, misc))
                {
                    data.addMisc(misc);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Ingr_:
            {
                IngrRecord ingr;
                if (importIngrRecord(recordJson, ingr))
                {
                    data.addIngr(ingr);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Alch_:
            {
                AlchRecord alch;
                if (importAlchRecord(recordJson, alch))
                {
                    data.addAlch(alch);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Ench_:
            {
                EnchRecord ench;
                if (importEnchRecord(recordJson, ench))
                {
                    data.addEnch(ench);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Cont_:
            {
                ContRecord cont;
                if (importContRecord(recordJson, cont))
                {
                    data.addCont(cont);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Race_:
            {
                RaceRecord race;
                if (importRaceRecord(recordJson, race))
                {
                    data.addRace(race);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_PerK_:
            {
                PerkRecord perk;
                if (importPerkRecord(recordJson, perk))
                {
                    data.addPerk(perk);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Magic_:
            {
                MagicRecord magic;
                if (importMagicRecord(recordJson, magic))
                {
                    data.addMagic(magic);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Pack_:
            {
                PackageRecord pack;
                if (importPackageRecord(recordJson, pack))
                {
                    data.addPack(pack);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Class_:
            {
                ClassRecord cls;
                if (importClassRecord(recordJson, cls))
                {
                    data.addClass(cls);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Fact_:
            {
                FactRecord fact;
                if (importFactRecord(recordJson, fact))
                {
                    data.addFact(fact);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Glob_:
            {
                GlobalVariable glob;
                if (importGlobRecord(recordJson, glob))
                {
                    data.addGlobVar(glob);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Tree_:
            {
                TreeRecord tree;
                if (importTreeRecord(recordJson, tree))
                {
                    data.addTree(tree);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Stat_:
            {
                StatRecord stat;
                if (importStatRecord(recordJson, stat))
                {
                    data.addStat(stat);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Lcrt_:
            {
                LocationRefType lcrt;
                if (importLcrtRecord(recordJson, lcrt))
                {
                    data.addLcrt(lcrt);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_WRLD_:
            {
                WorldspaceRecord wrld;
                if (importWorldspaceRecord(recordJson, wrld))
                {
                    data.addWorldspace(wrld);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_LOCT_:
            {
                LocationRecord loct;
                if (importLocationRecord(recordJson, loct))
                {
                    data.addLocation(loct);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Refr_:
            {
                RefrRecord refr;
                if (importRefrRecord(recordJson, refr))
                {
                    data.addRef(refr);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Material_:
            {
                MaterialRecord mat;
                if (importMaterialRecord(recordJson, mat))
                {
                    data.addMaterial(mat);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Dial_:
            {
                DialRecord dial;
                if (importDialRecord(recordJson, dial))
                {
                    data.addDial(dial);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Info_:
            {
                InfoRecord info;
                if (importInfoRecord(recordJson, info))
                {
                    data.addInfo(info);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Land_:
            {
                LandRecord land;
                if (importLandRecord(recordJson, land))
                {
                    data.addLand(land);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Soun_:
            {
                SounRecord soun;
                if (importSounRecord(recordJson, soun))
                {
                    data.addSoun(soun);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Wthr_:
            {
                WthrRecord wthr;
                if (importWthrRecord(recordJson, wthr))
                {
                    data.addWthr(wthr);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            case CkId::Type_Ltex_:
            {
                LtexRecord ltex;
                if (importLtexRecord(recordJson, ltex))
                {
                    data.addLtex(ltex);
                    result.recordsImported++;
                }
                else
                {
                    result.recordsSkipped++;
                }
                break;
            }
            default:
                result.warnings.append(QString("Unsupported type for import: %1").arg(typeName));
                result.recordsSkipped++;
                break;
            }
        }
    }
    
    return result;
}

DataImporter::ImportResult DataImporter::importCSVByType(Data& data, CkId::Type type, const QStringList& headers, const QStringList& lines)
{
    ImportResult result;
    result.recordsImported = 0;
    result.recordsSkipped = 0;
    
    for (int i = 0; i < lines.size(); i++)
    {
        QStringList fields = lines[i].split(",");
        if (fields.size() < headers.size())
        {
            result.warnings.append(QString("Line %1: insufficient fields").arg(i + 1));
            result.recordsSkipped++;
            continue;
        }
        
        QJsonObject recordJson;
        for (int j = 0; j < fields.size() && j < headers.size(); j++)
        {
            QString key = headers[j].trimmed();
            key[0] = key[0].toLower();
            if (key.endsWith("ID"))
            {
                key = key.left(key.length() - 2) + "Id";
            }
            recordJson[key] = fields[j].trimmed();
        }
        
        switch (type)
        {
        case CkId::Type_Npc_:
        {
            NpcRecord npc;
            if (importNpcRecord(recordJson, npc))
            {
                data.addNpc(npc);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Weap_:
        {
            WeaponRecord weap;
            if (importWeaponRecord(recordJson, weap))
            {
                data.addWeapon(weap);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Armor_:
        {
            ArmorRecord armor;
            if (importArmorRecord(recordJson, armor))
            {
                data.addArmor(armor);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Spel_:
        {
            SpellRecord spell;
            if (importSpellRecord(recordJson, spell))
            {
                data.addSpell(spell);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Quest_:
        {
            QuestRecord quest;
            if (importQuestRecord(recordJson, quest))
            {
                data.addQuest(quest);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Cel_:
        {
            CellRecord cell;
            if (importCellRecord(recordJson, cell))
            {
                data.addCell(cell);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Acti_:
        {
            ActiRecord acti;
            if (importActiRecord(recordJson, acti))
            {
                data.addActi(acti);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Book_:
        {
            BookRecord book;
            if (importBookRecord(recordJson, book))
            {
                data.addBook(book);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Misc_:
        {
            MiscRecord misc;
            if (importMiscRecord(recordJson, misc))
            {
                data.addMisc(misc);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Ingr_:
        {
            IngrRecord ingr;
            if (importIngrRecord(recordJson, ingr))
            {
                data.addIngr(ingr);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Alch_:
        {
            AlchRecord alch;
            if (importAlchRecord(recordJson, alch))
            {
                data.addAlch(alch);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Ench_:
        {
            EnchRecord ench;
            if (importEnchRecord(recordJson, ench))
            {
                data.addEnch(ench);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Cont_:
        {
            ContRecord cont;
            if (importContRecord(recordJson, cont))
            {
                data.addCont(cont);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Race_:
        {
            RaceRecord race;
            if (importRaceRecord(recordJson, race))
            {
                data.addRace(race);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_PerK_:
        {
            PerkRecord perk;
            if (importPerkRecord(recordJson, perk))
            {
                data.addPerk(perk);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Magic_:
        {
            MagicRecord magic;
            if (importMagicRecord(recordJson, magic))
            {
                data.addMagic(magic);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Pack_:
        {
            PackageRecord pack;
            if (importPackageRecord(recordJson, pack))
            {
                data.addPack(pack);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Class_:
        {
            ClassRecord cls;
            if (importClassRecord(recordJson, cls))
            {
                data.addClass(cls);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Fact_:
        {
            FactRecord fact;
            if (importFactRecord(recordJson, fact))
            {
                data.addFact(fact);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Glob_:
        {
            GlobalVariable glob;
            if (importGlobRecord(recordJson, glob))
            {
                data.addGlobVar(glob);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Tree_:
        {
            TreeRecord tree;
            if (importTreeRecord(recordJson, tree))
            {
                data.addTree(tree);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Stat_:
        {
            StatRecord stat;
            if (importStatRecord(recordJson, stat))
            {
                data.addStat(stat);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Lcrt_:
        {
            LocationRefType lcrt;
            if (importLcrtRecord(recordJson, lcrt))
            {
                data.addLcrt(lcrt);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_WRLD_:
        {
            WorldspaceRecord wrld;
            if (importWorldspaceRecord(recordJson, wrld))
            {
                data.addWorldspace(wrld);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_LOCT_:
        {
            LocationRecord loct;
            if (importLocationRecord(recordJson, loct))
            {
                data.addLocation(loct);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Refr_:
        {
            RefrRecord refr;
            if (importRefrRecord(recordJson, refr))
            {
                data.addRef(refr);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Material_:
        {
            MaterialRecord mat;
            if (importMaterialRecord(recordJson, mat))
            {
                data.addMaterial(mat);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Dial_:
        {
            DialRecord dial;
            if (importDialRecord(recordJson, dial))
            {
                data.addDial(dial);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Info_:
        {
            InfoRecord info;
            if (importInfoRecord(recordJson, info))
            {
                data.addInfo(info);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Land_:
        {
            LandRecord land;
            if (importLandRecord(recordJson, land))
            {
                data.addLand(land);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Soun_:
        {
            SounRecord soun;
            if (importSounRecord(recordJson, soun))
            {
                data.addSoun(soun);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Wthr_:
        {
            WthrRecord wthr;
            if (importWthrRecord(recordJson, wthr))
            {
                data.addWthr(wthr);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Ltex_:
        {
            LtexRecord ltex;
            if (importLtexRecord(recordJson, ltex))
            {
                data.addLtex(ltex);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        default:
            result.warnings.append(QString("Unsupported CSV type"));
            result.recordsSkipped++;
            break;
        }
    }
    
    return result;
}

DataImporter::ImportResult DataImporter::importFromCSV(Data& data, const QString& filePath)
{
    ImportResult result;
    result.recordsImported = 0;
    result.recordsSkipped = 0;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        result.error = QString("Cannot open file: %1").arg(filePath);
        return result;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    
    QStringList lines = in.readAll().split("\n", Qt::SkipEmptyParts);
    file.close();
    
    if (lines.isEmpty())
    {
        result.error = "Empty CSV file";
        return result;
    }
    
    CkId::Type detectedType = CkId::Type_None;
    int headerLineIndex = 0;
    
    for (int i = 0; i < lines.size(); i++)
    {
        QString line = lines[i].trimmed();
        if (line.startsWith("# Record Type:"))
        {
            QString typeStr = line.section(":", 1).trimmed();
            typeStr = typeStr.section("(", 1).section(")", 0).trimmed();
            if (typeStr.isEmpty())
            {
                typeStr = line.section(":", 1).trimmed();
            }
            detectedType = CkId::stringToType(typeStr);
            continue;
        }
        
        if (!line.startsWith("#") && !line.isEmpty())
        {
            headerLineIndex = i;
            break;
        }
    }
    
    QStringList headers = lines[headerLineIndex].split(",");
    for (int j = 0; j < headers.size(); j++)
    {
        headers[j] = headers[j].trimmed();
    }
    
    QStringList dataLines;
    for (int i = headerLineIndex + 1; i < lines.size(); i++)
    {
        QString line = lines[i].trimmed();
        if (!line.startsWith("#") && !line.isEmpty())
        {
            dataLines.append(line);
        }
    }
    
    if (detectedType != CkId::Type_None)
    {
        return importCSVByType(data, detectedType, headers, dataLines);
    }
    
    for (int i = 0; i < dataLines.size(); i++)
    {
        QStringList fields = dataLines[i].split(",");
        if (fields.size() < headers.size())
        {
            result.warnings.append(QString("Line %1: insufficient fields").arg(i + 1));
            result.recordsSkipped++;
            continue;
        }
        
        NpcRecord npc;
        for (int j = 0; j < fields.size() && j < headers.size(); j++)
        {
            if (headers[j] == "FormID")
            {
                npc.formId = parseFormId(fields[j]);
            }
            else if (headers[j] == "EditorID")
            {
                npc.editorId = fields[j];
            }
        }
        
        if (!npc.editorId.isEmpty())
        {
            data.addNpc(npc);
            result.recordsImported++;
        }
        else
        {
            result.recordsSkipped++;
        }
    }
    
    return result;
}

DataImporter::ImportResult DataImporter::importFromXML(Data& data, const QString& filePath)
{
    ImportResult result;
    result.recordsImported = 0;
    result.recordsSkipped = 0;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        result.error = QString("Cannot open file: %1").arg(filePath);
        return result;
    }
    
    QDomDocument doc;
    QString errorMsg;
    int errorLine;
    if (!doc.setContent(&file, &errorMsg, &errorLine))
    {
        result.error = QString("XML parse error at line %1: %2").arg(errorLine).arg(errorMsg);
        file.close();
        return result;
    }
    file.close();
    
    QDomElement root = doc.documentElement();
    QDomNodeList typeNodes = root.elementsByTagName("RecordType");
    
    for (int t = 0; t < typeNodes.count(); t++)
    {
        QDomElement typeElem = typeNodes.at(t).toElement();
        if (typeElem.isNull()) continue;
        
        QString typeName = typeElem.attribute("id");
        CkId::Type type = CkId::stringToType(typeName);
        if (type == CkId::Type_None) continue;
        
        QDomNodeList recordNodes = typeElem.elementsByTagName("Record");
        for (int i = 0; i < recordNodes.count(); i++)
        {
            QDomElement recordElem = recordNodes.at(i).toElement();
            if (recordElem.isNull()) continue;
            
            QJsonObject recordJson;
            QDomNamedNodeMap attrs = recordElem.attributes();
            for (int a = 0; a < attrs.count(); a++)
            {
                QDomAttr attr = attrs.item(a).toAttr();
                recordJson[attr.name()] = attr.value();
            }
            
            QDomNodeList children = recordElem.childNodes();
            for (int c = 0; c < children.count(); c++)
            {
                QDomElement child = children.at(c).toElement();
                if (!child.isNull())
                {
                    QDomNamedNodeMap childAttrs = child.attributes();
                    if (childAttrs.contains("data"))
                    {
                        recordJson[child.tagName()] = childAttrs.namedItem("data").toAttr().value();
                    }
                    else if (childAttrs.contains("value"))
                    {
                        recordJson[child.tagName()] = childAttrs.namedItem("value").toAttr().value();
                    }
                    else
                    {
                        recordJson[child.tagName()] = child.text();
                    }
                }
            }
            
        switch (type)
        {
        case CkId::Type_Npc_:
        {
            NpcRecord npc;
            if (importNpcRecord(recordJson, npc))
            {
                data.addNpc(npc);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Weap_:
        {
            WeaponRecord weap;
            if (importWeaponRecord(recordJson, weap))
            {
                data.addWeapon(weap);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Armor_:
        {
            ArmorRecord armor;
            if (importArmorRecord(recordJson, armor))
            {
                data.addArmor(armor);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Spel_:
        {
            SpellRecord spell;
            if (importSpellRecord(recordJson, spell))
            {
                data.addSpell(spell);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Quest_:
        {
            QuestRecord quest;
            if (importQuestRecord(recordJson, quest))
            {
                data.addQuest(quest);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Cel_:
        {
            CellRecord cell;
            if (importCellRecord(recordJson, cell))
            {
                data.addCell(cell);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Acti_:
        {
            ActiRecord acti;
            if (importActiRecord(recordJson, acti))
            {
                data.addActi(acti);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Book_:
        {
            BookRecord book;
            if (importBookRecord(recordJson, book))
            {
                data.addBook(book);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Misc_:
        {
            MiscRecord misc;
            if (importMiscRecord(recordJson, misc))
            {
                data.addMisc(misc);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Ingr_:
        {
            IngrRecord ingr;
            if (importIngrRecord(recordJson, ingr))
            {
                data.addIngr(ingr);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Alch_:
        {
            AlchRecord alch;
            if (importAlchRecord(recordJson, alch))
            {
                data.addAlch(alch);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Ench_:
        {
            EnchRecord ench;
            if (importEnchRecord(recordJson, ench))
            {
                data.addEnch(ench);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Cont_:
        {
            ContRecord cont;
            if (importContRecord(recordJson, cont))
            {
                data.addCont(cont);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Race_:
        {
            RaceRecord race;
            if (importRaceRecord(recordJson, race))
            {
                data.addRace(race);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_PerK_:
        {
            PerkRecord perk;
            if (importPerkRecord(recordJson, perk))
            {
                data.addPerk(perk);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Magic_:
        {
            MagicRecord magic;
            if (importMagicRecord(recordJson, magic))
            {
                data.addMagic(magic);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Pack_:
        {
            PackageRecord pack;
            if (importPackageRecord(recordJson, pack))
            {
                data.addPack(pack);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Class_:
        {
            ClassRecord cls;
            if (importClassRecord(recordJson, cls))
            {
                data.addClass(cls);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Fact_:
        {
            FactRecord fact;
            if (importFactRecord(recordJson, fact))
            {
                data.addFact(fact);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Glob_:
        {
            GlobalVariable glob;
            if (importGlobRecord(recordJson, glob))
            {
                data.addGlobVar(glob);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Tree_:
        {
            TreeRecord tree;
            if (importTreeRecord(recordJson, tree))
            {
                data.addTree(tree);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Stat_:
        {
            StatRecord stat;
            if (importStatRecord(recordJson, stat))
            {
                data.addStat(stat);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Lcrt_:
        {
            LocationRefType lcrt;
            if (importLcrtRecord(recordJson, lcrt))
            {
                data.addLcrt(lcrt);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_WRLD_:
        {
            WorldspaceRecord wrld;
            if (importWorldspaceRecord(recordJson, wrld))
            {
                data.addWorldspace(wrld);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_LOCT_:
        {
            LocationRecord loct;
            if (importLocationRecord(recordJson, loct))
            {
                data.addLocation(loct);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Refr_:
        {
            RefrRecord refr;
            if (importRefrRecord(recordJson, refr))
            {
                data.addRef(refr);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Material_:
        {
            MaterialRecord mat;
            if (importMaterialRecord(recordJson, mat))
            {
                data.addMaterial(mat);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Dial_:
        {
            DialRecord dial;
            if (importDialRecord(recordJson, dial))
            {
                data.addDial(dial);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Info_:
        {
            InfoRecord info;
            if (importInfoRecord(recordJson, info))
            {
                data.addInfo(info);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Land_:
        {
            LandRecord land;
            if (importLandRecord(recordJson, land))
            {
                data.addLand(land);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Soun_:
        {
            SounRecord soun;
            if (importSounRecord(recordJson, soun))
            {
                data.addSoun(soun);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Wthr_:
        {
            WthrRecord wthr;
            if (importWthrRecord(recordJson, wthr))
            {
                data.addWthr(wthr);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        case CkId::Type_Ltex_:
        {
            LtexRecord ltex;
            if (importLtexRecord(recordJson, ltex))
            {
                data.addLtex(ltex);
                result.recordsImported++;
            }
            else
            {
                result.recordsSkipped++;
            }
            break;
        }
        default:
            result.warnings.append(QString("Unsupported XML type: %1").arg(typeName));
            result.recordsSkipped++;
            break;
        }
        }
    }
    
    return result;
}
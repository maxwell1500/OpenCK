#include "exporttemplatesdialog.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/idcollection.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>

// ── ExportTemplate serialization ──────────────────────────────────────────────

QJsonObject ExportTemplate::toJson() const
{
    QJsonObject obj;
    obj["name"] = name;
    obj["recordType"] = recordType;
    obj["delimiter"] = delimiter;
    obj["quoteChar"] = quoteChar;
    obj["includeHeader"] = includeHeader;

    QJsonArray fieldsArr;
    for (const auto& f : fields)
    {
        QJsonObject fieldObj;
        fieldObj["fieldName"] = f.first;
        fieldObj["headerLabel"] = f.second;
        fieldsArr.append(fieldObj);
    }
    obj["fields"] = fieldsArr;
    return obj;
}

ExportTemplate ExportTemplate::fromJson(const QJsonObject& json)
{
    ExportTemplate t;
    t.name = json["name"].toString();
    t.recordType = json["recordType"].toString();
    t.delimiter = json["delimiter"].toString(",");
    t.quoteChar = json["quoteChar"].toString("\"");
    t.includeHeader = json["includeHeader"].toBool(true);

    QJsonArray fieldsArr = json["fields"].toArray();
    for (const auto& f : fieldsArr)
    {
        QJsonObject fo = f.toObject();
        t.fields.append(qMakePair(fo["fieldName"].toString(), fo["headerLabel"].toString()));
    }
    return t;
}

// ── TemplateManager ──────────────────────────────────────────────────────────

QString TemplateManager::defaultPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/export_templates.json";
}

QVector<ExportTemplate> TemplateManager::loadTemplates(const QString& path)
{
    QString filePath = path.isEmpty() ? defaultPath() : path;
    QVector<ExportTemplate> result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return result;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray())
        return result;

    QJsonArray arr = doc.array();
    for (const auto& item : arr)
    {
        if (item.isObject())
            result.append(ExportTemplate::fromJson(item.toObject()));
    }
    return result;
}

void TemplateManager::saveTemplates(const QVector<ExportTemplate>& templates, const QString& path)
{
    QString filePath = path.isEmpty() ? defaultPath() : path;

    QJsonArray arr;
    for (const auto& t : templates)
        arr.append(t.toJson());

    QFile file(filePath);
    QFileInfo fi(file);
    QDir().mkpath(fi.absolutePath());

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        LOG_WARNING(QString("Failed to save export templates to %1").arg(filePath));
        return;
    }

    file.write(QJsonDocument(arr).toJson());
    file.close();
}

static QVector<QPair<QString, QString>> fieldsForType(const QString& recordType)
{
    QVector<QPair<QString, QString>> fields;
    fields.append(qMakePair(QString("editorId"), QString("Editor ID")));
    fields.append(qMakePair(QString("formId"), QString("Form ID")));

    if (recordType == "NPC_")
    {
        fields.append(qMakePair(QString("fullName"), QString("Full Name")));
        fields.append(qMakePair(QString("level"), QString("Level")));
        fields.append(qMakePair(QString("health"), QString("Health")));
        fields.append(qMakePair(QString("magicka"), QString("Magicka")));
        fields.append(qMakePair(QString("stamina"), QString("Stamina")));
        fields.append(qMakePair(QString("attack"), QString("Attack")));
        fields.append(qMakePair(QString("defense"), QString("Defense")));
        fields.append(qMakePair(QString("race"), QString("Race")));
        fields.append(qMakePair(QString("sex"), QString("Sex")));
        fields.append(qMakePair(QString("class_"), QString("Class")));
        fields.append(qMakePair(QString("faction"), QString("Faction")));
        fields.append(qMakePair(QString("disposition"), QString("Disposition")));
        fields.append(qMakePair(QString("reputation"), QString("Reputation")));
    }
    else if (recordType == "WEAP_")
    {
        fields.append(qMakePair(QString("fullName"), QString("Full Name")));
        fields.append(qMakePair(QString("weaponType"), QString("Weapon Type")));
        fields.append(qMakePair(QString("damage"), QString("Damage")));
        fields.append(qMakePair(QString("speed"), QString("Speed")));
        fields.append(qMakePair(QString("reach"), QString("Reach")));
        fields.append(qMakePair(QString("weight"), QString("Weight")));
        fields.append(qMakePair(QString("value"), QString("Value")));
        fields.append(qMakePair(QString("enchantment"), QString("Enchantment")));
        fields.append(qMakePair(QString("magicSchool"), QString("Magic School")));
    }
    else if (recordType == "ARMOR_")
    {
        fields.append(qMakePair(QString("fullName"), QString("Full Name")));
        fields.append(qMakePair(QString("armorRating"), QString("Armor Rating")));
        fields.append(qMakePair(QString("weight"), QString("Weight")));
        fields.append(qMakePair(QString("value"), QString("Value")));
        fields.append(qMakePair(QString("health"), QString("Health")));
    }
    else if (recordType == "SPEL_")
    {
        fields.append(qMakePair(QString("fullName"), QString("Full Name")));
        fields.append(qMakePair(QString("cost"), QString("Cost")));
        fields.append(qMakePair(QString("castingSound"), QString("Casting Sound")));
        fields.append(qMakePair(QString("enchantment"), QString("Enchantment")));
    }
    else if (recordType == "BOOK_")
    {
        fields.append(qMakePair(QString("fullName"), QString("Full Name")));
        fields.append(qMakePair(QString("pageCount"), QString("Page Count")));
        fields.append(qMakePair(QString("iconPath"), QString("Icon Path")));
        fields.append(qMakePair(QString("modelPath"), QString("Model Path")));
    }
    else if (recordType == "ALCH_")
    {
        fields.append(qMakePair(QString("weight"), QString("Weight")));
        fields.append(qMakePair(QString("value"), QString("Value")));
    }
    else if (recordType == "INGR_")
    {
        fields.append(qMakePair(QString("weight"), QString("Weight")));
        fields.append(qMakePair(QString("value"), QString("Value")));
        fields.append(qMakePair(QString("flags"), QString("Flags")));
        fields.append(qMakePair(QString("iconPath"), QString("Icon Path")));
        fields.append(qMakePair(QString("modelPath"), QString("Model Path")));
    }
    else if (recordType == "ENCH_")
    {
        fields.append(qMakePair(QString("name"), QString("Name")));
        fields.append(qMakePair(QString("costLimit"), QString("Cost Limit")));
        fields.append(qMakePair(QString("charges"), QString("Charges")));
        fields.append(qMakePair(QString("type"), QString("Type")));
        fields.append(qMakePair(QString("soulGem"), QString("Soul Gem")));
    }
    else if (recordType == "CONT_")
    {
        fields.append(qMakePair(QString("weight"), QString("Weight")));
        fields.append(qMakePair(QString("value"), QString("Value")));
        fields.append(qMakePair(QString("contents"), QString("Contents")));
        fields.append(qMakePair(QString("inventoryControl"), QString("Inventory Control")));
        fields.append(qMakePair(QString("flags"), QString("Flags")));
    }
    else if (recordType == "MISC_")
    {
        fields.append(qMakePair(QString("weight"), QString("Weight")));
        fields.append(qMakePair(QString("value"), QString("Value")));
        fields.append(qMakePair(QString("flags"), QString("Flags")));
        fields.append(qMakePair(QString("iconPath"), QString("Icon Path")));
        fields.append(qMakePair(QString("modelPath"), QString("Model Path")));
    }
    else if (recordType == "ACTI_")
    {
        fields.append(qMakePair(QString("modelPath"), QString("Model Path")));
        fields.append(qMakePair(QString("iconPath"), QString("Icon Path")));
    }
    else if (recordType == "STAT_")
    {
        fields.append(qMakePair(QString("modelPath"), QString("Model Path")));
        fields.append(qMakePair(QString("lodModelPath"), QString("LOD Model Path")));
        fields.append(qMakePair(QString("iconPath"), QString("Icon Path")));
    }
    else if (recordType == "RACE_")
    {
        fields.append(qMakePair(QString("raceFlags"), QString("Race Flags")));
    }
    else if (recordType == "CLASS_")
    {
        fields.append(qMakePair(QString("className"), QString("Class Name")));
        fields.append(qMakePair(QString("description"), QString("Description")));
        fields.append(qMakePair(QString("serviceFlags"), QString("Service Flags")));
    }
    else if (recordType == "FACT_")
    {
        fields.append(qMakePair(QString("factionName"), QString("Faction Name")));
        fields.append(qMakePair(QString("description"), QString("Description")));
    }
    else if (recordType == "QUST_")
    {
        fields.append(qMakePair(QString("questName"), QString("Quest Name")));
        fields.append(qMakePair(QString("questDesc"), QString("Quest Description")));
        fields.append(qMakePair(QString("questType"), QString("Quest Type")));
    }
    else if (recordType == "DIAL_")
    {
        fields.append(qMakePair(QString("topicName"), QString("Topic Name")));
        fields.append(qMakePair(QString("flags"), QString("Flags")));
    }
    else if (recordType == "INFO_")
    {
        fields.append(qMakePair(QString("responseText"), QString("Response Text")));
        fields.append(qMakePair(QString("flags"), QString("Flags")));
    }
    else if (recordType == "PACK_")
    {
        fields.append(qMakePair(QString("packageType"), QString("Package Type")));
        fields.append(qMakePair(QString("targetType"), QString("Target Type")));
    }
    else if (recordType == "CELL")
    {
        fields.append(qMakePair(QString("cellName"), QString("Cell Name")));
        fields.append(qMakePair(QString("flags"), QString("Flags")));
        fields.append(qMakePair(QString("cellX"), QString("Cell X")));
        fields.append(qMakePair(QString("cellY"), QString("Cell Y")));
        fields.append(qMakePair(QString("owner"), QString("Owner")));
        fields.append(qMakePair(QString("lockLevel"), QString("Lock Level")));
    }
    else if (recordType == "WRLD_")
    {
        fields.append(qMakePair(QString("name"), QString("Name")));
        fields.append(qMakePair(QString("mapWidth"), QString("Map Width")));
        fields.append(qMakePair(QString("mapHeight"), QString("Map Height")));
        fields.append(qMakePair(QString("flags"), QString("Flags")));
    }
    else if (recordType == "LOCT_")
    {
        fields.append(qMakePair(QString("locationName"), QString("Location Name")));
        fields.append(qMakePair(QString("parentId"), QString("Parent ID")));
        fields.append(qMakePair(QString("x"), QString("X")));
        fields.append(qMakePair(QString("y"), QString("Y")));
        fields.append(qMakePair(QString("z"), QString("Z")));
    }
    return fields;
}

static QString quoteField(const QString& value, const QString& quoteChar, const QString& delimiter)
{
    if (value.isEmpty())
        return value;
    if (value.contains(delimiter) || value.contains(quoteChar) || value.contains('\n'))
    {
        QString escaped = value;
        escaped.replace(quoteChar, quoteChar + quoteChar);
        return quoteChar + escaped + quoteChar;
    }
    return value;
}

static QString recordFieldValue(const NpcRecord& npc, const QString& field)
{
    if (field == "editorId") return npc.editorId;
    if (field == "formId") return "0x" + QString::number(npc.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "fullName") return npc.fullName;
    if (field == "level") return QString::number(npc.level);
    if (field == "race") return QString::number(npc.race);
    if (field == "class_") return QString::number(npc.class_);
    if (field == "faction") return QString::number(npc.faction);
    if (field == "flags") return "0x" + QString::number(npc.flags, 16);
    return {};
}

static QString recordFieldValue(const WeaponRecord& wpn, const QString& field)
{
    if (field == "editorId") return wpn.editorId;
    if (field == "formId") return "0x" + QString::number(wpn.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "fullName") return wpn.fullName;
    if (field == "weaponType") return QString::number(wpn.weaponType);
    if (field == "damage") return QString::number(wpn.damage);
    if (field == "speed") return QString::number(wpn.speed);
    if (field == "reach") return QString::number(wpn.reach);
    if (field == "weight") return QString::number(wpn.weight);
    if (field == "value") return QString::number(wpn.value);
    if (field == "enchantment") return QString::number(wpn.enchantment);
    if (field == "magicSchool") return QString::number(wpn.magicSchool);
    if (field == "iconPath") return wpn.iconPath;
    if (field == "modelPath") return wpn.modelPath;
    if (field == "flags") return "0x" + QString::number(wpn.flags, 16);
    return {};
}

static QString recordFieldValue(const ArmorRecord& arm, const QString& field)
{
    if (field == "editorId") return arm.editorId;
    if (field == "formId") return "0x" + QString::number(arm.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "fullName") return arm.fullName;
    if (field == "armorRating") return QString::number(arm.armorRating);
    if (field == "weight") return QString::number(arm.weight);
    if (field == "value") return QString::number(arm.value);
    if (field == "health") return QString::number(arm.health);
    if (field == "iconPath") return arm.iconPath;
    if (field == "modelPath") return arm.modelPath;
    if (field == "flags") return "0x" + QString::number(arm.flags, 16);
    return {};
}

static QString recordFieldValue(const BookRecord& book, const QString& field)
{
    if (field == "editorId") return book.editorId;
    if (field == "formId") return "0x" + QString::number(book.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "pageCount") return QString::number(book.pageCount);
    if (field == "iconPath") return book.iconPath;
    if (field == "modelPath") return book.modelPath;
    if (field == "flags") return "0x" + QString::number(book.flags, 16);
    return {};
}

static QString recordFieldValue(const SpellRecord& spell, const QString& field)
{
    if (field == "editorId") return spell.editorId;
    if (field == "formId") return "0x" + QString::number(spell.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "fullName") return spell.fullName;
    if (field == "cost") return QString::number(spell.cost);
    if (field == "castingSound") return QString::number(spell.castingSound);
    if (field == "enchantment") return QString::number(spell.enchantment);
    if (field == "flags") return "0x" + QString::number(spell.flags, 16);
    return {};
}

static QString recordFieldValue(const AlchRecord& alch, const QString& field)
{
    if (field == "editorId") return alch.editorId;
    if (field == "formId") return "0x" + QString::number(alch.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "weight") return QString::number(alch.weight);
    if (field == "value") return QString::number(alch.value);
    if (field == "iconPath") return alch.iconPath;
    if (field == "modelPath") return alch.modelPath;
    if (field == "flags") return "0x" + QString::number(alch.flags, 16);
    return {};
}

static QString recordFieldValue(const QuestRecord& quest, const QString& field)
{
    if (field == "editorId") return quest.editorId;
    if (field == "formId") return "0x" + QString::number(quest.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "questName") return quest.questName;
    if (field == "questDesc") return quest.questDesc;
    if (field == "questType") return QString::number(quest.questType);
    if (field == "flags") return "0x" + QString::number(quest.flags, 16);
    return {};
}

static QString recordFieldValue(const DialRecord& dial, const QString& field)
{
    if (field == "editorId") return dial.editorId;
    if (field == "formId") return "0x" + QString::number(dial.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "topicName") return dial.topicName;
    if (field == "flags") return "0x" + QString::number(dial.flags, 16);
    return {};
}

static QString recordFieldValue(const PackageRecord& pack, const QString& field)
{
    if (field == "editorId") return pack.editorId;
    if (field == "formId") return "0x" + QString::number(pack.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "packageType") return QString::number(pack.packageType);
    if (field == "targetType") return QString::number(pack.targetType);
    if (field == "flags") return "0x" + QString::number(pack.flags, 16);
    return {};
}

static QString recordFieldValue(const IngrRecord& ingr, const QString& field)
{
    if (field == "editorId") return ingr.editorId;
    if (field == "formId") return "0x" + QString::number(ingr.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "weight") return QString::number(ingr.weight);
    if (field == "value") return QString::number(ingr.value);
    if (field == "flags") return "0x" + QString::number(ingr.flags, 16);
    if (field == "iconPath") return ingr.iconPath;
    if (field == "modelPath") return ingr.modelPath;
    return {};
}

static QString recordFieldValue(const EnchRecord& ench, const QString& field)
{
    if (field == "editorId") return ench.editorId;
    if (field == "formId") return "0x" + QString::number(ench.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "name") return ench.name;
    if (field == "costLimit") return QString::number(ench.costLimit);
    if (field == "charges") return QString::number(ench.charges);
    if (field == "type") return QString::number(ench.type);
    if (field == "soulGem") return QString::number(ench.soulGem);
    if (field == "flags") return "0x" + QString::number(ench.flags, 16);
    return {};
}

static QString recordFieldValue(const ContRecord& cont, const QString& field)
{
    if (field == "editorId") return cont.editorId;
    if (field == "formId") return "0x" + QString::number(cont.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "weight") return QString::number(cont.weight);
    if (field == "value") return QString::number(cont.value);
    if (field == "contents") return QString::number(cont.contents);
    if (field == "inventoryControl") return QString::number(cont.inventoryControl);
    if (field == "flags") return "0x" + QString::number(cont.flags, 16);
    return {};
}

static QString recordFieldValue(const MiscRecord& misc, const QString& field)
{
    if (field == "editorId") return misc.editorId;
    if (field == "formId") return "0x" + QString::number(misc.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "weight") return QString::number(misc.weight);
    if (field == "value") return QString::number(misc.value);
    if (field == "flags") return "0x" + QString::number(misc.flags, 16);
    if (field == "iconPath") return misc.iconPath;
    if (field == "modelPath") return misc.modelPath;
    return {};
}

static QString recordFieldValue(const ActiRecord& acti, const QString& field)
{
    if (field == "editorId") return acti.editorId;
    if (field == "formId") return "0x" + QString::number(acti.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "modelPath") return acti.modelPath;
    if (field == "iconPath") return acti.iconPath;
    if (field == "flags") return "0x" + QString::number(acti.flags, 16);
    return {};
}

static QString recordFieldValue(const StatRecord& stat, const QString& field)
{
    if (field == "editorId") return stat.editorId;
    if (field == "formId") return "0x" + QString::number(stat.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "modelPath") return stat.modelPath;
    if (field == "lodModelPath") return stat.lodModelPath;
    if (field == "iconPath") return stat.iconPath;
    if (field == "flags") return "0x" + QString::number(stat.flags, 16);
    return {};
}

static QString recordFieldValue(const RaceRecord& race, const QString& field)
{
    if (field == "editorId") return race.editorId;
    if (field == "formId") return "0x" + QString::number(race.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "raceFlags") return "0x" + QString::number(race.raceFlags, 16);
    if (field == "flags") return "0x" + QString::number(race.flags, 16);
    return {};
}

static QString recordFieldValue(const ClassRecord& cls, const QString& field)
{
    if (field == "editorId") return cls.editorId;
    if (field == "formId") return "0x" + QString::number(cls.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "className") return cls.className;
    if (field == "description") return cls.description;
    if (field == "serviceFlags") return "0x" + QString::number(cls.serviceFlags, 16);
    if (field == "flags") return "0x" + QString::number(cls.flags, 16);
    return {};
}

static QString recordFieldValue(const FactRecord& fact, const QString& field)
{
    if (field == "editorId") return fact.editorId;
    if (field == "formId") return "0x" + QString::number(fact.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "factionName") return fact.factionName;
    if (field == "description") return fact.description;
    if (field == "flags") return "0x" + QString::number(fact.flags, 16);
    return {};
}

static QString recordFieldValue(const InfoRecord& info, const QString& field)
{
    if (field == "editorId") return info.editorId;
    if (field == "formId") return "0x" + QString::number(info.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "responseText") return info.responseText;
    if (field == "voiceFile") return info.voiceFile;
    if (field == "flags") return "0x" + QString::number(info.flags, 16);
    return {};
}

static QString recordFieldValue(const CellRecord& cell, const QString& field)
{
    if (field == "editorId") return cell.editorId;
    if (field == "formId") return "0x" + QString::number(cell.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "cellName") return cell.cellName;
    if (field == "flags") return "0x" + QString::number(cell.flags, 16);
    if (field == "cellX") return QString::number(cell.cellX);
    if (field == "cellY") return QString::number(cell.cellY);
    if (field == "owner") return "0x" + QString::number(cell.owner, 16).toUpper().rightJustified(8, '0');
    if (field == "lockLevel") return QString::number(cell.lockLevel);
    return {};
}

static QString recordFieldValue(const WorldspaceRecord& wrld, const QString& field)
{
    if (field == "editorId") return wrld.editorId;
    if (field == "formId") return "0x" + QString::number(wrld.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "name") return wrld.name;
    if (field == "mapWidth") return QString::number(wrld.mapWidth);
    if (field == "mapHeight") return QString::number(wrld.mapHeight);
    if (field == "flags") return "0x" + QString::number(wrld.flags, 16);
    return {};
}

static QString recordFieldValue(const LocationRecord& loc, const QString& field)
{
    if (field == "editorId") return loc.editorId;
    if (field == "formId") return "0x" + QString::number(loc.formId, 16).toUpper().rightJustified(8, '0');
    if (field == "locationName") return loc.locationName;
    if (field == "parentId") return "0x" + QString::number(loc.parentId, 16).toUpper().rightJustified(8, '0');
    if (field == "x") return QString::number(loc.x);
    if (field == "y") return QString::number(loc.y);
    if (field == "z") return QString::number(loc.z);
    if (field == "flags") return "0x" + QString::number(loc.flags, 16);
    return {};
}

QVector<ExportTemplate> TemplateManager::builtinTemplates()
{
    QVector<ExportTemplate> builtins;

    // 1) CK Default — all common fields, comma-delimited
    ExportTemplate ckDefault;
    ckDefault.name = "CK Default";
    ckDefault.recordType = "NPC_";
    ckDefault.delimiter = ",";
    ckDefault.quoteChar = "\"";
    ckDefault.includeHeader = true;
    ckDefault.fields = fieldsForType("NPC_");
    builtins.append(ckDefault);

    // 2) Spreadsheet — key fields for quick analysis, tab-delimited
    ExportTemplate spreadsheet;
    spreadsheet.name = "Spreadsheet (NPC)";
    spreadsheet.recordType = "NPC_";
    spreadsheet.delimiter = "\t";
    spreadsheet.quoteChar = "\"";
    spreadsheet.includeHeader = true;
    spreadsheet.fields = {
        {"editorId", "Editor ID"},
        {"fullName", "Name"},
        {"level", "Level"},
        {"health", "Health"},
        {"attack", "Attack"},
        {"defense", "Defense"},
        {"race", "Race"},
        {"sex", "Sex"}
    };
    builtins.append(spreadsheet);

    // 3) Wiki Table — pipe-delimited wiki format
    ExportTemplate wikiTable;
    wikiTable.name = "Wiki Table (Weapons)";
    wikiTable.recordType = "WEAP_";
    wikiTable.delimiter = "|";
    wikiTable.quoteChar = "\"";
    wikiTable.includeHeader = true;
    wikiTable.fields = {
        {"editorId", "ID"},
        {"fullName", "Name"},
        {"damage", "Damage"},
        {"speed", "Speed"},
        {"weight", "Weight"},
        {"value", "Value"}
    };
    builtins.append(wikiTable);

    // 4) Mod Description — focused on description-relevant fields
    ExportTemplate modDesc;
    modDesc.name = "Mod Description (NPC)";
    modDesc.recordType = "NPC_";
    modDesc.delimiter = ",";
    modDesc.quoteChar = "\"";
    modDesc.includeHeader = true;
    modDesc.fields = {
        {"editorId", "Editor ID"},
        {"fullName", "Full Name"},
        {"level", "Level"},
        {"race", "Race"},
        {"sex", "Sex"}
    };
    builtins.append(modDesc);

    return builtins;
}

// ── TemplateEditDialog ───────────────────────────────────────────────────────

TemplateEditDialog::TemplateEditDialog(const ExportTemplate& existing, QWidget* parent)
    : QDialog(parent), mResult(existing)
{
    setWindowTitle(existing.name.isEmpty() ? "New Export Template" : "Edit Export Template");
    setMinimumSize(500, 500);

    auto* layout = new QVBoxLayout(this);

    // Name
    auto* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Template Name:", this));
    mNameEdit = new QLineEdit(existing.name, this);
    nameLayout->addWidget(mNameEdit);
    layout->addLayout(nameLayout);

    // Record type
    auto* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Record Type:", this));
    mRecordTypeCombo = new QComboBox(this);
    QStringList types = {"NPC_", "WEAP_", "ARMOR_", "SPEL_", "BOOK_", "ALCH_",
                         "INGR_", "ENCH_", "CONT_", "MISC_", "ACTI_", "STAT_",
                         "RACE_", "CLASS_", "FACT_", "QUST_", "DIAL_", "INFO_",
                         "PACK_", "CELL", "WRLD_", "LOCT_"};
    mRecordTypeCombo->addItems(types);
    if (!existing.recordType.isEmpty())
    {
        int idx = mRecordTypeCombo->findText(existing.recordType);
        if (idx >= 0) mRecordTypeCombo->setCurrentIndex(idx);
    }
    typeLayout->addWidget(mRecordTypeCombo);
    layout->addLayout(typeLayout);

    // Delimiter
    auto* delimLayout = new QHBoxLayout();
    delimLayout->addWidget(new QLabel("Delimiter:", this));
    mDelimiterEdit = new QLineEdit(existing.delimiter, this);
    mDelimiterEdit->setMaximumWidth(80);
    mDelimiterEdit->setToolTip(", for comma, \\t for tab, | for pipe");
    delimLayout->addWidget(mDelimiterEdit);
    delimLayout->addStretch();
    layout->addLayout(delimLayout);

    // Quote char
    auto* quoteLayout = new QHBoxLayout();
    quoteLayout->addWidget(new QLabel("Quote Character:", this));
    mQuoteCharEdit = new QLineEdit(existing.quoteChar, this);
    mQuoteCharEdit->setMaximumWidth(80);
    quoteLayout->addWidget(mQuoteCharEdit);
    quoteLayout->addStretch();
    layout->addLayout(quoteLayout);

    // Include header
    mIncludeHeaderCheck = new QCheckBox("Include header row", this);
    mIncludeHeaderCheck->setChecked(existing.includeHeader);
    layout->addWidget(mIncludeHeaderCheck);

    // Fields group
    auto* fieldGroup = new QGroupBox("Fields to Export", this);
    auto* fieldLayout = new QVBoxLayout(fieldGroup);

    auto* btnRow = new QHBoxLayout();
    auto* selectAllBtn = new QPushButton("Select All", this);
    auto* deselectAllBtn = new QPushButton("Deselect All", this);
    btnRow->addWidget(selectAllBtn);
    btnRow->addWidget(deselectAllBtn);
    btnRow->addStretch();
    fieldLayout->addLayout(btnRow);

    mFieldList = new QListWidget(this);
    fieldLayout->addWidget(mFieldList);
    layout->addWidget(fieldGroup, 1);

    connect(selectAllBtn, &QPushButton::clicked, this, &TemplateEditDialog::onSelectAll);
    connect(deselectAllBtn, &QPushButton::clicked, this, &TemplateEditDialog::onDeselectAll);
    connect(mRecordTypeCombo, &QComboBox::currentTextChanged, this, &TemplateEditDialog::onRecordTypeChanged);

    // Populate fields
    populateFields(mRecordTypeCombo->currentText());

    // Buttons
    auto* btnBox = new QHBoxLayout();
    auto* okBtn = new QPushButton("OK", this);
    auto* cancelBtn = new QPushButton("Cancel", this);
    btnBox->addStretch();
    btnBox->addWidget(okBtn);
    btnBox->addWidget(cancelBtn);
    layout->addLayout(btnBox);

    connect(okBtn, &QPushButton::clicked, this, [this]() {
        mResult.name = mNameEdit->text().trimmed();
        mResult.recordType = mRecordTypeCombo->currentText();
        QString delim = mDelimiterEdit->text();
        if (delim == "\\t") delim = "\t";
        mResult.delimiter = delim;
        mResult.quoteChar = mQuoteCharEdit->text();
        mResult.includeHeader = mIncludeHeaderCheck->isChecked();
        mResult.fields.clear();
        for (int i = 0; i < mFieldList->count(); ++i)
        {
            QListWidgetItem* item = mFieldList->item(i);
            if (item->checkState() == Qt::Checked)
            {
                QPair<QString, QString> field = item->data(Qt::UserRole).value<QPair<QString, QString>>();
                mResult.fields.append(field);
            }
        }
        accept();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

ExportTemplate TemplateEditDialog::result() const
{
    return mResult;
}

void TemplateEditDialog::onRecordTypeChanged(const QString& type)
{
    populateFields(type);
}

void TemplateEditDialog::onSelectAll()
{
    for (int i = 0; i < mFieldList->count(); ++i)
        mFieldList->item(i)->setCheckState(Qt::Checked);
}

void TemplateEditDialog::onDeselectAll()
{
    for (int i = 0; i < mFieldList->count(); ++i)
        mFieldList->item(i)->setCheckState(Qt::Unchecked);
}

void TemplateEditDialog::populateFields(const QString& recordType)
{
    mFieldList->clear();
    QVector<QPair<QString, QString>> availFields = fieldsForType(recordType);

    // Determine which fields were previously selected
    QSet<QString> selectedFields;
    for (const auto& f : mResult.fields)
        selectedFields.insert(f.first);

    for (const auto& field : availFields)
    {
        auto* item = new QListWidgetItem(field.second, mFieldList);
        item->setData(Qt::UserRole, QVariant::fromValue(field));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        bool checked = mResult.recordType == recordType
            ? selectedFields.contains(field.first)
            : true;
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }
}

// ── ExportTemplatesDialog ────────────────────────────────────────────────────

ExportTemplatesDialog::ExportTemplatesDialog(Data* data, QWidget* parent)
    : QDialog(parent), mData(data)
{
    setWindowTitle("Export Templates");
    setMinimumSize(550, 450);

    auto* layout = new QVBoxLayout(this);

    auto* label = new QLabel("Manage and use custom CSV export templates:", this);
    layout->addWidget(label);

    mTemplateList = new QListWidget(this);
    mTemplateList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(mTemplateList, 1);

    connect(mTemplateList, &QListWidget::itemSelectionChanged, this, &ExportTemplatesDialog::onTemplateSelected);
    connect(mTemplateList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        onEditClicked();
    });

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* newBtn = new QPushButton("New...", this);
    mEditBtn = new QPushButton("Edit...", this);
    mDeleteBtn = new QPushButton("Delete", this);
    mExportBtn = new QPushButton("Export", this);
    auto* closeBtn = new QPushButton("Close", this);

    mEditBtn->setEnabled(false);
    mDeleteBtn->setEnabled(false);
    mExportBtn->setEnabled(false);

    btnLayout->addWidget(newBtn);
    btnLayout->addWidget(mEditBtn);
    btnLayout->addWidget(mDeleteBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(mExportBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    connect(newBtn, &QPushButton::clicked, this, &ExportTemplatesDialog::onNewClicked);
    connect(mEditBtn, &QPushButton::clicked, this, &ExportTemplatesDialog::onEditClicked);
    connect(mDeleteBtn, &QPushButton::clicked, this, &ExportTemplatesDialog::onDeleteClicked);
    connect(mExportBtn, &QPushButton::clicked, this, &ExportTemplatesDialog::onExportClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    // Load templates: builtins + saved
    QVector<ExportTemplate> builtins = TemplateManager::builtinTemplates();
    QVector<ExportTemplate> saved = TemplateManager::loadTemplates();

    // Merge: builtins first, then saved (skip duplicates by name)
    QSet<QString> names;
    for (const auto& t : builtins)
    {
        mTemplates.append(t);
        names.insert(t.name);
    }
    for (const auto& t : saved)
    {
        if (!names.contains(t.name))
        {
            mTemplates.append(t);
            names.insert(t.name);
        }
    }

    refreshList();
}

void ExportTemplatesDialog::refreshList()
{
    mTemplateList->clear();
    for (int i = 0; i < mTemplates.size(); ++i)
    {
        const ExportTemplate& t = mTemplates[i];
        QString label = QString("%1 [%2] (%3 fields)")
                            .arg(t.name)
                            .arg(t.recordType)
                            .arg(t.fields.size());
        auto* item = new QListWidgetItem(label, mTemplateList);
        item->setData(Qt::UserRole, i);
    }
}

void ExportTemplatesDialog::onTemplateSelected()
{
    bool hasSelection = !mTemplateList->selectedItems().isEmpty();
    mEditBtn->setEnabled(hasSelection);
    mDeleteBtn->setEnabled(hasSelection);
    mExportBtn->setEnabled(hasSelection);
}

void ExportTemplatesDialog::onNewClicked()
{
    TemplateEditDialog dlg(ExportTemplate(), this);
    if (dlg.exec() == QDialog::Accepted)
    {
        ExportTemplate t = dlg.result();
        if (t.name.isEmpty())
        {
            QMessageBox::warning(this, "Template", "Template name cannot be empty.");
            return;
        }
        mTemplates.append(t);
        TemplateManager::saveTemplates(mTemplates);
        refreshList();
    }
}

void ExportTemplatesDialog::onEditClicked()
{
    QListWidgetItem* item = mTemplateList->currentItem();
    if (!item) return;

    int idx = item->data(Qt::UserRole).toInt();
    TemplateEditDialog dlg(mTemplates[idx], this);
    if (dlg.exec() == QDialog::Accepted)
    {
        mTemplates[idx] = dlg.result();
        TemplateManager::saveTemplates(mTemplates);
        refreshList();
    }
}

void ExportTemplatesDialog::onDeleteClicked()
{
    QListWidgetItem* item = mTemplateList->currentItem();
    if (!item) return;

    int idx = item->data(Qt::UserRole).toInt();
    int ret = QMessageBox::question(this, "Delete Template",
        QString("Delete template '%1'?").arg(mTemplates[idx].name));
    if (ret == QMessageBox::Yes)
    {
        mTemplates.removeAt(idx);
        TemplateManager::saveTemplates(mTemplates);
        refreshList();
    }
}

void ExportTemplatesDialog::onExportClicked()
{
    QListWidgetItem* item = mTemplateList->currentItem();
    if (!item) return;

    int idx = item->data(Qt::UserRole).toInt();
    exportWithTemplate(mTemplates[idx]);
}

template <typename CollectionT>
void exportRecords(QTextStream& out, const ExportTemplate& tmplt,
                   const CollectionT& coll, int& count)
{
    const QString& delim = tmplt.delimiter;
    const QString& quote = tmplt.quoteChar;
    for (int i = 0; i < coll.size(); ++i)
    {
        const auto& rec = coll.getRecord(i).get();
        QStringList values;
        for (const auto& f : tmplt.fields)
            values.append(quoteField(recordFieldValue(rec, f.first), quote, delim));
        out << values.join(delim) << "\n";
        count++;
    }
}

void ExportTemplatesDialog::exportWithTemplate(const ExportTemplate& tmplt)
{
    if (!mData)
    {
        QMessageBox::information(this, "Export", "No data loaded.");
        return;
    }

    if (tmplt.fields.isEmpty())
    {
        QMessageBox::warning(this, "Export", "Template has no fields selected.");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "Export CSV", "",
        "CSV Files (*.csv);;All Files (*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "Export Error", QString("Cannot write to file: %1").arg(filePath));
        return;
    }

    QTextStream out(&file);
    const QString& delim = tmplt.delimiter;
    const QString& quote = tmplt.quoteChar;

    // Header row
    if (tmplt.includeHeader)
    {
        QStringList headers;
        for (const auto& f : tmplt.fields)
            headers.append(quoteField(f.second, quote, delim));
        out << headers.join(delim) << "\n";
    }

    int count = 0;

    if (tmplt.recordType == "NPC_")       exportRecords(out, tmplt, mData->getNpcCollection(), count);
    else if (tmplt.recordType == "WEAP_") exportRecords(out, tmplt, mData->getWeaponCollection(), count);
    else if (tmplt.recordType == "ARMOR_") exportRecords(out, tmplt, mData->getArmorCollection(), count);
    else if (tmplt.recordType == "SPEL_") exportRecords(out, tmplt, mData->getSpellCollection(), count);
    else if (tmplt.recordType == "BOOK_") exportRecords(out, tmplt, mData->getBookCollection(), count);
    else if (tmplt.recordType == "ALCH_") exportRecords(out, tmplt, mData->getAlchCollection(), count);
    else if (tmplt.recordType == "INGR_") exportRecords(out, tmplt, mData->getIngrCollection(), count);
    else if (tmplt.recordType == "ENCH_") exportRecords(out, tmplt, mData->getEnchCollection(), count);
    else if (tmplt.recordType == "CONT_") exportRecords(out, tmplt, mData->getContCollection(), count);
    else if (tmplt.recordType == "MISC_") exportRecords(out, tmplt, mData->getMiscCollection(), count);
    else if (tmplt.recordType == "ACTI_") exportRecords(out, tmplt, mData->getActiCollection(), count);
    else if (tmplt.recordType == "STAT_") exportRecords(out, tmplt, mData->getStatCollection(), count);
    else if (tmplt.recordType == "RACE_") exportRecords(out, tmplt, mData->getRaceCollection(), count);
    else if (tmplt.recordType == "CLASS_") exportRecords(out, tmplt, mData->getClassCollection(), count);
    else if (tmplt.recordType == "FACT_") exportRecords(out, tmplt, mData->getFactCollection(), count);
    else if (tmplt.recordType == "QUST_") exportRecords(out, tmplt, mData->getQuestCollection(), count);
    else if (tmplt.recordType == "DIAL_") exportRecords(out, tmplt, mData->getDialCollection(), count);
    else if (tmplt.recordType == "INFO_") exportRecords(out, tmplt, mData->getInfoCollection(), count);
    else if (tmplt.recordType == "PACK_") exportRecords(out, tmplt, mData->getPackCollection(), count);
    else if (tmplt.recordType == "CELL")  exportRecords(out, tmplt, mData->getCellCollection(), count);
    else if (tmplt.recordType == "WRLD_") exportRecords(out, tmplt, mData->getWorldspaceCollection(), count);
    else if (tmplt.recordType == "LOCT_") exportRecords(out, tmplt, mData->getLocationCollection(), count);
    else
    {
        QMessageBox::information(this, "Export",
            QString("Export for record type '%1' is not yet supported.").arg(tmplt.recordType));
        file.close();
        return;
    }

    file.close();
    QMessageBox::information(this, "Export Complete",
        QString("Exported %1 records to:\n%2").arg(count).arg(filePath));
    LOG_INFO(QString("Template export: %1 records (%2) to %3")
                 .arg(count).arg(tmplt.recordType).arg(filePath));
}

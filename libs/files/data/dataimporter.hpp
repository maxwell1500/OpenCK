#ifndef DATAIMPORTER_H
#define DATAIMPORTER_H

#include "../../src/model/world/data.hpp"
#include "../../src/model/world/ckid.hpp"

#include <QString>
#include <QList>
#include <QVariantMap>
#include <QJsonObject>

class DataImporter
{
public:
    struct ImportResult {
        int recordsImported;
        int recordsSkipped;
        QString error;
        QStringList warnings;
    };
    
    /// \brief Import records from a JSON file
    /// \param data Target data object to populate with imported records
    /// \param filePath Path to JSON file (format: {"records": [...], "metadata": {...}})
    /// \return ImportResult with counts of imported/skipped records and any errors
    static ImportResult importFromJSON(Data& data, const QString& filePath);
    
    /// \brief Import records from a CSV file
    /// \param data Target data object to populate with imported records
    /// \param filePath Path to CSV file (first row must be headers)
    /// \return ImportResult with counts of imported/skipped records and any errors
    static ImportResult importFromCSV(Data& data, const QString& filePath);
    
    /// \brief Import records from an XML file
    /// \param data Target data object to populate with imported records
    /// \param filePath Path to XML file (format: <records><record type="NPC_" .../></records>)
    /// \return ImportResult with counts of imported/skipped records and any errors
    static ImportResult importFromXML(Data& data, const QString& filePath);
    
    /// \brief Import an NPC record from JSON
    /// \param json JSON object with NPC_ record fields
    /// \param record Output NpcRecord to populate
    /// \return true if import successful
    static bool importNpcRecord(const QJsonObject& json, NpcRecord& record);
    
    /// \brief Import a weapon record from JSON
    /// \param json JSON object with WEAP record fields
    /// \param record Output WeaponRecord to populate
    /// \return true if import successful
    static bool importWeaponRecord(const QJsonObject& json, WeaponRecord& record);
    
    /// \brief Import an armor record from JSON
    /// \param json JSON object with ARMOR record fields
    /// \param record Output ArmorRecord to populate
    /// \return true if import successful
    static bool importArmorRecord(const QJsonObject& json, ArmorRecord& record);
    
    /// \brief Import a spell record from JSON
    /// \param json JSON object with SPEL record fields
    /// \param record Output SpellRecord to populate
    /// \return true if import successful
    static bool importSpellRecord(const QJsonObject& json, SpellRecord& record);
    
    /// \brief Import a quest record from JSON
    /// \param json JSON object with QUEST record fields
    /// \param record Output QuestRecord to populate
    /// \return true if import successful
    static bool importQuestRecord(const QJsonObject& json, QuestRecord& record);
    
    /// \brief Import a cell record from JSON
    /// \param json JSON object with CELL record fields
    /// \param record Output CellRecord to populate
    /// \return true if import successful
    static bool importCellRecord(const QJsonObject& json, CellRecord& record);
    
    /// \brief Import an activator record from JSON
    /// \param json JSON object with ACTI record fields
    /// \param record Output ActiRecord to populate
    /// \return true if import successful
    static bool importActiRecord(const QJsonObject& json, ActiRecord& record);
    
    /// \brief Import a book record from JSON
    /// \param json JSON object with BOOK record fields
    /// \param record Output BookRecord to populate
    /// \return true if import successful
    static bool importBookRecord(const QJsonObject& json, BookRecord& record);
    
    /// \brief Import a misc item record from JSON
    /// \param json JSON object with MISC record fields
    /// \param record Output MiscRecord to populate
    /// \return true if import successful
    static bool importMiscRecord(const QJsonObject& json, MiscRecord& record);
    
    /// \brief Import an ingredient record from JSON
    /// \param json JSON object with INGR record fields
    /// \param record Output IngrRecord to populate
    /// \return true if import successful
    static bool importIngrRecord(const QJsonObject& json, IngrRecord& record);
    
    /// \brief Import an alchemy record from JSON
    /// \param json JSON object with ALCH record fields
    /// \param record Output AlchRecord to populate
    /// \return true if import successful
    static bool importAlchRecord(const QJsonObject& json, AlchRecord& record);
    
    /// \brief Import an enchantment record from JSON
    /// \param json JSON object with ENCH record fields
    /// \param record Output EnchRecord to populate
    /// \return true if import successful
    static bool importEnchRecord(const QJsonObject& json, EnchRecord& record);
    
    /// \brief Import a container record from JSON
    /// \param json JSON object with CONT record fields
    /// \param record Output ContRecord to populate
    /// \return true if import successful
    static bool importContRecord(const QJsonObject& json, ContRecord& record);
    
    /// \brief Import a race record from JSON
    /// \param json JSON object with RACE record fields
    /// \param record Output RaceRecord to populate
    /// \return true if import successful
    static bool importRaceRecord(const QJsonObject& json, RaceRecord& record);
    
    /// \brief Import a perk record from JSON
    /// \param json JSON object with PERK record fields
    /// \param record Output PerkRecord to populate
    /// \return true if import successful
    static bool importPerkRecord(const QJsonObject& json, PerkRecord& record);
    
    /// \brief Import a magic effect record from JSON
    /// \param json JSON object with MAGIC record fields
    /// \param record Output MagicRecord to populate
    /// \return true if import successful
    static bool importMagicRecord(const QJsonObject& json, MagicRecord& record);
    
    /// \brief Import a package record from JSON
    /// \param json JSON object with PACK record fields
    /// \param record Output PackageRecord to populate
    /// \return true if import successful
    static bool importPackageRecord(const QJsonObject& json, PackageRecord& record);
    
    /// \brief Import a class record from JSON
    /// \param json JSON object with CLASS record fields
    /// \param record Output ClassRecord to populate
    /// \return true if import successful
    static bool importClassRecord(const QJsonObject& json, ClassRecord& record);
    
    /// \brief Import a faction record from JSON
    /// \param json JSON object with FACT record fields
    /// \param record Output FactRecord to populate
    /// \return true if import successful
    static bool importFactRecord(const QJsonObject& json, FactRecord& record);
    
    /// \brief Import a global variable record from JSON
    /// \param json JSON object with GLOB record fields
    /// \param record Output GlobalVariable to populate
    /// \return true if import successful
    static bool importGlobRecord(const QJsonObject& json, GlobalVariable& record);
    
    /// \brief Import a tree record from JSON
    /// \param json JSON object with TREE record fields
    /// \param record Output TreeRecord to populate
    /// \return true if import successful
    static bool importTreeRecord(const QJsonObject& json, TreeRecord& record);
    
    /// \brief Import a static record from JSON
    /// \param json JSON object with STAT record fields
    /// \param record Output StatRecord to populate
    /// \return true if import successful
    static bool importStatRecord(const QJsonObject& json, StatRecord& record);
    
    /// \brief Import a location reference type record from JSON
    /// \param json JSON object with LCRT record fields
    /// \param record Output LocationRefType to populate
    /// \return true if import successful
    static bool importLcrtRecord(const QJsonObject& json, LocationRefType& record);
    
    /// \brief Import a worldspace record from JSON
    /// \param json JSON object with WRLD record fields
    /// \param record Output WorldspaceRecord to populate
    /// \return true if import successful
    static bool importWorldspaceRecord(const QJsonObject& json, WorldspaceRecord& record);
    
    /// \brief Import a location record from JSON
    /// \param json JSON object with LCTN record fields
    /// \param record Output LocationRecord to populate
    /// \return true if import successful
    static bool importLocationRecord(const QJsonObject& json, LocationRecord& record);
    
    /// \brief Import a reference record from JSON
    /// \param json JSON object with REFR record fields
    /// \param record Output RefrRecord to populate
    /// \return true if import successful
    static bool importRefrRecord(const QJsonObject& json, RefrRecord& record);
    
    /// \brief Import a material record from JSON
    /// \param json JSON object with MATL record fields
    /// \param record Output MaterialRecord to populate
    /// \return true if import successful
    static bool importMaterialRecord(const QJsonObject& json, MaterialRecord& record);
    
    /// \brief Import a dialogue record from JSON
    /// \param json JSON object with DIAL record fields
    /// \param record Output DialRecord to populate
    /// \return true if import successful
    static bool importDialRecord(const QJsonObject& json, DialRecord& record);
    
    /// \brief Import a dialogue info record from JSON
    /// \param json JSON object with INFO record fields
    /// \param record Output InfoRecord to populate
    /// \return true if import successful
    static bool importInfoRecord(const QJsonObject& json, InfoRecord& record);
    
    /// \brief Import a landscape record from JSON
    /// \param json JSON object with LAND record fields
    /// \param record Output LandRecord to populate
    /// \return true if import successful
    static bool importLandRecord(const QJsonObject& json, LandRecord& record);
    
    /// \brief Import a sound record from JSON
    /// \param json JSON object with SOUN record fields
    /// \param record Output SounRecord to populate
    /// \return true if import successful
    static bool importSounRecord(const QJsonObject& json, SounRecord& record);
    
    /// \brief Import a weather record from JSON
    /// \param json JSON object with WTHR record fields
    /// \param record Output WthrRecord to populate
    /// \return true if import successful
    static bool importWthrRecord(const QJsonObject& json, WthrRecord& record);
    
    /// \brief Import a land texture record from JSON
    /// \param json JSON object with LTEX record fields
    /// \param record Output LtexRecord to populate
    /// \return true if import successful
    static bool importLtexRecord(const QJsonObject& json, LtexRecord& record);

private:
    static quint32 parseFormId(const QString& formIdStr);
    static State parseState(const QString& stateStr);
    static QString extractField(const QJsonObject& obj, const QString& key);
    static int extractIntField(const QJsonObject& obj, const QString& key, int defaultValue = 0);
    static quint32 extractUInt32Field(const QJsonObject& obj, const QString& key, quint32 defaultValue = 0);
    static float extractFloatField(const QJsonObject& obj, const QString& key, float defaultValue = 0.0f);
    static bool extractBoolField(const QJsonObject& obj, const QString& key, bool defaultValue = false);
    static QVector<quint32> extractIntVector(const QJsonObject& obj, const QString& key);
    static QVector<QString> extractStringVector(const QJsonObject& obj, const QString& key);
    static ImportResult importCSVByType(Data& data, CkId::Type type, const QStringList& headers, const QStringList& lines);
};

#endif // DATAIMPORTER_H

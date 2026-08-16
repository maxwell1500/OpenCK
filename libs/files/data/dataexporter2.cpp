#include "dataexporter.hpp"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QDomElement>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>
#include <QByteArray>

QString DataExporter::getEditorId(const BaseRecord& record)
{
    if (auto* r = dynamic_cast<const Record<NpcRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<WeaponRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<ArmorRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<SpellRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<MagicRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<QuestRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<DialRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<InfoRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<GlobalVariable>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<LocationRefType>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<PackageRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<TreeRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<AlchRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<IngrRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<ContRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<EnchRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<BookRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<MiscRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<ActiRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<StatRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<RaceRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<ClassRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<FactRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<PerkRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<CellRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<WorldspaceRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<LocationRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<RefrRecord>*>(&record)) return QString();
    if (auto* r = dynamic_cast<const Record<MaterialRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<LandRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<SounRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<WthrRecord>*>(&record)) return r->get().editorId;
    if (auto* r = dynamic_cast<const Record<LtexRecord>*>(&record)) return r->get().editorId;
    return QString();
}

quint32 DataExporter::getFormId(const BaseRecord& record)
{
    if (auto* r = dynamic_cast<const Record<NpcRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<WeaponRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<ArmorRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<SpellRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<MagicRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<QuestRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<DialRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<InfoRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<PackageRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<TreeRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<AlchRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<IngrRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<ContRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<EnchRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<BookRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<MiscRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<ActiRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<StatRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<RaceRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<ClassRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<FactRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<PerkRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<CellRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<WorldspaceRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<LocationRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<RefrRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<MaterialRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<LandRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<SounRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<WthrRecord>*>(&record)) return r->get().formId;
    if (auto* r = dynamic_cast<const Record<LtexRecord>*>(&record)) return r->get().formId;
    return 0;
}

QString DataExporter::getStateString(State state)
{
    switch (state)
    {
    case State_Base: return "Base";
    case State_Modified: return "Modified";
    case State_ModifiedOnly: return "ModifiedOnly";
    case State_Deleted: return "Deleted";
    case State_Erased: return "Erased";
    }
    return "Unknown";
}

bool DataExporter::matchesFilter(const BaseRecord& record, const ExportFilter& filter)
{
    if (filter.onlyModified && filter.onlyDeleted)
    {
        if (record.state != State_Modified && record.state != State_ModifiedOnly && record.state != State_Deleted)
            return false;
    }
    else if (filter.onlyModified)
    {
        if (record.state != State_Modified && record.state != State_ModifiedOnly)
            return false;
    }
    else if (filter.onlyDeleted)
    {
        if (record.state != State_Deleted)
            return false;
    }

    if (!filter.editorIdPattern.isEmpty())
    {
        static QRegularExpression regex;
        static QString lastPattern;
        if (filter.editorIdPattern != lastPattern)
        {
            regex = QRegularExpression(filter.editorIdPattern);
            lastPattern = filter.editorIdPattern;
        }
        QString editorId = getEditorId(record);
        if (!regex.match(editorId).hasMatch())
            return false;
    }

    quint32 formId = getFormId(record);
    if (formId < filter.formIdMin || formId > filter.formIdMax)
        return false;

    return true;
}

DataExporter::ExportResult DataExporter::exportToJSON(const Data& data, const QStringList& recordTypes, const QString& outputPath, const ExportFilter& filter)
{
    ExportResult result;
    result.recordsExported = 0;
    
    QJsonObject root;
    root["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["format"] = "OpenCK JSON Export";
    root["version"] = "1.0";
    
    QJsonArray typeArray;
    
    for (const QString& typeStr : recordTypes)
    {
        CkId::Type type = CkId::stringToType(typeStr);
        if (type == CkId::Type_None) continue;
        
        QJsonObject typeObj;
        typeObj["type"] = typeStr;
        typeObj["typeName"] = CkId(type).getTypeName();
        
        QJsonArray records;
        
        int typeCount = 0;
        auto addCollection = [&](const BaseCollection& collection) {
            typeCount += exportCollectionToJSON(collection, type, records, filter);
        };
        
        switch (type)
        {
        case CkId::Type_Npc_:      addCollection(data.getNpcCollection()); break;
        case CkId::Type_Weap_:     addCollection(data.getWeaponCollection()); break;
        case CkId::Type_Armor_:    addCollection(data.getArmorCollection()); break;
        case CkId::Type_Spel_:     addCollection(data.getSpellCollection()); break;
        case CkId::Type_Magic_:    addCollection(data.getMagicCollection()); break;
        case CkId::Type_Quest_:    addCollection(data.getQuestCollection()); break;
        case CkId::Type_Dial_:     addCollection(data.getDialCollection()); break;
        case CkId::Type_Info_:     addCollection(data.getInfoCollection()); break;
        case CkId::Type_Glob_:     addCollection(data.getGlobCollection()); break;
        case CkId::Type_Lcrt_:     addCollection(data.getLcrtCollection()); break;
        case CkId::Type_Pack_:     addCollection(data.getPackCollection()); break;
        case CkId::Type_Tree_:     addCollection(data.getTreeCollection()); break;
        case CkId::Type_Alch_:     addCollection(data.getAlchCollection()); break;
        case CkId::Type_Ingr_:     addCollection(data.getIngrCollection()); break;
        case CkId::Type_Cont_:     addCollection(data.getContCollection()); break;
        case CkId::Type_Ench_:     addCollection(data.getEnchCollection()); break;
        case CkId::Type_Book_:     addCollection(data.getBookCollection()); break;
        case CkId::Type_Misc_:     addCollection(data.getMiscCollection()); break;
        case CkId::Type_Acti_:     addCollection(data.getActiCollection()); break;
        case CkId::Type_Stat_:     addCollection(data.getStatCollection()); break;
        case CkId::Type_Race_:     addCollection(data.getRaceCollection()); break;
        case CkId::Type_Class_:    addCollection(data.getClassCollection()); break;
        case CkId::Type_Fact_:     addCollection(data.getFactCollection()); break;
        case CkId::Type_PerK_:     addCollection(data.getPerkCollection()); break;
        case CkId::Type_Cel_:      addCollection(data.getCellCollection()); break;
        case CkId::Type_WRLD_:     addCollection(data.getWorldspaceCollection()); break;
        case CkId::Type_LOCT_:     addCollection(data.getLocationCollection()); break;
        case CkId::Type_Refr_:     addCollection(data.getRefrCollection()); break;
        case CkId::Type_Material_: addCollection(data.getMaterialCollection()); break;
        case CkId::Type_Land_:    addCollection(data.getLandCollection()); break;
        case CkId::Type_Soun_:     addCollection(data.getSounCollection()); break;
        case CkId::Type_Wthr_:     addCollection(data.getWthrCollection()); break;
        case CkId::Type_Ltex_:     addCollection(data.getLtexCollection()); break;
        default: break;
        }
        
        result.recordsExported += typeCount;
        if (typeCount > 0)
            result.recordsByType[typeStr] = typeCount;
        
        typeObj["records"] = records;
        typeArray.append(typeObj);
    }
    
    root["records"] = typeArray;
    
    QJsonDocument doc(root);
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        result.error = QString("Cannot write to %1").arg(outputPath);
        return result;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    QFileInfo fileInfo(outputPath);
    result.fileSize = fileInfo.size();
    result.outputPath = outputPath;
    return result;
}

DataExporter::ExportResult DataExporter::exportToCSV(const Data& data, const QStringList& recordTypes, const QString& outputPath, const ExportFilter& filter)
{
    ExportResult result;
    result.recordsExported = 0;
    
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        result.error = QString("Cannot write to %1").arg(outputPath);
        return result;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    bool firstFile = true;
    
    for (const QString& typeStr : recordTypes)
    {
        CkId::Type type = CkId::stringToType(typeStr);
        if (type == CkId::Type_None) continue;
        
        if (!firstFile)
        {
            out << "\n\n";
        }
        firstFile = false;
        
        out << QString("# Record Type: %1 (%2)\n\n").arg(CkId(type).getTypeName(), typeStr);
        
        int typeCount = 0;
        auto addCollection = [&](const BaseCollection& collection) {
            QString csvContent;
            QStringList headers;
            typeCount += exportCollectionToCSV(collection, type, csvContent, headers, filter);
            out << headers.join(",") << "\n";
            out << csvContent;
        };
        
        switch (type)
        {
        case CkId::Type_Npc_:      addCollection(data.getNpcCollection()); break;
        case CkId::Type_Weap_:     addCollection(data.getWeaponCollection()); break;
        case CkId::Type_Armor_:    addCollection(data.getArmorCollection()); break;
        case CkId::Type_Spel_:     addCollection(data.getSpellCollection()); break;
        case CkId::Type_Magic_:    addCollection(data.getMagicCollection()); break;
        case CkId::Type_Quest_:    addCollection(data.getQuestCollection()); break;
        case CkId::Type_Dial_:     addCollection(data.getDialCollection()); break;
        case CkId::Type_Info_:     addCollection(data.getInfoCollection()); break;
        case CkId::Type_Glob_:     addCollection(data.getGlobCollection()); break;
        case CkId::Type_Lcrt_:     addCollection(data.getLcrtCollection()); break;
        case CkId::Type_Pack_:     addCollection(data.getPackCollection()); break;
        case CkId::Type_Tree_:     addCollection(data.getTreeCollection()); break;
        case CkId::Type_Alch_:     addCollection(data.getAlchCollection()); break;
        case CkId::Type_Ingr_:     addCollection(data.getIngrCollection()); break;
        case CkId::Type_Cont_:     addCollection(data.getContCollection()); break;
        case CkId::Type_Ench_:     addCollection(data.getEnchCollection()); break;
        case CkId::Type_Book_:     addCollection(data.getBookCollection()); break;
        case CkId::Type_Misc_:     addCollection(data.getMiscCollection()); break;
        case CkId::Type_Acti_:     addCollection(data.getActiCollection()); break;
        case CkId::Type_Stat_:     addCollection(data.getStatCollection()); break;
        case CkId::Type_Race_:     addCollection(data.getRaceCollection()); break;
        case CkId::Type_Class_:    addCollection(data.getClassCollection()); break;
        case CkId::Type_Fact_:     addCollection(data.getFactCollection()); break;
        case CkId::Type_PerK_:     addCollection(data.getPerkCollection()); break;
        case CkId::Type_Cel_:      addCollection(data.getCellCollection()); break;
        case CkId::Type_WRLD_:     addCollection(data.getWorldspaceCollection()); break;
        case CkId::Type_LOCT_:     addCollection(data.getLocationCollection()); break;
        case CkId::Type_Refr_:     addCollection(data.getRefrCollection()); break;
        case CkId::Type_Material_: addCollection(data.getMaterialCollection()); break;
        case CkId::Type_Land_:    addCollection(data.getLandCollection()); break;
        case CkId::Type_Soun_:     addCollection(data.getSounCollection()); break;
        case CkId::Type_Wthr_:     addCollection(data.getWthrCollection()); break;
        case CkId::Type_Ltex_:     addCollection(data.getLtexCollection()); break;
        default: break;
        }
        
        result.recordsExported += typeCount;
        if (typeCount > 0)
            result.recordsByType[typeStr] = typeCount;
    }
    
    file.close();
    QFileInfo fileInfo(outputPath);
    result.fileSize = fileInfo.size();
    result.outputPath = outputPath;
    return result;
}

DataExporter::ExportResult DataExporter::exportToXML(const Data& data, const QStringList& recordTypes, const QString& outputPath, const ExportFilter& filter)
{
    ExportResult result;
    result.recordsExported = 0;
    
    QDomDocument doc;
    QDomProcessingInstruction instruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);
    
    QDomElement root = doc.createElement("OpenCKExport");
    root.setAttribute("date", QDateTime::currentDateTime().toString(Qt::ISODate));
    root.setAttribute("format", "OpenCK XML Export");
    root.setAttribute("version", "1.0");
    doc.appendChild(root);
    
    for (const QString& typeStr : recordTypes)
    {
        CkId::Type type = CkId::stringToType(typeStr);
        if (type == CkId::Type_None) continue;
        
        int typeCount = 0;
        auto addCollection = [&](const BaseCollection& collection) {
            QDomElement typeElem = doc.createElement("RecordType");
            typeElem.setAttribute("id", typeStr);
            typeElem.setAttribute("name", CkId(type).getTypeName());
            
            typeCount += exportCollectionToXML(collection, type, doc, typeElem, filter);
            
            root.appendChild(typeElem);
        };
        
        switch (type)
        {
        case CkId::Type_Npc_:      addCollection(data.getNpcCollection()); break;
        case CkId::Type_Weap_:     addCollection(data.getWeaponCollection()); break;
        case CkId::Type_Armor_:    addCollection(data.getArmorCollection()); break;
        case CkId::Type_Spel_:     addCollection(data.getSpellCollection()); break;
        case CkId::Type_Magic_:    addCollection(data.getMagicCollection()); break;
        case CkId::Type_Quest_:    addCollection(data.getQuestCollection()); break;
        case CkId::Type_Dial_:     addCollection(data.getDialCollection()); break;
        case CkId::Type_Info_:     addCollection(data.getInfoCollection()); break;
        case CkId::Type_Glob_:     addCollection(data.getGlobCollection()); break;
        case CkId::Type_Lcrt_:     addCollection(data.getLcrtCollection()); break;
        case CkId::Type_Pack_:     addCollection(data.getPackCollection()); break;
        case CkId::Type_Tree_:     addCollection(data.getTreeCollection()); break;
        case CkId::Type_Alch_:     addCollection(data.getAlchCollection()); break;
        case CkId::Type_Ingr_:     addCollection(data.getIngrCollection()); break;
        case CkId::Type_Cont_:     addCollection(data.getContCollection()); break;
        case CkId::Type_Ench_:     addCollection(data.getEnchCollection()); break;
        case CkId::Type_Book_:     addCollection(data.getBookCollection()); break;
        case CkId::Type_Misc_:     addCollection(data.getMiscCollection()); break;
        case CkId::Type_Acti_:     addCollection(data.getActiCollection()); break;
        case CkId::Type_Stat_:     addCollection(data.getStatCollection()); break;
        case CkId::Type_Race_:     addCollection(data.getRaceCollection()); break;
        case CkId::Type_Class_:    addCollection(data.getClassCollection()); break;
        case CkId::Type_Fact_:     addCollection(data.getFactCollection()); break;
        case CkId::Type_PerK_:     addCollection(data.getPerkCollection()); break;
        case CkId::Type_Cel_:      addCollection(data.getCellCollection()); break;
        case CkId::Type_WRLD_:     addCollection(data.getWorldspaceCollection()); break;
        case CkId::Type_LOCT_:     addCollection(data.getLocationCollection()); break;
        case CkId::Type_Refr_:     addCollection(data.getRefrCollection()); break;
        case CkId::Type_Material_: addCollection(data.getMaterialCollection()); break;
        case CkId::Type_Land_:    addCollection(data.getLandCollection()); break;
        case CkId::Type_Soun_:     addCollection(data.getSounCollection()); break;
        case CkId::Type_Wthr_:     addCollection(data.getWthrCollection()); break;
        case CkId::Type_Ltex_:     addCollection(data.getLtexCollection()); break;
        default: break;
        }
        
        result.recordsExported += typeCount;
        if (typeCount > 0)
            result.recordsByType[typeStr] = typeCount;
    }
    
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        result.error = QString("Cannot write to %1").arg(outputPath);
        return result;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << doc.toString();
    
    file.close();
    QFileInfo fileInfo(outputPath);
    result.fileSize = fileInfo.size();
    result.outputPath = outputPath;
    return result;
}

int DataExporter::exportCollectionToJSON(const BaseCollection& collection, CkId::Type type, QJsonArray& array, const ExportFilter& filter)
{
    int count = 0;
    for (int i = 0; i < collection.size(); i++)
    {
        const BaseRecord& record = collection.getRecord(i);
        if (!matchesFilter(record, filter))
            continue;
        array.append(recordToJSON(record, type));
        count++;
    }
    return count;
}

int DataExporter::exportCollectionToCSV(const BaseCollection& collection, CkId::Type type, QString& csvContent, QStringList& headers, const ExportFilter& filter)
{
    if (collection.size() == 0) return 0;
    
    const BaseRecord& first = collection.getRecord(0);
    headers = recordToCSVFields(first, type, headers);
    
    int count = 0;
    for (int i = 0; i < collection.size(); i++)
    {
        const BaseRecord& record = collection.getRecord(i);
        if (!matchesFilter(record, filter))
            continue;
        QStringList fields = recordToCSVFields(record, type, headers);
        csvContent += fields.join(",") + "\n";
        count++;
    }
    
    return count;
}

int DataExporter::exportCollectionToXML(const BaseCollection& collection, CkId::Type type, QDomDocument& doc, QDomElement& root, const ExportFilter& filter)
{
    int count = 0;
    for (int i = 0; i < collection.size(); i++)
    {
        const BaseRecord& record = collection.getRecord(i);
        if (!matchesFilter(record, filter))
            continue;
        QDomElement recordElem = recordToXML(record, type, doc);
        root.appendChild(recordElem);
        count++;
    }
    return count;
}

static void jsonVecU32(QJsonObject& obj, const QString& key, const QVector<quint32>& vec)
{
    QJsonArray arr;
    for (const auto& v : vec) arr.append(static_cast<qint64>(v));
    obj[key] = arr;
}
static void jsonVecStr(QJsonObject& obj, const QString& key, const QVector<QString>& vec)
{
    QJsonArray arr;
    for (const auto& v : vec) arr.append(v);
    obj[key] = arr;
}
static inline QJsonValue toJV(quint32 v) { return QJsonValue(static_cast<qint64>(v)); }
static inline QJsonValue toJV(const QString& v) { return QJsonValue(v); }
static inline QJsonValue toJV(float v) { return QJsonValue(static_cast<double>(v)); }
static inline QJsonValue toJV(bool v) { return QJsonValue(v); }
static inline QJsonValue toJV(qint64 v) { return QJsonValue(v); }
static inline QJsonValue toJV(const QVariant& v) { return QJsonValue::fromVariant(v); }
static inline QJsonValue toJV(int v) { return QJsonValue(v); }

QJsonObject DataExporter::recordToJSON(const BaseRecord& record, CkId::Type type)
{
    QJsonObject obj;
    obj["formId"] = toJV(QString("0x%1").arg(getFormId(record), 8, 16, QChar('0')));
    obj["editorId"] = toJV(getEditorId(record));
    obj["state"] = toJV(getStateString(record.state));

    switch (type) {
    case CkId::Type_Npc_: {
        const auto& npc = dynamic_cast<const Record<NpcRecord>&>(record).get();
        obj["flags"] = toJV(npc.flags);
        obj["fullName"] = toJV(npc.fullName);
        obj["level"] = toJV(npc.level);
        obj["health"] = toJV(npc.health);
        obj["magicka"] = toJV(npc.magicka);
        obj["stamina"] = toJV(npc.stamina);
        obj["attack"] = toJV(npc.attack);
        obj["defense"] = toJV(npc.defense);
        obj["personality"] = toJV(npc.personality);
        obj["intelligence"] = toJV(npc.intelligence);
        obj["willpower"] = toJV(npc.willpower);
        obj["agility"] = toJV(npc.agility);
        obj["luck"] = toJV(npc.luck);
        obj["disposition"] = toJV(npc.disposition);
        obj["reputation"] = toJV(npc.reputation);
        obj["race"] = toJV(npc.race);
        obj["classId"] = toJV(npc.class_);
        obj["faction"] = toJV(npc.faction);
        obj["sex"] = toJV(npc.sex);
        jsonVecU32(obj, "spells", npc.spells);
        jsonVecU32(obj, "inventoryItems", npc.inventoryItems);
        jsonVecU32(obj, "relationships", npc.relationships);
        break;
    }
    case CkId::Type_Weap_: {
        const auto& w = dynamic_cast<const Record<WeaponRecord>&>(record).get();
        obj["flags"] = toJV(w.flags);
        obj["fullName"] = toJV(w.fullName);
        obj["weaponType"] = toJV(w.weaponType);
        obj["damage"] = toJV(w.damage);
        obj["speed"] = toJV(w.speed);
        obj["reach"] = toJV(w.reach);
        obj["weight"] = toJV(w.weight);
        obj["value"] = toJV(w.value);
        obj["enchantment"] = toJV(w.enchantment);
        obj["iconPath"] = toJV(w.iconPath);
        obj["modelPath"] = toJV(w.modelPath);
        obj["magicSchool"] = toJV(w.magicSchool);
        obj["enchantLimit"] = toJV(w.enchantLimit);
        break;
    }
    case CkId::Type_Armor_: {
        const auto& a = dynamic_cast<const Record<ArmorRecord>&>(record).get();
        obj["flags"] = toJV(a.flags);
        obj["fullName"] = toJV(a.fullName);
        obj["armorRating"] = toJV(a.armorRating);
        obj["weight"] = toJV(a.weight);
        obj["value"] = toJV(a.value);
        obj["iconPath"] = toJV(a.iconPath);
        obj["modelPath"] = toJV(a.modelPath);
        obj["health"] = toJV(a.health);
        break;
    }
    case CkId::Type_Spel_: {
        const auto& s = dynamic_cast<const Record<SpellRecord>&>(record).get();
        obj["flags"] = toJV(s.flags);
        obj["fullName"] = toJV(s.fullName);
        obj["cost"] = toJV(s.cost);
        obj["castingSound"] = toJV(s.castingSound);
        jsonVecU32(obj, "effects", s.effects);
        obj["enchantment"] = toJV(s.enchantment);
        break;
    }
    case CkId::Type_Magic_: {
        const auto& m = dynamic_cast<const Record<MagicRecord>&>(record).get();
        obj["flags"] = toJV(m.flags);
        obj["schools"] = toJV(m.schools);
        obj["damageType"] = toJV(m.damageType);
        obj["castingSound"] = toJV(m.castingSound);
        obj["iconPath"] = toJV(m.iconPath);
        obj["modelPath"] = toJV(m.modelPath);
        jsonVecU32(obj, "effects", m.effects);
        break;
    }
    case CkId::Type_Quest_: {
        const auto& q = dynamic_cast<const Record<QuestRecord>&>(record).get();
        obj["flags"] = toJV(q.flags);
        obj["questName"] = toJV(q.questName);
        obj["questDesc"] = toJV(q.questDesc);
        obj["questType"] = toJV(q.questType);
        jsonVecU32(obj, "stageIds", q.stageIds);
        jsonVecStr(obj, "stageDescriptions", q.stageDescriptions);
        jsonVecU32(obj, "objectiveIds", q.objectiveIds);
        jsonVecU32(obj, "aliasIds", q.aliasIds);
        obj["dialogueView"] = toJV(q.dialogueView);
        jsonVecU32(obj, "scriptIds", q.scriptIds);
        break;
    }
    case CkId::Type_Dial_: {
        const auto& d = dynamic_cast<const Record<DialRecord>&>(record).get();
        obj["flags"] = toJV(d.flags);
        obj["topicName"] = toJV(d.topicName);
        jsonVecU32(obj, "responseIds", d.responseIds);
        jsonVecU32(obj, "conditionIds", d.conditionIds);
        jsonVecU32(obj, "animationIds", d.animationIds);
        jsonVecU32(obj, "emotionIds", d.emotionIds);
        break;
    }
    case CkId::Type_Info_: {
        const auto& i = dynamic_cast<const Record<InfoRecord>&>(record).get();
        obj["flags"] = toJV(i.flags);
        obj["responseText"] = toJV(i.responseText);
        jsonVecU32(obj, "conditionIds", i.conditionIds);
        obj["targetId"] = toJV(i.targetId);
        jsonVecU32(obj, "scriptIds", i.scriptIds);
        break;
    }
    case CkId::Type_Glob_: {
        const auto& g = dynamic_cast<const Record<GlobalVariable>&>(record).get();
        obj["constant"] = toJV(g.constant);
        obj["value"] = toJV(g.value.getData());
        break;
    }
    case CkId::Type_Lcrt_: {
        const auto& l = dynamic_cast<const Record<LocationRefType>&>(record).get();
        obj["color"] = toJV(l.color);
        break;
    }
    case CkId::Type_Pack_: {
        const auto& p = dynamic_cast<const Record<PackageRecord>&>(record).get();
        obj["flags"] = toJV(p.flags);
        obj["packageType"] = toJV(p.packageType);
        obj["targetType"] = toJV(p.targetType);
        jsonVecU32(obj, "targetIds", p.targetIds);
        jsonVecU32(obj, "parameters", p.parameters);
        break;
    }
    case CkId::Type_Tree_: {
        const auto& t = dynamic_cast<const Record<TreeRecord>&>(record).get();
        obj["flags"] = toJV(t.flags);
        obj["iconPath"] = toJV(t.iconPath);
        obj["modelPath"] = toJV(t.modelPath);
        obj["leafCurvature"] = toJV(t.leafCurvature);
        obj["leafAmplitude"] = toJV(t.leafAmplitude);
        obj["lodModelPath"] = toJV(t.lodModelPath);
        obj["lodFlags"] = toJV(t.lodFlags);
        break;
    }
    case CkId::Type_Alch_: {
        const auto& a = dynamic_cast<const Record<AlchRecord>&>(record).get();
        obj["flags"] = toJV(a.flags);
        obj["iconPath"] = toJV(a.iconPath);
        obj["modelPath"] = toJV(a.modelPath);
        obj["weight"] = toJV(a.weight);
        obj["value"] = toJV(a.value);
        break;
    }
    case CkId::Type_Ingr_: {
        const auto& i = dynamic_cast<const Record<IngrRecord>&>(record).get();
        obj["flags"] = toJV(i.flags);
        obj["iconPath"] = toJV(i.iconPath);
        obj["modelPath"] = toJV(i.modelPath);
        obj["weight"] = toJV(i.weight);
        obj["value"] = toJV(i.value);
        break;
    }
    case CkId::Type_Cont_: {
        const auto& c = dynamic_cast<const Record<ContRecord>&>(record).get();
        obj["flags"] = toJV(c.flags);
        obj["iconPath"] = toJV(c.iconPath);
        obj["modelPath"] = toJV(c.modelPath);
        obj["contents"] = toJV(c.contents);
        obj["inventoryControl"] = toJV(c.inventoryControl);
        obj["weight"] = toJV(c.weight);
        obj["value"] = toJV(c.value);
        break;
    }
    case CkId::Type_Ench_: {
        const auto& e = dynamic_cast<const Record<EnchRecord>&>(record).get();
        obj["flags"] = toJV(e.flags);
        obj["name"] = toJV(e.name);
        obj["costLimit"] = toJV(e.costLimit);
        obj["charges"] = toJV(e.charges);
        obj["enchantmentData"] = toJV(e.enchantmentData);
        obj["charge"] = toJV(e.charge);
        obj["duration"] = toJV(e.duration);
        obj["magnitude"] = toJV(e.magnitude);
        obj["type"] = toJV(e.type);
        obj["soulGem"] = toJV(e.soulGem);
        break;
    }
    case CkId::Type_Book_: {
        const auto& b = dynamic_cast<const Record<BookRecord>&>(record).get();
        obj["flags"] = toJV(b.flags);
        obj["pageCount"] = toJV(b.pageCount);
        obj["pages"] = toJV(b.pages);
        obj["iconPath"] = toJV(b.iconPath);
        obj["modelPath"] = toJV(b.modelPath);
        break;
    }
    case CkId::Type_Misc_: {
        const auto& m = dynamic_cast<const Record<MiscRecord>&>(record).get();
        obj["flags"] = toJV(m.flags);
        obj["iconPath"] = toJV(m.iconPath);
        obj["modelPath"] = toJV(m.modelPath);
        obj["weight"] = toJV(m.weight);
        obj["value"] = toJV(m.value);
        break;
    }
    case CkId::Type_Acti_: {
        const auto& a = dynamic_cast<const Record<ActiRecord>&>(record).get();
        obj["flags"] = toJV(a.flags);
        obj["iconPath"] = toJV(a.iconPath);
        obj["modelPath"] = toJV(a.modelPath);
        break;
    }
    case CkId::Type_Stat_: {
        const auto& s = dynamic_cast<const Record<StatRecord>&>(record).get();
        obj["flags"] = toJV(s.flags);
        obj["iconPath"] = toJV(s.iconPath);
        obj["modelPath"] = toJV(s.modelPath);
        obj["lodModelPath"] = toJV(s.lodModelPath);
        obj["lodFlags"] = toJV(s.lodFlags);
        break;
    }
    case CkId::Type_Race_: {
        const auto& r = dynamic_cast<const Record<RaceRecord>&>(record).get();
        obj["flags"] = toJV(r.flags);
        obj["raceFlags"] = toJV(r.raceFlags);
        jsonVecU32(obj, "npcVariables", r.npcVariables);
        jsonVecU32(obj, "faceData", r.faceData);
        jsonVecU32(obj, "headData", r.headData);
        break;
    }
    case CkId::Type_Class_: {
        const auto& c = dynamic_cast<const Record<ClassRecord>&>(record).get();
        obj["flags"] = toJV(c.flags);
        obj["className"] = toJV(c.className);
        obj["description"] = toJV(c.description);
        obj["serviceFlags"] = toJV(c.serviceFlags);
        obj["iconPath"] = toJV(c.iconPath);
        break;
    }
    case CkId::Type_Fact_: {
        const auto& f = dynamic_cast<const Record<FactRecord>&>(record).get();
        obj["flags"] = toJV(f.flags);
        obj["factionName"] = toJV(f.factionName);
        obj["description"] = toJV(f.description);
        obj["iconPath"] = toJV(f.iconPath);
        jsonVecStr(obj, "ranks", f.ranks);
        jsonVecU32(obj, "relations", f.relations);
        break;
    }
    case CkId::Type_PerK_: {
        const auto& p = dynamic_cast<const Record<PerkRecord>&>(record).get();
        obj["flags"] = toJV(p.flags);
        obj["description"] = toJV(p.description);
        obj["requirements"] = toJV(p.requirements);
        obj["iconPath"] = toJV(p.iconPath);
        jsonVecU32(obj, "conditions", p.conditions);
        break;
    }
    case CkId::Type_Cel_: {
        const auto& c = dynamic_cast<const Record<CellRecord>&>(record).get();
        obj["flags"] = toJV(c.flags);
        obj["cellX"] = toJV(c.cellX);
        obj["cellY"] = toJV(c.cellY);
        obj["owner"] = toJV(c.owner);
        obj["lockLevel"] = toJV(c.lockLevel);
        obj["cellName"] = toJV(c.cellName);
        break;
    }
    case CkId::Type_WRLD_: {
        const auto& w = dynamic_cast<const Record<WorldspaceRecord>&>(record).get();
        obj["flags"] = toJV(w.flags);
        obj["name"] = toJV(w.name);
        obj["waterType"] = toJV(w.waterType);
        obj["climateId"] = toJV(w.climateId);
        obj["lightingId"] = toJV(w.lightingId);
        obj["mapWidth"] = toJV(w.mapWidth);
        obj["mapHeight"] = toJV(w.mapHeight);
        obj["mapNwX"] = toJV(w.mapNwX);
        obj["mapNwY"] = toJV(w.mapNwY);
        obj["mapSeX"] = toJV(w.mapSeX);
        obj["mapSeY"] = toJV(w.mapSeY);
        obj["mapScale"] = toJV(w.mapScale());
        obj["mapLodBias"] = toJV(w.mapLodBias);
        obj["mapSize"] = toJV(w.mapSize);
        obj["template"] = toJV(w.templ);
        obj["terrain"] = toJV(w.terrain);
        obj["mapImage"] = toJV(w.mapImage);
        obj["lodNoise"] = toJV(w.lodNoise);
        obj["billboardTexture"] = toJV(w.billboardTexture);
        obj["music"] = toJV(w.music);
        obj["dnam"] = toJV(w.dnam);
        obj["dataMinX"] = toJV(w.dataMinX);
        obj["dataMinY"] = toJV(w.dataMinY);
        jsonVecU32(obj, "cellIds", w.cellIds);
        jsonVecU32(obj, "navPointIds", w.navPointIds);
        break;
    }
    case CkId::Type_LOCT_: {
        const auto& l = dynamic_cast<const Record<LocationRecord>&>(record).get();
        obj["flags"] = toJV(l.flags);
        obj["locationName"] = toJV(l.locationName);
        obj["parentId"] = toJV(l.parentId);
        obj["x"] = toJV(l.x);
        obj["y"] = toJV(l.y);
        obj["z"] = toJV(l.z);
        break;
    }
    case CkId::Type_Refr_: {
        const auto& r = dynamic_cast<const Record<RefrRecord>&>(record).get();
        obj["baseId"] = toJV(r.baseId);
        obj["posX"] = toJV(r.posX);
        obj["posY"] = toJV(r.posY);
        obj["posZ"] = toJV(r.posZ);
        obj["rotX"] = toJV(r.rotX);
        obj["rotY"] = toJV(r.rotY);
        obj["rotZ"] = toJV(r.rotZ);
        obj["scale"] = toJV(r.scale);
        obj["owner"] = toJV(r.owner);
        obj["lockLevel"] = toJV(r.lockLevel);
        obj["initiallyDisabled"] = toJV(r.initiallyDisabled);
        jsonVecU32(obj, "scriptIds", r.scriptIds);
        break;
    }
    case CkId::Type_Material_: {
        const auto& m = dynamic_cast<const Record<MaterialRecord>&>(record).get();
        obj["flags"] = toJV(m.flags);
        obj["materialName"] = toJV(m.materialName);
        obj["name"] = toJV(m.name);
        obj["description"] = toJV(m.description);
        obj["iconPath"] = toJV(m.iconPath);
        obj["modelPath"] = toJV(m.modelPath);
        obj["bnam"] = toJV(m.bnam);
        obj["cnam"] = toJV(m.cnam);
        obj["texturePath"] = toJV(m.texturePath);
        obj["materialType"] = toJV(m.materialType);
        obj["value"] = toJV(m.value);
        obj["weight"] = toJV(m.weight);
        obj["health"] = toJV(m.health);
        obj["magicka"] = toJV(m.magicka);
        obj["stamina"] = toJV(m.stamina);
        obj["level"] = toJV(m.level);
        obj["race"] = toJV(m.race);
        obj["faction"] = toJV(m.faction);
        obj["stage"] = toJV(m.stage);
        obj["difficulty"] = toJV(m.difficulty);
        break;
    }
    case CkId::Type_Land_: {
        const auto& l = dynamic_cast<const Record<LandRecord>&>(record).get();
        obj["flags"] = toJV(l.flags);
        obj["cellX"] = toJV(l.cellX);
        obj["cellY"] = toJV(l.cellY);
        obj["baseHeight"] = toJV(l.baseHeight);
        obj["hasHeightData"] = toJV(l.hasHeightData);
        obj["hasNormalData"] = toJV(l.hasNormalData);
        obj["hasColorData"] = toJV(l.hasColorData);
        if (l.hasHeightData) {
            obj["heightData"] = QString(QByteArray::fromRawData(reinterpret_cast<const char*>(l.heightData), sizeof(l.heightData)).toBase64());
        }
        if (l.hasNormalData) {
            obj["normalData"] = QString(QByteArray::fromRawData(reinterpret_cast<const char*>(l.normalData), sizeof(l.normalData)).toBase64());
        }
        if (l.hasColorData) {
            obj["colorData"] = QString(QByteArray::fromRawData(reinterpret_cast<const char*>(l.colorData), sizeof(l.colorData)).toBase64());
        }
        obj["numTextureLayers"] = toJV(l.numTextureLayers);
        QJsonArray texArr;
        for (int i = 0; i < l.numTextureLayers; ++i) {
            QJsonObject texObj;
            texObj["textureFormId"] = toJV(l.textureLayers[i].textureFormId);
            texObj["opacity"] = toJV(l.textureLayers[i].opacity);
            texArr.append(texObj);
        }
        obj["textureLayers"] = texArr;
        break;
    }
    case CkId::Type_Soun_: {
        const auto& s = dynamic_cast<const Record<SounRecord>&>(record).get();
        obj["flags"] = toJV(s.flags);
        obj["soundFile"] = toJV(s.soundFile);
        break;
    }
    case CkId::Type_Wthr_: {
        const auto& w = dynamic_cast<const Record<WthrRecord>&>(record).get();
        obj["flags"] = toJV(w.flags);
        obj["sunTexture"] = toJV(w.sunTexture);
        break;
    }
    case CkId::Type_Ltex_: {
        const auto& t = dynamic_cast<const Record<LtexRecord>&>(record).get();
        obj["flags"] = toJV(t.flags);
        obj["iconPath"] = toJV(t.iconPath);
        obj["havokMaterial"] = toJV(t.havokMaterial);
        QJsonArray grassArr;
        for (quint32 gid : t.grassFormIds)
            grassArr.append(static_cast<qint64>(gid));
        obj["grassFormIds"] = grassArr;
        break;
    }
    default:
        break;
    }

    return obj;
}
static QString vecU32Csv(const QVector<quint32>& vec)
{
    QStringList s;
    for (const auto& v : vec) s << QString::number(v);
    return s.join(";");
}
static QString vecStrCsv(const QVector<QString>& vec)
{
    QStringList s;
    for (const auto& v : vec) s << v;
    return s.join(";");
}

QStringList DataExporter::recordToCSVFields(const BaseRecord& record, CkId::Type type, QStringList& headers)
{
    if (headers.isEmpty())
    {
        headers << "FormID" << "EditorID" << "State";
    }

    QStringList fields;
    fields << QString("0x%1").arg(getFormId(record), 8, 16, QChar('0'));
    fields << getEditorId(record);
    fields << getStateString(record.state);

    switch (type) {
    case CkId::Type_Npc_: {
        const auto& npc = dynamic_cast<const Record<NpcRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "FullName" << "Level"
                    << "Health" << "Magicka" << "Stamina" << "Intelligence"
                    << "Race" << "ClassId" << "Faction" << "Sex"
                    << "Spells" << "InventoryItems" << "Relationships";
        }
        fields << QString::number(npc.flags) << npc.fullName
               << QString::number(npc.level)
               << QString::number(npc.health) << QString::number(npc.magicka)
               << QString::number(npc.stamina) << QString::number(npc.intelligence)
               << QString::number(npc.race) << QString::number(npc.class_)
               << QString::number(npc.faction) << QString::number(npc.sex)
               << vecU32Csv(npc.spells) << vecU32Csv(npc.inventoryItems)
               << vecU32Csv(npc.relationships);
        break;
    }
    case CkId::Type_Weap_: {
        const auto& w = dynamic_cast<const Record<WeaponRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "FullName" << "WeaponType" << "Damage" << "Speed" << "Reach"
                    << "Weight" << "Value" << "Enchantment" << "IconPath" << "ModelPath"
                    << "MagicSchool" << "EnchantLimit";
        }
        fields << QString::number(w.flags) << w.fullName
               << QString::number(w.weaponType) << QString::number(w.damage, 'f')
               << QString::number(w.speed, 'f') << QString::number(w.reach, 'f')
               << QString::number(w.weight, 'f')
               << QString::number(w.value) << QString::number(w.enchantment)
               << w.iconPath << w.modelPath
               << QString::number(w.magicSchool) << QString::number(w.enchantLimit);
        break;
    }
    case CkId::Type_Armor_: {
        const auto& a = dynamic_cast<const Record<ArmorRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "FullName" << "ArmorRating" << "Weight" << "Value"
                    << "IconPath" << "ModelPath" << "Health";
        }
        fields << QString::number(a.flags) << a.fullName
               << QString::number(a.armorRating) << QString::number(a.weight, 'f')
               << QString::number(a.value) << a.iconPath << a.modelPath
               << QString::number(a.health, 'f');
        break;
    }
    case CkId::Type_Spel_: {
        const auto& s = dynamic_cast<const Record<SpellRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "FullName" << "Cost" << "CastingSound" << "Effects" << "Enchantment";
        }
        fields << QString::number(s.flags) << s.fullName
               << QString::number(s.cost) << QString::number(s.castingSound)
               << vecU32Csv(s.effects) << QString::number(s.enchantment);
        break;
    }
    case CkId::Type_Magic_: {
        const auto& m = dynamic_cast<const Record<MagicRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "Schools" << "DamageType" << "CastingSound"
                    << "IconPath" << "ModelPath" << "Effects";
        }
        fields << QString::number(m.flags)
               << QString::number(m.schools) << QString::number(m.damageType)
               << QString::number(m.castingSound) << m.iconPath << m.modelPath
               << vecU32Csv(m.effects);
        break;
    }
    case CkId::Type_Quest_: {
        const auto& q = dynamic_cast<const Record<QuestRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "QuestName" << "QuestDesc" << "QuestType"
                    << "StageIds" << "StageDescriptions" << "ObjectiveIds"
                    << "AliasIds" << "DialogueView" << "ScriptIds";
        }
        fields << QString::number(q.flags) << q.questName << q.questDesc
               << QString::number(q.questType)
               << vecU32Csv(q.stageIds) << vecStrCsv(q.stageDescriptions)
               << vecU32Csv(q.objectiveIds) << vecU32Csv(q.aliasIds)
               << q.dialogueView << vecU32Csv(q.scriptIds);
        break;
    }
    case CkId::Type_Dial_: {
        const auto& d = dynamic_cast<const Record<DialRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "TopicName" << "ResponseIds" << "ConditionIds"
                    << "AnimationIds" << "EmotionIds";
        }
        fields << QString::number(d.flags) << d.topicName
               << vecU32Csv(d.responseIds) << vecU32Csv(d.conditionIds)
               << vecU32Csv(d.animationIds) << vecU32Csv(d.emotionIds);
        break;
    }
    case CkId::Type_Info_: {
        const auto& i = dynamic_cast<const Record<InfoRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "ResponseText" << "ConditionIds" << "TargetId" << "ScriptIds";
        }
        fields << QString::number(i.flags) << i.responseText
               << vecU32Csv(i.conditionIds) << QString::number(i.targetId)
               << vecU32Csv(i.scriptIds);
        break;
    }
    case CkId::Type_Glob_: {
        const auto& g = dynamic_cast<const Record<GlobalVariable>&>(record).get();
        if (headers.size() == 3) {
            headers << "Constant" << "Value";
        }
        fields << (g.constant ? "1" : "0") << g.value.getData().toString();
        break;
    }
    case CkId::Type_Lcrt_: {
        const auto& l = dynamic_cast<const Record<LocationRefType>&>(record).get();
        if (headers.size() == 3) {
            headers << "Color";
        }
        fields << QString::number(l.color);
        break;
    }
    case CkId::Type_Pack_: {
        const auto& p = dynamic_cast<const Record<PackageRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "PackageType" << "TargetType" << "TargetIds" << "Parameters";
        }
        fields << QString::number(p.flags)
               << QString::number(p.packageType) << QString::number(p.targetType)
               << vecU32Csv(p.targetIds) << vecU32Csv(p.parameters);
        break;
    }
    case CkId::Type_Tree_: {
        const auto& t = dynamic_cast<const Record<TreeRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "IconPath" << "ModelPath" << "LeafCurvature" << "LeafAmplitude"
                    << "LodModelPath" << "LodFlags";
        }
        fields << QString::number(t.flags) << t.iconPath << t.modelPath
               << QString::number(t.leafCurvature) << QString::number(t.leafAmplitude)
               << t.lodModelPath
               << QString::number(t.lodFlags);
        break;
    }
    case CkId::Type_Alch_: {
        const auto& a = dynamic_cast<const Record<AlchRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "IconPath" << "ModelPath" << "Weight" << "Value";
        }
        fields << QString::number(a.flags) << a.iconPath << a.modelPath
               << QString::number(a.weight, 'f')
               << QString::number(a.value);
        break;
    }
    case CkId::Type_Ingr_: {
        const auto& i = dynamic_cast<const Record<IngrRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "IconPath" << "ModelPath" << "Weight" << "Value";
        }
        fields << QString::number(i.flags) << i.iconPath << i.modelPath
               << QString::number(i.weight, 'f')
               << QString::number(i.value);
        break;
    }
    case CkId::Type_Cont_: {
        const auto& c = dynamic_cast<const Record<ContRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "IconPath" << "ModelPath" << "Contents"
                    << "InventoryControl" << "Weight" << "Value";
        }
        fields << QString::number(c.flags) << c.iconPath << c.modelPath
               << QString::number(c.contents) << QString::number(c.inventoryControl)
               << QString::number(c.weight, 'f') << QString::number(c.value);
        break;
    }
    case CkId::Type_Ench_: {
        const auto& e = dynamic_cast<const Record<EnchRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "Name" << "CostLimit" << "Charges" << "EnchantmentData"
                    << "Charge" << "Duration" << "Magnitude" << "Type" << "SoulGem";
        }
        fields << QString::number(e.flags) << e.name
               << QString::number(e.costLimit) << QString::number(e.charges)
               << QString::number(e.enchantmentData) << QString::number(e.charge, 'f')
               << QString::number(e.duration) << QString::number(e.magnitude, 'f')
               << QString::number(e.type) << QString::number(e.soulGem);
        break;
    }
    case CkId::Type_Book_: {
        const auto& b = dynamic_cast<const Record<BookRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "PageCount" << "Pages" << "IconPath" << "ModelPath";
        }
        fields << QString::number(b.flags) << QString::number(b.pageCount)
               << b.pages << b.iconPath << b.modelPath;
        break;
    }
    case CkId::Type_Misc_: {
        const auto& m = dynamic_cast<const Record<MiscRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "IconPath" << "ModelPath" << "Weight" << "Value";
        }
        fields << QString::number(m.flags) << m.iconPath << m.modelPath
               << QString::number(m.weight, 'f')
               << QString::number(m.value);
        break;
    }
    case CkId::Type_Acti_: {
        const auto& a = dynamic_cast<const Record<ActiRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "IconPath" << "ModelPath";
        }
        fields << QString::number(a.flags) << a.iconPath << a.modelPath;
        break;
    }
    case CkId::Type_Stat_: {
        const auto& s = dynamic_cast<const Record<StatRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "IconPath" << "ModelPath"
                    << "LodModelPath" << "LodFlags";
        }
        fields << QString::number(s.flags) << s.iconPath << s.modelPath
               << s.lodModelPath
               << QString::number(s.lodFlags);
        break;
    }
    case CkId::Type_Race_: {
        const auto& r = dynamic_cast<const Record<RaceRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "RaceFlags" << "NpcVariables" << "FaceData" << "HeadData";
        }
        fields << QString::number(r.flags) << QString::number(r.raceFlags)
               << vecU32Csv(r.npcVariables) << vecU32Csv(r.faceData) << vecU32Csv(r.headData);
        break;
    }
    case CkId::Type_Class_: {
        const auto& c = dynamic_cast<const Record<ClassRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "ClassName" << "Description" << "ServiceFlags" << "IconPath";
        }
        fields << QString::number(c.flags) << c.className << c.description
               << QString::number(c.serviceFlags) << c.iconPath;
        break;
    }
    case CkId::Type_Fact_: {
        const auto& f = dynamic_cast<const Record<FactRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "FactionName" << "Description" << "IconPath" << "Ranks" << "Relations";
        }
        fields << QString::number(f.flags) << f.factionName << f.description << f.iconPath
               << vecStrCsv(f.ranks) << vecU32Csv(f.relations);
        break;
    }
    case CkId::Type_PerK_: {
        const auto& p = dynamic_cast<const Record<PerkRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "Description" << "Requirements" << "IconPath" << "Conditions";
        }
        fields << QString::number(p.flags) << p.description << p.requirements << p.iconPath
               << vecU32Csv(p.conditions);
        break;
    }
    case CkId::Type_Cel_: {
        const auto& c = dynamic_cast<const Record<CellRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "CellX" << "CellY" << "Owner" << "LockLevel" << "CellName";
        }
        fields << QString::number(c.flags) << QString::number(c.cellX) << QString::number(c.cellY)
               << QString::number(c.owner) << QString::number(c.lockLevel) << c.cellName;
        break;
    }
    case CkId::Type_WRLD_: {
        const auto& w = dynamic_cast<const Record<WorldspaceRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "Name" << "WaterType" << "ClimateId" << "LightingId"
                    << "MapSize" << "Template" << "Terrain" << "MapImage"
                    << "LodNoise" << "BillboardTexture" << "Music" << "Dnam"
                    << "DataMinX" << "DataMinY" << "CellIds" << "NavPointIds";
        }
        fields << QString::number(w.flags) << w.name
               << QString::number(w.waterType) << QString::number(w.climateId)
               << QString::number(w.lightingId) << QString::number(w.mapSize)
               << QString::number(w.templ) << QString::number(w.terrain)
               << w.mapImage << w.lodNoise << w.billboardTexture
               << QString::number(w.music) << QString::number(w.dnam)
               << QString::number(w.dataMinX) << QString::number(w.dataMinY)
               << vecU32Csv(w.cellIds) << vecU32Csv(w.navPointIds);
        break;
    }
    case CkId::Type_LOCT_: {
        const auto& l = dynamic_cast<const Record<LocationRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "LocationName" << "ParentId" << "X" << "Y" << "Z";
        }
        fields << QString::number(l.flags) << l.locationName
               << QString::number(l.parentId) << QString::number(l.x)
               << QString::number(l.y) << QString::number(l.z);
        break;
    }
    case CkId::Type_Refr_: {
        const auto& r = dynamic_cast<const Record<RefrRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "BaseId"
                    << "PosX" << "PosY" << "PosZ" << "RotX" << "RotY" << "RotZ"
                    << "Scale" << "Owner" << "LockLevel" << "InitiallyDisabled"
                    << "ScriptIds";
        }
        fields << QString::number(r.baseId)
               << QString::number(r.posX, 'f') << QString::number(r.posY, 'f')
               << QString::number(r.posZ, 'f') << QString::number(r.rotX, 'f')
               << QString::number(r.rotY, 'f') << QString::number(r.rotZ, 'f')
               << QString::number(r.scale, 'f') << QString::number(r.owner)
               << QString::number(r.lockLevel)
               << (r.initiallyDisabled ? "1" : "0")
               << vecU32Csv(r.scriptIds);
        break;
    }
    case CkId::Type_Material_: {
        const auto& m = dynamic_cast<const Record<MaterialRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "MaterialName" << "Name" << "Description"
                    << "IconPath" << "ModelPath" << "Bnam" << "Cnam" << "TexturePath"
                    << "MaterialType" << "Value" << "Weight" << "Health"
                    << "Magicka" << "Stamina" << "Level" << "Race" << "Faction"
                    << "Stage" << "Difficulty";
        }
        fields << QString::number(m.flags) << m.materialName << m.name << m.description
               << m.iconPath << m.modelPath << m.bnam << m.cnam << m.texturePath
               << QString::number(m.materialType) << QString::number(m.value)
               << QString::number(m.weight) << QString::number(m.health)
               << QString::number(m.magicka) << QString::number(m.stamina)
               << QString::number(m.level) << QString::number(m.race)
               << QString::number(m.faction) << QString::number(m.stage)
               << QString::number(m.difficulty);
        break;
    }
    case CkId::Type_Land_: {
        const auto& l = dynamic_cast<const Record<LandRecord>&>(record).get();
        if (headers.size() == 3) {
            headers << "Flags" << "CellX" << "CellY" << "BaseHeight"
                    << "HasHeightData" << "HasNormalData" << "HasColorData"
                    << "NumTextureLayers";
        }
        fields << QString::number(l.flags) << QString::number(l.cellX) << QString::number(l.cellY)
               << QString::number(l.baseHeight, 'f')
               << (l.hasHeightData ? "1" : "0") << (l.hasNormalData ? "1" : "0")
               << (l.hasColorData ? "1" : "0") << QString::number(l.numTextureLayers);
        break;
    }
    case CkId::Type_Soun_: {
        const auto& s = dynamic_cast<const Record<SounRecord>&>(record).get();
        if (headers.size() == 3)
            headers << "Flags" << "SoundFile";
        fields << QString::number(s.flags) << s.soundFile;
        break;
    }
    case CkId::Type_Wthr_: {
        const auto& w = dynamic_cast<const Record<WthrRecord>&>(record).get();
        if (headers.size() == 3)
            headers << "Flags" << "SunTexture";
        fields << QString::number(w.flags) << w.sunTexture;
        break;
    }
    case CkId::Type_Ltex_: {
        const auto& t = dynamic_cast<const Record<LtexRecord>&>(record).get();
        if (headers.size() == 3)
            headers << "Flags" << "IconPath" << "HavokMaterial" << "GrassCount";
        fields << QString::number(t.flags) << t.iconPath
               << QString::number(t.havokMaterial) << QString::number(t.grassFormIds.size());
        break;
    }
    default:
        break;
    }

    return fields;
}
QDomElement DataExporter::recordToXML(const BaseRecord& record, CkId::Type type, QDomDocument& doc)
{
    QDomElement elem = doc.createElement("Record");
    elem.setAttribute("formId", QString("0x%1").arg(getFormId(record), 8, 16, QChar('0')));
    elem.setAttribute("editorId", getEditorId(record));
    elem.setAttribute("state", getStateString(record.state));

    auto addInt = [&](const QString& n, quint32 v) {
        QDomElement c = doc.createElement(n);
        c.setAttribute("value", v);
        elem.appendChild(c);
    };
    auto addFloat = [&](const QString& n, float v) {
        QDomElement c = doc.createElement(n);
        c.setAttribute("value", v);
        elem.appendChild(c);
    };
    auto addStr = [&](const QString& n, const QString& v) {
        QDomElement c = doc.createElement(n);
        c.setAttribute("value", v);
        elem.appendChild(c);
    };
    auto addBool = [&](const QString& n, bool v) {
        QDomElement c = doc.createElement(n);
        c.setAttribute("value", v ? "true" : "false");
        elem.appendChild(c);
    };
    auto addVecU32 = [&](const QString& n, const QVector<quint32>& vec) {
        QDomElement le = doc.createElement(n);
        for (const auto& v : vec) {
            QDomElement it = doc.createElement("item");
            it.setAttribute("value", v);
            le.appendChild(it);
        }
        elem.appendChild(le);
    };
    auto addVecStr = [&](const QString& n, const QVector<QString>& vec) {
        QDomElement le = doc.createElement(n);
        for (const auto& v : vec) {
            QDomElement it = doc.createElement("item");
            it.setAttribute("value", v);
            le.appendChild(it);
        }
        elem.appendChild(le);
    };

    switch (type) {
    case CkId::Type_Npc_: {
        const auto& npc = dynamic_cast<const Record<NpcRecord>&>(record).get();
        addInt("flags", npc.flags);
        addStr("fullName", npc.fullName);
        addInt("level", npc.level);
        addInt("health", npc.health);
        addInt("magicka", npc.magicka);
        addInt("stamina", npc.stamina);
        addInt("intelligence", npc.intelligence);
        addInt("race", npc.race);
        addInt("classId", npc.class_);
        addInt("faction", npc.faction);
        addInt("sex", npc.sex);
        addVecU32("spells", npc.spells);
        addVecU32("inventoryItems", npc.inventoryItems);
        addVecU32("relationships", npc.relationships);
        break;
    }
    case CkId::Type_Weap_: {
        const auto& w = dynamic_cast<const Record<WeaponRecord>&>(record).get();
        addInt("flags", w.flags);
        addStr("fullName", w.fullName);
        addInt("weaponType", w.weaponType);
        addFloat("damage", w.damage);
        addFloat("speed", w.speed);
        addFloat("reach", w.reach);
        addFloat("weight", w.weight);
        addInt("value", w.value);
        addInt("enchantment", w.enchantment);
        addStr("iconPath", w.iconPath);
        addStr("modelPath", w.modelPath);
        addInt("magicSchool", w.magicSchool);
        addInt("enchantLimit", w.enchantLimit);
        break;
    }
    case CkId::Type_Armor_: {
        const auto& a = dynamic_cast<const Record<ArmorRecord>&>(record).get();
        addInt("flags", a.flags);
        addStr("fullName", a.fullName);
        addInt("armorRating", a.armorRating);
        addFloat("weight", a.weight);
        addInt("value", a.value);
        addStr("iconPath", a.iconPath);
        addStr("modelPath", a.modelPath);
        addFloat("health", a.health);
        break;
    }
    case CkId::Type_Spel_: {
        const auto& s = dynamic_cast<const Record<SpellRecord>&>(record).get();
        addInt("flags", s.flags);
        addStr("fullName", s.fullName);
        addInt("cost", s.cost);
        addInt("castingSound", s.castingSound);
        addVecU32("effects", s.effects);
        addInt("enchantment", s.enchantment);
        break;
    }
    case CkId::Type_Magic_: {
        const auto& m = dynamic_cast<const Record<MagicRecord>&>(record).get();
        addInt("flags", m.flags);
        addInt("schools", m.schools);
        addInt("damageType", m.damageType);
        addInt("castingSound", m.castingSound);
        addStr("iconPath", m.iconPath);
        addStr("modelPath", m.modelPath);
        addVecU32("effects", m.effects);
        break;
    }
    case CkId::Type_Quest_: {
        const auto& q = dynamic_cast<const Record<QuestRecord>&>(record).get();
        addInt("flags", q.flags);
        addStr("questName", q.questName);
        addStr("questDesc", q.questDesc);
        addInt("questType", q.questType);
        addVecU32("stageIds", q.stageIds);
        addVecStr("stageDescriptions", q.stageDescriptions);
        addVecU32("objectiveIds", q.objectiveIds);
        addVecU32("aliasIds", q.aliasIds);
        addStr("dialogueView", q.dialogueView);
        addVecU32("scriptIds", q.scriptIds);
        break;
    }
    case CkId::Type_Dial_: {
        const auto& d = dynamic_cast<const Record<DialRecord>&>(record).get();
        addInt("flags", d.flags);
        addStr("topicName", d.topicName);
        addVecU32("responseIds", d.responseIds);
        addVecU32("conditionIds", d.conditionIds);
        addVecU32("animationIds", d.animationIds);
        addVecU32("emotionIds", d.emotionIds);
        break;
    }
    case CkId::Type_Info_: {
        const auto& i = dynamic_cast<const Record<InfoRecord>&>(record).get();
        addInt("flags", i.flags);
        addStr("responseText", i.responseText);
        addVecU32("conditionIds", i.conditionIds);
        addInt("targetId", i.targetId);
        addVecU32("scriptIds", i.scriptIds);
        break;
    }
    case CkId::Type_Glob_: {
        const auto& g = dynamic_cast<const Record<GlobalVariable>&>(record).get();
        addBool("constant", g.constant);
        QDomElement ve = doc.createElement("value");
        ve.setAttribute("type", g.value.getType());
        ve.setAttribute("data", g.value.getData().toString());
        elem.appendChild(ve);
        break;
    }
    case CkId::Type_Lcrt_: {
        const auto& l = dynamic_cast<const Record<LocationRefType>&>(record).get();
        addInt("color", l.color);
        break;
    }
    case CkId::Type_Pack_: {
        const auto& p = dynamic_cast<const Record<PackageRecord>&>(record).get();
        addInt("flags", p.flags);
        addInt("packageType", p.packageType);
        addInt("targetType", p.targetType);
        addVecU32("targetIds", p.targetIds);
        addVecU32("parameters", p.parameters);
        break;
    }
    case CkId::Type_Tree_: {
        const auto& t = dynamic_cast<const Record<TreeRecord>&>(record).get();
        addInt("flags", t.flags);
        addStr("iconPath", t.iconPath);
        addStr("modelPath", t.modelPath);
        addFloat("leafCurvature", t.leafCurvature);
        addFloat("leafAmplitude", t.leafAmplitude);
        addStr("lodModelPath", t.lodModelPath);
        addInt("lodFlags", t.lodFlags);
        break;
    }
    case CkId::Type_Alch_: {
        const auto& a = dynamic_cast<const Record<AlchRecord>&>(record).get();
        addInt("flags", a.flags);
        addStr("iconPath", a.iconPath);
        addStr("modelPath", a.modelPath);
        addFloat("weight", a.weight);
        addInt("value", a.value);
        break;
    }
    case CkId::Type_Ingr_: {
        const auto& i = dynamic_cast<const Record<IngrRecord>&>(record).get();
        addInt("flags", i.flags);
        addStr("iconPath", i.iconPath);
        addStr("modelPath", i.modelPath);
        addFloat("weight", i.weight);
        addInt("value", i.value);
        break;
    }
    case CkId::Type_Cont_: {
        const auto& c = dynamic_cast<const Record<ContRecord>&>(record).get();
        addInt("flags", c.flags);
        addStr("iconPath", c.iconPath);
        addStr("modelPath", c.modelPath);
        addInt("contents", c.contents);
        addInt("inventoryControl", c.inventoryControl);
        addFloat("weight", c.weight);
        addInt("value", c.value);
        break;
    }
    case CkId::Type_Ench_: {
        const auto& e = dynamic_cast<const Record<EnchRecord>&>(record).get();
        addInt("flags", e.flags);
        addStr("name", e.name);
        addInt("costLimit", e.costLimit);
        addInt("charges", e.charges);
        addInt("enchantmentData", e.enchantmentData);
        addFloat("charge", e.charge);
        addInt("duration", e.duration);
        addFloat("magnitude", e.magnitude);
        addInt("type", e.type);
        addInt("soulGem", e.soulGem);
        break;
    }
    case CkId::Type_Book_: {
        const auto& b = dynamic_cast<const Record<BookRecord>&>(record).get();
        addInt("flags", b.flags);
        addInt("pageCount", b.pageCount);
        addStr("pages", b.pages);
        addStr("iconPath", b.iconPath);
        addStr("modelPath", b.modelPath);
        break;
    }
    case CkId::Type_Misc_: {
        const auto& m = dynamic_cast<const Record<MiscRecord>&>(record).get();
        addInt("flags", m.flags);
        addStr("iconPath", m.iconPath);
        addStr("modelPath", m.modelPath);
        addFloat("weight", m.weight);
        addInt("value", m.value);
        break;
    }
    case CkId::Type_Acti_: {
        const auto& a = dynamic_cast<const Record<ActiRecord>&>(record).get();
        addInt("flags", a.flags);
        addStr("iconPath", a.iconPath);
        addStr("modelPath", a.modelPath);
        break;
    }
    case CkId::Type_Stat_: {
        const auto& s = dynamic_cast<const Record<StatRecord>&>(record).get();
        addInt("flags", s.flags);
        addStr("iconPath", s.iconPath);
        addStr("modelPath", s.modelPath);
        addStr("lodModelPath", s.lodModelPath);
        addInt("lodFlags", s.lodFlags);
        break;
    }
    case CkId::Type_Race_: {
        const auto& r = dynamic_cast<const Record<RaceRecord>&>(record).get();
        addInt("flags", r.flags);
        addInt("raceFlags", r.raceFlags);
        addVecU32("npcVariables", r.npcVariables);
        addVecU32("faceData", r.faceData);
        addVecU32("headData", r.headData);
        break;
    }
    case CkId::Type_Class_: {
        const auto& c = dynamic_cast<const Record<ClassRecord>&>(record).get();
        addInt("flags", c.flags);
        addStr("className", c.className);
        addStr("description", c.description);
        addInt("serviceFlags", c.serviceFlags);
        addStr("iconPath", c.iconPath);
        break;
    }
    case CkId::Type_Fact_: {
        const auto& f = dynamic_cast<const Record<FactRecord>&>(record).get();
        addInt("flags", f.flags);
        addStr("factionName", f.factionName);
        addStr("description", f.description);
        addStr("iconPath", f.iconPath);
        addVecStr("ranks", f.ranks);
        addVecU32("relations", f.relations);
        break;
    }
    case CkId::Type_PerK_: {
        const auto& p = dynamic_cast<const Record<PerkRecord>&>(record).get();
        addInt("flags", p.flags);
        addStr("description", p.description);
        addStr("requirements", p.requirements);
        addStr("iconPath", p.iconPath);
        addVecU32("conditions", p.conditions);
        break;
    }
    case CkId::Type_Cel_: {
        const auto& c = dynamic_cast<const Record<CellRecord>&>(record).get();
        addInt("flags", c.flags);
        addInt("cellX", c.cellX);
        addInt("cellY", c.cellY);
        addInt("owner", c.owner);
        addInt("lockLevel", c.lockLevel);
        addStr("cellName", c.cellName);
        break;
    }
    case CkId::Type_WRLD_: {
        const auto& w = dynamic_cast<const Record<WorldspaceRecord>&>(record).get();
        addInt("flags", w.flags);
        addStr("name", w.name);
        addInt("waterType", w.waterType);
        addInt("climateId", w.climateId);
        addInt("lightingId", w.lightingId);
        addInt("mapSize", w.mapSize);
        addInt("template", w.templ);
        addInt("terrain", w.terrain);
        addStr("mapImage", w.mapImage);
        addStr("lodNoise", w.lodNoise);
        addStr("billboardTexture", w.billboardTexture);
        addInt("music", w.music);
        addInt("dnam", w.dnam);
        addInt("dataMinX", w.dataMinX);
        addInt("dataMinY", w.dataMinY);
        addVecU32("cellIds", w.cellIds);
        addVecU32("navPointIds", w.navPointIds);
        break;
    }
    case CkId::Type_LOCT_: {
        const auto& l = dynamic_cast<const Record<LocationRecord>&>(record).get();
        addInt("flags", l.flags);
        addStr("locationName", l.locationName);
        addInt("parentId", l.parentId);
        addInt("x", l.x);
        addInt("y", l.y);
        addInt("z", l.z);
        break;
    }
    case CkId::Type_Refr_: {
        const auto& r = dynamic_cast<const Record<RefrRecord>&>(record).get();
        addInt("baseId", r.baseId);
        addFloat("posX", r.posX);
        addFloat("posY", r.posY);
        addFloat("posZ", r.posZ);
        addFloat("rotX", r.rotX);
        addFloat("rotY", r.rotY);
        addFloat("rotZ", r.rotZ);
        addFloat("scale", r.scale);
        addInt("owner", r.owner);
        addInt("lockLevel", r.lockLevel);
        addBool("initiallyDisabled", r.initiallyDisabled);
        addVecU32("scriptIds", r.scriptIds);
        break;
    }
    case CkId::Type_Material_: {
        const auto& m = dynamic_cast<const Record<MaterialRecord>&>(record).get();
        addInt("flags", m.flags);
        addStr("materialName", m.materialName);
        addStr("name", m.name);
        addStr("description", m.description);
        addStr("iconPath", m.iconPath);
        addStr("modelPath", m.modelPath);
        addStr("bnam", m.bnam);
        addStr("cnam", m.cnam);
        addStr("texturePath", m.texturePath);
        addInt("materialType", m.materialType);
        addInt("value", m.value);
        addInt("weight", m.weight);
        addInt("health", m.health);
        addInt("magicka", m.magicka);
        addInt("stamina", m.stamina);
        addInt("level", m.level);
        addInt("race", m.race);
        addInt("faction", m.faction);
        addInt("stage", m.stage);
        addInt("difficulty", m.difficulty);
        break;
    }
    case CkId::Type_Land_: {
        const auto& l = dynamic_cast<const Record<LandRecord>&>(record).get();
        addInt("flags", l.flags);
        addInt("cellX", l.cellX);
        addInt("cellY", l.cellY);
        addFloat("baseHeight", l.baseHeight);
        addBool("hasHeightData", l.hasHeightData);
        addBool("hasNormalData", l.hasNormalData);
        addBool("hasColorData", l.hasColorData);
        if (l.hasHeightData) {
            QDomElement hd = doc.createElement("heightData");
            hd.setAttribute("data", QByteArray::fromRawData(reinterpret_cast<const char*>(l.heightData), sizeof(l.heightData)).toBase64());
            elem.appendChild(hd);
        }
        if (l.hasNormalData) {
            QDomElement nd = doc.createElement("normalData");
            nd.setAttribute("data", QByteArray::fromRawData(reinterpret_cast<const char*>(l.normalData), sizeof(l.normalData)).toBase64());
            elem.appendChild(nd);
        }
        if (l.hasColorData) {
            QDomElement cd = doc.createElement("colorData");
            cd.setAttribute("data", QByteArray::fromRawData(reinterpret_cast<const char*>(l.colorData), sizeof(l.colorData)).toBase64());
            elem.appendChild(cd);
        }
        addInt("numTextureLayers", l.numTextureLayers);
        QDomElement tlElem = doc.createElement("textureLayers");
        for (int i = 0; i < l.numTextureLayers; ++i) {
            QDomElement tlItem = doc.createElement("item");
            tlItem.setAttribute("textureFormId", l.textureLayers[i].textureFormId);
            tlItem.setAttribute("opacity", l.textureLayers[i].opacity);
            tlElem.appendChild(tlItem);
        }
        elem.appendChild(tlElem);
        break;
    }
    case CkId::Type_Soun_: {
        const auto& s = dynamic_cast<const Record<SounRecord>&>(record).get();
        addInt("flags", s.flags);
        addStr("soundFile", s.soundFile);
        break;
    }
    case CkId::Type_Wthr_: {
        const auto& w = dynamic_cast<const Record<WthrRecord>&>(record).get();
        addInt("flags", w.flags);
        addStr("sunTexture", w.sunTexture);
        break;
    }
    case CkId::Type_Ltex_: {
        const auto& t = dynamic_cast<const Record<LtexRecord>&>(record).get();
        addInt("flags", t.flags);
        addStr("iconPath", t.iconPath);
        addInt("havokMaterial", t.havokMaterial);
        QDomElement grassElem = doc.createElement("grassFormIds");
        for (quint32 gid : t.grassFormIds) {
            QDomElement gItem = doc.createElement("item");
            gItem.setAttribute("id", gid);
            grassElem.appendChild(gItem);
        }
        elem.appendChild(grassElem);
        break;
    }
    default:
        break;
    }

    return elem;
}

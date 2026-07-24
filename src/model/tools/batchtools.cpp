#include "batchtools.hpp"

#include "../world/data.hpp"
#include "../world/collection.hpp"

#include <QRegularExpression>

template<typename ESXRecord>
static BatchTools::RenameResult batchRenameImpl(Collection<ESXRecord>& collection,
                                                 const QString& findPattern,
                                                 const QString& replaceWith,
                                                 bool useRegex)
{
    BatchTools::RenameResult result;

    QRegularExpression regex;
    if (useRegex)
    {
        regex = QRegularExpression(findPattern);
        if (!regex.isValid())
        {
            result.warnings << "Invalid regex: " + regex.errorString();
            return result;
        }
    }

    if (findPattern.isEmpty())
    {
        return result;
    }

    const auto& records = collection.getRecords();

    for (int i = 0; i < records.size(); ++i)
    {
        if (records[i].isErased() || records[i].isDeleted())
        {
            continue;
        }

        const ESXRecord& rec = records[i].get();
        QString oldId = rec.editorId;

        if (oldId.isEmpty())
        {
            result.warnings << "Skipping record with empty editorId at index " + QString::number(i);
            continue;
        }

        QString newId;
        if (useRegex)
        {
            newId = oldId;
            newId.replace(regex, replaceWith);
        }
        else
        {
            newId = oldId;
            newId.replace(findPattern, replaceWith);
        }

        if (newId != oldId)
        {
            collection.renameRecord(i, newId);
            result.recordsRenamed++;
        }
    }

    return result;
}

template<typename ESXRecord>
static void reassignFormIdsImpl(Collection<ESXRecord>& collection, quint32& currentFormId)
{
    for (int i = 0; i < collection.size(); ++i)
    {
        Record<ESXRecord>& rec = collection.getRecord(i);
        if (rec.isErased() || rec.isDeleted())
        {
            continue;
        }

        if constexpr (HasFormIdField<ESXRecord>::value)
        {
            quint32 oldFormId = rec.get().formId;
            quint32 pluginBits = oldFormId & 0xFFFF0000;
            rec.get().formId = pluginBits | (currentFormId & 0xFFFF);
            currentFormId++;
        }
    }
}

BatchTools::RenameResult BatchTools::batchRename(Data& data, CkId::Type type,
                                                   const QString& findPattern, const QString& replaceWith,
                                                   bool useRegex)
{
    BaseCollection* baseCol = data.getCollectionByType(type);
    if (!baseCol)
    {
        RenameResult result;
        result.warnings << "No collection found for type";
        return result;
    }

    // Dispatch to templated implementation based on record type
    switch (type)
    {
    case CkId::Type_Npc_:
        return batchRenameImpl<NpcRecord>(
            *static_cast<Collection<NpcRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Weap_:
        return batchRenameImpl<WeaponRecord>(
            *static_cast<Collection<WeaponRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Armor_:
        return batchRenameImpl<ArmorRecord>(
            *static_cast<Collection<ArmorRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Spel_:
        return batchRenameImpl<SpellRecord>(
            *static_cast<Collection<SpellRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Magic_:
        return batchRenameImpl<MagicRecord>(
            *static_cast<Collection<MagicRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Quest_:
        return batchRenameImpl<QuestRecord>(
            *static_cast<Collection<QuestRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Dial_:
        return batchRenameImpl<DialRecord>(
            *static_cast<Collection<DialRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Info_:
        return batchRenameImpl<InfoRecord>(
            *static_cast<Collection<InfoRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Glob_:
        return batchRenameImpl<GlobalVariable>(
            *static_cast<Collection<GlobalVariable>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Lcrt_:
        return batchRenameImpl<LocationRefType>(
            *static_cast<Collection<LocationRefType>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Pack_:
        return batchRenameImpl<PackageRecord>(
            *static_cast<Collection<PackageRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Tree_:
        return batchRenameImpl<TreeRecord>(
            *static_cast<Collection<TreeRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Alch_:
        return batchRenameImpl<AlchRecord>(
            *static_cast<Collection<AlchRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Ingr_:
        return batchRenameImpl<IngrRecord>(
            *static_cast<Collection<IngrRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Cont_:
        return batchRenameImpl<ContRecord>(
            *static_cast<Collection<ContRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Ench_:
        return batchRenameImpl<EnchRecord>(
            *static_cast<Collection<EnchRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Book_:
        return batchRenameImpl<BookRecord>(
            *static_cast<Collection<BookRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Misc_:
        return batchRenameImpl<MiscRecord>(
            *static_cast<Collection<MiscRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Acti_:
        return batchRenameImpl<ActiRecord>(
            *static_cast<Collection<ActiRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Stat_:
        return batchRenameImpl<StatRecord>(
            *static_cast<Collection<StatRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Race_:
        return batchRenameImpl<RaceRecord>(
            *static_cast<Collection<RaceRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Class_:
        return batchRenameImpl<ClassRecord>(
            *static_cast<Collection<ClassRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Fact_:
        return batchRenameImpl<FactRecord>(
            *static_cast<Collection<FactRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_PerK_:
        return batchRenameImpl<PerkRecord>(
            *static_cast<Collection<PerkRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Cel_:
        return batchRenameImpl<CellRecord>(
            *static_cast<Collection<CellRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_WRLD_:
        return batchRenameImpl<WorldspaceRecord>(
            *static_cast<Collection<WorldspaceRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_LOCT_:
        return batchRenameImpl<LocationRecord>(
            *static_cast<Collection<LocationRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Refr_:
        return batchRenameImpl<RefrRecord>(
            *static_cast<Collection<RefrRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    case CkId::Type_Material_:
        return batchRenameImpl<MaterialRecord>(
            *static_cast<Collection<MaterialRecord>*>(baseCol), findPattern, replaceWith, useRegex);
    default:
    {
        RenameResult result;
        result.warnings << "Unsupported record type for batch rename";
        return result;
    }
    }
}

BatchTools::FormIdResult BatchTools::batchReassignFormIds(Data& data, quint32 startFormId)
{
    FormIdResult result;
    quint32 currentFormId = startFormId;

    auto collections = data.allCollectionsWithTypes();

    for (auto& tc : collections)
    {
        BaseCollection* baseCol = static_cast<BaseCollection*>(tc.collection);
        if (!baseCol || baseCol->count() == 0)
        {
            continue;
        }

        // Dispatch to templated implementation
        switch (tc.type)
        {
        case CkId::Type_Npc_:
            reassignFormIdsImpl<NpcRecord>(
                *static_cast<Collection<NpcRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Weap_:
            reassignFormIdsImpl<WeaponRecord>(
                *static_cast<Collection<WeaponRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Armor_:
            reassignFormIdsImpl<ArmorRecord>(
                *static_cast<Collection<ArmorRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Spel_:
            reassignFormIdsImpl<SpellRecord>(
                *static_cast<Collection<SpellRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Magic_:
            reassignFormIdsImpl<MagicRecord>(
                *static_cast<Collection<MagicRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Quest_:
            reassignFormIdsImpl<QuestRecord>(
                *static_cast<Collection<QuestRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Dial_:
            reassignFormIdsImpl<DialRecord>(
                *static_cast<Collection<DialRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Info_:
            reassignFormIdsImpl<InfoRecord>(
                *static_cast<Collection<InfoRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Glob_:
            reassignFormIdsImpl<GlobalVariable>(
                *static_cast<Collection<GlobalVariable>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Lcrt_:
            reassignFormIdsImpl<LocationRefType>(
                *static_cast<Collection<LocationRefType>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Pack_:
            reassignFormIdsImpl<PackageRecord>(
                *static_cast<Collection<PackageRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Tree_:
            reassignFormIdsImpl<TreeRecord>(
                *static_cast<Collection<TreeRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Alch_:
            reassignFormIdsImpl<AlchRecord>(
                *static_cast<Collection<AlchRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Ingr_:
            reassignFormIdsImpl<IngrRecord>(
                *static_cast<Collection<IngrRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Cont_:
            reassignFormIdsImpl<ContRecord>(
                *static_cast<Collection<ContRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Ench_:
            reassignFormIdsImpl<EnchRecord>(
                *static_cast<Collection<EnchRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Book_:
            reassignFormIdsImpl<BookRecord>(
                *static_cast<Collection<BookRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Misc_:
            reassignFormIdsImpl<MiscRecord>(
                *static_cast<Collection<MiscRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Acti_:
            reassignFormIdsImpl<ActiRecord>(
                *static_cast<Collection<ActiRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Stat_:
            reassignFormIdsImpl<StatRecord>(
                *static_cast<Collection<StatRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Race_:
            reassignFormIdsImpl<RaceRecord>(
                *static_cast<Collection<RaceRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Class_:
            reassignFormIdsImpl<ClassRecord>(
                *static_cast<Collection<ClassRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Fact_:
            reassignFormIdsImpl<FactRecord>(
                *static_cast<Collection<FactRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_PerK_:
            reassignFormIdsImpl<PerkRecord>(
                *static_cast<Collection<PerkRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Cel_:
            reassignFormIdsImpl<CellRecord>(
                *static_cast<Collection<CellRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_WRLD_:
            reassignFormIdsImpl<WorldspaceRecord>(
                *static_cast<Collection<WorldspaceRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_LOCT_:
            reassignFormIdsImpl<LocationRecord>(
                *static_cast<Collection<LocationRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Refr_:
            reassignFormIdsImpl<RefrRecord>(
                *static_cast<Collection<RefrRecord>*>(baseCol), currentFormId);
            break;
        case CkId::Type_Material_:
            reassignFormIdsImpl<MaterialRecord>(
                *static_cast<Collection<MaterialRecord>*>(baseCol), currentFormId);
            break;
        default:
            break;
        }
    }

    result.formIdsReassigned = static_cast<int>(currentFormId - startFormId);
    return result;
}

#include "bashedpatchgenerator.hpp"

#include "../../model/world/data.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"
#include "../../../libs/files/esm/tes4.hpp"
#include "../../../libs/files/esm/records.hpp"

#include <QFile>
#include <QFileInfo>
#include <QSet>

BashedPatchGenerator::BashedPatchGenerator(Data* data)
    : mData(data)
{
}

bool BashedPatchGenerator::generatePatch(const QString& outputPath, const PatchConfig& config)
{
    mergedPlugins.clear();
    mergedRecordCount = 0;
    patchLog.clear();

    if (!mData)
    {
        patchLog += "Error: No data loaded.\n";
        return false;
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        patchLog += "Error: Cannot open output file: " + outputPath + "\n";
        return false;
    }

    ESMWriter esmWriter;
    esmWriter.setVersion(0.94f);
    esmWriter.setAuthor("OpenCK Bashed Patch Generator");
    esmWriter.setDescription("Auto-generated bashed patch");

    // First pass: count total records to merge
    int totalRecords = 0;

    if (config.mergeNPCs)
        totalRecords += mData->getNpcCollection().size();
    if (config.mergeWeapons)
        totalRecords += mData->getWeaponCollection().size();
    if (config.mergeArmor)
        totalRecords += mData->getArmorCollection().size();
    if (config.mergeSpells)
        totalRecords += mData->getSpellCollection().size();
    if (config.mergeAlchemy)
        totalRecords += mData->getAlchCollection().size();
    if (config.mergeIngredients)
        totalRecords += mData->getIngrCollection().size();
    if (config.mergeBooks)
        totalRecords += mData->getBookCollection().size();
    if (config.mergeEnchantments)
        totalRecords += mData->getEnchCollection().size();
    if (config.mergeContainers)
        totalRecords += mData->getContCollection().size();
    if (config.mergeMisc)
        totalRecords += mData->getMiscCollection().size();
    if (config.mergeActivators)
        totalRecords += mData->getActiCollection().size();
    if (config.mergeRace)
        totalRecords += mData->getRaceCollection().size();
    if (config.mergeClass)
        totalRecords += mData->getClassCollection().size();
    if (config.mergeQuest)
        totalRecords += mData->getQuestCollection().size();
    if (config.mergePackage)
        totalRecords += mData->getPackCollection().size();
    if (config.mergeFact)
        totalRecords += mData->getFactCollection().size();
    if (config.mergePerk)
        totalRecords += mData->getPerkCollection().size();

    patchLog += "Total records in collections: " + QString::number(totalRecords) + "\n";

    // Collect records to merge
    QVector<NpcRecord> npcRecords;
    QVector<WeaponRecord> weaponRecords;
    QVector<ArmorRecord> armorRecords;
    QVector<SpellRecord> spellRecords;
    QVector<AlchRecord> alchRecords;
    QVector<IngrRecord> ingrRecords;
    QVector<BookRecord> bookRecords;
    QVector<EnchRecord> enchRecords;
    QVector<ContRecord> contRecords;
    QVector<MiscRecord> miscRecords;
    QVector<ActiRecord> actiRecords;
    QVector<RaceRecord> raceRecords;
    QVector<ClassRecord> classRecords;
    QVector<QuestRecord> questRecords;
    QVector<PackageRecord> packRecords;
    QVector<FactRecord> factRecords;
    QVector<PerkRecord> perkRecords;

    if (config.mergeNPCs)
        mergeCollection<NpcRecord>("NPC", mData->getNpcCollection(), npcRecords);
    if (config.mergeWeapons)
        mergeCollection<WeaponRecord>("WEAP", mData->getWeaponCollection(), weaponRecords);
    if (config.mergeArmor)
        mergeCollection<ArmorRecord>("ARMO", mData->getArmorCollection(), armorRecords);
    if (config.mergeSpells)
        mergeCollection<SpellRecord>("SPEL", mData->getSpellCollection(), spellRecords);
    if (config.mergeAlchemy)
        mergeCollection<AlchRecord>("ALCH", mData->getAlchCollection(), alchRecords);
    if (config.mergeIngredients)
        mergeCollection<IngrRecord>("INGR", mData->getIngrCollection(), ingrRecords);
    if (config.mergeBooks)
        mergeCollection<BookRecord>("BOOK", mData->getBookCollection(), bookRecords);
    if (config.mergeEnchantments)
        mergeCollection<EnchRecord>("ENCH", mData->getEnchCollection(), enchRecords);
    if (config.mergeContainers)
        mergeCollection<ContRecord>("CONT", mData->getContCollection(), contRecords);
    if (config.mergeMisc)
        mergeCollection<MiscRecord>("MISC", mData->getMiscCollection(), miscRecords);
    if (config.mergeActivators)
        mergeCollection<ActiRecord>("ACTI", mData->getActiCollection(), actiRecords);
    if (config.mergeRace)
        mergeCollection<RaceRecord>("RACE", mData->getRaceCollection(), raceRecords);
    if (config.mergeClass)
        mergeCollection<ClassRecord>("CLAS", mData->getClassCollection(), classRecords);
    if (config.mergeQuest)
        mergeCollection<QuestRecord>("QUST", mData->getQuestCollection(), questRecords);
    if (config.mergePackage)
        mergeCollection<PackageRecord>("PACK", mData->getPackCollection(), packRecords);
    if (config.mergeFact)
        mergeCollection<FactRecord>("FACT", mData->getFactCollection(), factRecords);
    if (config.mergePerk)
        mergeCollection<PerkRecord>("PERK", mData->getPerkCollection(), perkRecords);

    patchLog += "Records to write: " + QString::number(mergedRecordCount) + "\n";

    // Write the patch file
    esmWriter.setNumRecords(mergedRecordCount);
    esmWriter.clearMasters();
    esmWriter.save(file);

    // Write merged records
    for (const auto& rec : npcRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('NPC_', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : weaponRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('WEAP', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : armorRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('ARMO', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : spellRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('SPEL', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : alchRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('ALCH', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : ingrRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('INGR', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : bookRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('BOOK', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : enchRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('ENCH', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : contRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('CONT', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : miscRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('MISC', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : actiRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('ACTI', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : raceRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('RACE', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : classRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('CLAS', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : questRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('QUST', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : packRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('PACK', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : factRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('FACT', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    for (const auto& rec : perkRecords)
    {
        RecHeader header;
        header.id = rec.formId;
        esmWriter.startRecord('PERK', header);
        rec.save(esmWriter);
        esmWriter.endRecord();
    }

    esmWriter.close();
    file.close();

    patchLog += "Patch generated successfully: " + outputPath + "\n";
    patchLog += "Total records merged: " + QString::number(mergedRecordCount) + "\n";

    return true;
}

QVector<QString> BashedPatchGenerator::getMergedPluginList() const
{
    return mergedPlugins;
}

int BashedPatchGenerator::getMergedRecordCount() const
{
    return mergedRecordCount;
}

QString BashedPatchGenerator::getPatchLog() const
{
    return patchLog;
}

template<typename RecordT, typename CollectionT>
int BashedPatchGenerator::mergeCollection(const QString& typeName, CollectionT& collection,
                                           QVector<RecordT>& outputRecords)
{
    int added = 0;
    QSet<QString> seenEditorIds;
    QStringList files = mData->getContentFiles();

    const auto& records = collection.getRecords();
    for (int i = 0; i < records.size(); ++i)
    {
        const auto& record = records[i];

        if (record.isErased() || record.isDeleted())
        {
            continue;
        }

        const RecordT& data = record.get();
        QString editorId = data.editorId.toLower();

        if (editorId.isEmpty())
        {
            continue;
        }

        if (seenEditorIds.contains(editorId))
        {
            patchLog += "  " + typeName + ": Skipped duplicate: " + data.editorId + "\n";
            continue;
        }

        seenEditorIds.insert(editorId);
        outputRecords.append(data);
        ++added;
        ++mergedRecordCount;

        int pluginIndex = (data.formId >> 16) & 0xFFFF;
        if (pluginIndex > 0 && pluginIndex - 1 < files.size())
        {
            QString pluginName = files[pluginIndex - 1];
            if (!mergedPlugins.contains(pluginName))
            {
                mergedPlugins.append(pluginName);
            }
        }
    }

    patchLog += typeName + ": Merged " + QString::number(added) + " records\n";
    return added;
}

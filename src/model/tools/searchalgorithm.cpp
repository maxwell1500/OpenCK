#include "searchalgorithm.hpp"

#include <QRegularExpression>

template<typename Collection, typename NameFunc>
static std::tuple<QVector<QString>, QVector<QString>, QVector<QString>>
getCollectionData(const Collection& collection, NameFunc getName)
{
    QVector<QString> ids, formIds, names;
    for (int i = 0; i < collection.size(); i++)
    {
        ids.append(collection.getId(i));
        formIds.append(QString("0x%1").arg(collection.getRecord(i).get().formId, 8, 16, QChar('0')));
        names.append(getName(collection.getRecord(i).get()));
    }
    return std::make_tuple(ids, formIds, names);
}

QVector<SearchAlgorithm::SearchResult> SearchAlgorithm::search(const Data& data, const QString& text,
                                                               const QString& field, CkId::Type typeFilter)
{
    SearchCriteria criteria;
    criteria.text = text;
    criteria.field = field;
    criteria.typeFilter = typeFilter;
    criteria.matchMode = MatchMode::Contains;
    criteria.caseSensitive = false;
    
    return searchAdvanced(data, criteria);
}

QVector<SearchAlgorithm::SearchResult> SearchAlgorithm::searchAdvanced(const Data& data, const SearchCriteria& criteria)
{
    QVector<SearchResult> results;
    
    auto processCollection = [&](const QString& typeName, CkId::Type type,
                                   const QVector<QString>& ids,
                                   const QVector<QString>& formIds,
                                   const QVector<QString>& names,
                                   int collectionIndex) {
        if (criteria.typeFilter != CkId::Type_None && criteria.typeFilter != type)
        {
            return;
        }
        
        auto collectionResults = searchCollection(criteria, type, ids, formIds, names, collectionIndex);
        results.append(collectionResults);
    };

    auto edid = [](const auto& r) { return r.editorId; };
    auto fullName = [](const auto& r) { return r.fullName; };
    auto topicName = [](const auto& r) { return r.topicName; };
    auto responseText = [](const auto& r) { return r.responseText; };
    auto questName = [](const auto& r) { return r.questName; };
    auto className = [](const auto& r) { return r.className; };
    auto factionName = [](const auto& r) { return r.factionName; };

    auto npcData = getCollectionData(data.getNpcCollection(), fullName);
    processCollection("NPC", CkId::Type_Npc_,
                      std::get<0>(npcData), std::get<1>(npcData), std::get<2>(npcData), 0);

    auto weapData = getCollectionData(data.getWeaponCollection(), edid);
    processCollection("Weapon", CkId::Type_Weap_,
                      std::get<0>(weapData), std::get<1>(weapData), std::get<2>(weapData), 1);

    auto armorData = getCollectionData(data.getArmorCollection(), edid);
    processCollection("Armor", CkId::Type_Armor_,
                      std::get<0>(armorData), std::get<1>(armorData), std::get<2>(armorData), 2);

    auto spellData = getCollectionData(data.getSpellCollection(), edid);
    processCollection("Spell", CkId::Type_Spel_,
                      std::get<0>(spellData), std::get<1>(spellData), std::get<2>(spellData), 3);

    auto magicData = getCollectionData(data.getMagicCollection(), edid);
    processCollection("Magic Effect", CkId::Type_Magic_,
                      std::get<0>(magicData), std::get<1>(magicData), std::get<2>(magicData), 4);

    auto questData = getCollectionData(data.getQuestCollection(), questName);
    processCollection("Quest", CkId::Type_Quest_,
                      std::get<0>(questData), std::get<1>(questData), std::get<2>(questData), 5);

    auto dialData = getCollectionData(data.getDialCollection(), topicName);
    processCollection("Dialogue", CkId::Type_Dial_,
                      std::get<0>(dialData), std::get<1>(dialData), std::get<2>(dialData), 6);

    auto infoData = getCollectionData(data.getInfoCollection(), responseText);
    processCollection("Information", CkId::Type_Info_,
                      std::get<0>(infoData), std::get<1>(infoData), std::get<2>(infoData), 7);

    auto packData = getCollectionData(data.getPackCollection(), edid);
    processCollection("Package", CkId::Type_Pack_,
                      std::get<0>(packData), std::get<1>(packData), std::get<2>(packData), 8);

    auto alchData = getCollectionData(data.getAlchCollection(), edid);
    processCollection("Alchemy", CkId::Type_Alch_,
                      std::get<0>(alchData), std::get<1>(alchData), std::get<2>(alchData), 9);

    auto ingrData = getCollectionData(data.getIngrCollection(), edid);
    processCollection("Ingredient", CkId::Type_Ingr_,
                      std::get<0>(ingrData), std::get<1>(ingrData), std::get<2>(ingrData), 10);

    auto contData = getCollectionData(data.getContCollection(), edid);
    processCollection("Container", CkId::Type_Cont_,
                      std::get<0>(contData), std::get<1>(contData), std::get<2>(contData), 11);

    auto enchData = getCollectionData(data.getEnchCollection(), edid);
    processCollection("Enchantment", CkId::Type_Ench_,
                      std::get<0>(enchData), std::get<1>(enchData), std::get<2>(enchData), 12);

    auto bookData = getCollectionData(data.getBookCollection(), edid);
    processCollection("Book", CkId::Type_Book_,
                      std::get<0>(bookData), std::get<1>(bookData), std::get<2>(bookData), 13);

    auto miscData = getCollectionData(data.getMiscCollection(), edid);
    processCollection("Miscellaneous", CkId::Type_Misc_,
                      std::get<0>(miscData), std::get<1>(miscData), std::get<2>(miscData), 14);

    auto actiData = getCollectionData(data.getActiCollection(), edid);
    processCollection("Activator", CkId::Type_Acti_,
                      std::get<0>(actiData), std::get<1>(actiData), std::get<2>(actiData), 15);

    auto raceData = getCollectionData(data.getRaceCollection(), edid);
    processCollection("Race", CkId::Type_Race_,
                      std::get<0>(raceData), std::get<1>(raceData), std::get<2>(raceData), 16);

    auto classData = getCollectionData(data.getClassCollection(), className);
    processCollection("Class", CkId::Type_Class_,
                      std::get<0>(classData), std::get<1>(classData), std::get<2>(classData), 17);

    auto factData = getCollectionData(data.getFactCollection(), factionName);
    processCollection("Faction", CkId::Type_Fact_,
                      std::get<0>(factData), std::get<1>(factData), std::get<2>(factData), 18);

    auto perkData = getCollectionData(data.getPerkCollection(), edid);
    processCollection("Perk", CkId::Type_PerK_,
                      std::get<0>(perkData), std::get<1>(perkData), std::get<2>(perkData), 19);

    auto cellData = getCollectionData(data.getCellCollection(), edid);
    processCollection("Cell", CkId::Type_Cel_,
                      std::get<0>(cellData), std::get<1>(cellData), std::get<2>(cellData), 20);

    auto wrldData = getCollectionData(data.getWorldspaceCollection(), edid);
    processCollection("Worldspace", CkId::Type_WRLD_,
                      std::get<0>(wrldData), std::get<1>(wrldData), std::get<2>(wrldData), 21);

    auto loctData = getCollectionData(data.getLocationCollection(), edid);
    processCollection("Location", CkId::Type_LOCT_,
                      std::get<0>(loctData), std::get<1>(loctData), std::get<2>(loctData), 22);

    auto refrData = getCollectionData(data.getRefrCollection(), edid);
    processCollection("Reference", CkId::Type_Refr_,
                      std::get<0>(refrData), std::get<1>(refrData), std::get<2>(refrData), 23);

    return results;
}

bool SearchAlgorithm::matchesCriteria(const QString& value, const QString& searchText, MatchMode mode, bool caseSensitive)
{
    if (searchText.isEmpty()) return true;
    if (value.isEmpty()) return false;
    
    QString val = caseSensitive ? value : value.toLower();
    QString search = caseSensitive ? searchText : searchText.toLower();
    
    switch (mode)
    {
    case MatchMode::Contains:
        return val.contains(search);
    case MatchMode::StartsWith:
        return val.startsWith(search);
    case MatchMode::EndsWith:
        return val.endsWith(search);
    case MatchMode::Exact:
        return val == search;
    case MatchMode::Regex:
    {
        QRegularExpression regex(search, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
        return regex.match(val).hasMatch();
    }
    }
    return false;
}

bool SearchAlgorithm::matchesAllCriteria(const QVector<QString>& ids, const QVector<QString>& formIds, const QVector<QString>& names,
                                         const SearchCriteria& criteria)
{
    if (criteria.additionalCriteria.isEmpty())
    {
        QString field = criteria.field;
        QString text = criteria.text;
        MatchMode mode = criteria.matchMode;
        bool caseSensitive = criteria.caseSensitive;
        
        if (field == "EditorID" || field == "Editor ID" || field == "id")
        {
            return matchesCriteria(ids.isEmpty() ? QString() : ids[0], text, mode, caseSensitive);
        }
        else if (field == "FormID" || field == "Form ID" || field == "formid")
        {
            return matchesCriteria(formIds.isEmpty() ? QString() : formIds[0], text, mode, caseSensitive);
        }
        else if (field == "Name" || field == "name")
        {
            return matchesCriteria(names.isEmpty() ? QString() : names[0], text, mode, caseSensitive);
        }
        else
        {
            bool idMatch = matchesCriteria(ids.isEmpty() ? QString() : ids[0], text, mode, caseSensitive);
            bool formMatch = matchesCriteria(formIds.isEmpty() ? QString() : formIds[0], text, mode, caseSensitive);
            bool nameMatch = matchesCriteria(names.isEmpty() ? QString() : names[0], text, mode, caseSensitive);
            return idMatch || formMatch || nameMatch;
        }
    }
    
    if (criteria.allCriteriaMustMatch)
    {
        for (const auto& criterion : criteria.additionalCriteria)
        {
            QString field = criterion.field;
            QString value = criterion.value;
            MatchMode mode = criterion.mode;
            bool caseSensitive = criterion.caseSensitive;
            
            QString fieldValue;
            if (field == "EditorID" || field == "Editor ID" || field == "id")
            {
                fieldValue = ids.isEmpty() ? QString() : ids[0];
            }
            else if (field == "FormID" || field == "Form ID" || field == "formid")
            {
                fieldValue = formIds.isEmpty() ? QString() : formIds[0];
            }
            else if (field == "Name" || field == "name")
            {
                fieldValue = names.isEmpty() ? QString() : names[0];
            }
            
            if (!matchesCriteria(fieldValue, value, mode, caseSensitive))
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        for (const auto& criterion : criteria.additionalCriteria)
        {
            QString field = criterion.field;
            QString value = criterion.value;
            MatchMode mode = criterion.mode;
            bool caseSensitive = criterion.caseSensitive;
            
            QString fieldValue;
            if (field == "EditorID" || field == "Editor ID" || field == "id")
            {
                fieldValue = ids.isEmpty() ? QString() : ids[0];
            }
            else if (field == "FormID" || field == "Form ID" || field == "formid")
            {
                fieldValue = formIds.isEmpty() ? QString() : formIds[0];
            }
            else if (field == "Name" || field == "name")
            {
                fieldValue = names.isEmpty() ? QString() : names[0];
            }
            
            if (matchesCriteria(fieldValue, value, mode, caseSensitive))
            {
                return true;
            }
        }
        return false;
    }
}

QVector<SearchAlgorithm::SearchResult> SearchAlgorithm::searchCollection(
    const SearchCriteria& criteria, CkId::Type type,
    const QVector<QString>& ids, const QVector<QString>& formIds,
    const QVector<QString>& names, int collectionIndex)
{
    QVector<SearchResult> results;
    
    if (ids.isEmpty()) return results;
    
    for (int i = 0; i < ids.size(); i++)
    {
        QVector<QString> singleIds = {ids[i]};
        QVector<QString> singleFormIds = {formIds[i]};
        QVector<QString> singleNames = {names[i]};
        
        if (matchesAllCriteria(singleIds, singleFormIds, singleNames, criteria))
        {
            SearchResult result;
            result.editorId = ids[i];
            result.formId = formIds[i];
            result.type = type;
            result.collectionIndex = collectionIndex;
            result.recordIndex = i;
            results.append(result);
        }
    }
    
    return results;
}

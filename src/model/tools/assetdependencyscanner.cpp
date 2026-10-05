#include "assetdependencyscanner.hpp"
#include "assetresolver.hpp"
#include "logger.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSet>

#include "../../../libs/files/esm/statrecord.hpp"
#include "../../../libs/files/esm/weaprecord.hpp"
#include "../../../libs/files/esm/armorrecord.hpp"
#include "../../../libs/files/esm/bookrecord.hpp"
#include "../../../libs/files/esm/miscrecord.hpp"
#include "../../../libs/files/esm/ingrrecord.hpp"
#include "../../../libs/files/esm/alchrecord.hpp"
#include "../../../libs/files/esm/contrecord.hpp"
#include "../../../libs/files/esm/actirecord.hpp"
#include "../../../libs/files/esm/treerecord.hpp"
#include "../world/collection.hpp"

// ============================================================================
// ScanResult
// ============================================================================

// ============================================================================
// Helpers
// ============================================================================

int AssetDependencyScanner::levenshteinDistance(const QString& s1, const QString& s2)
{
    const int len1 = s1.size();
    const int len2 = s2.size();
    QVector<QVector<int>> d(len1 + 1, QVector<int>(len2 + 1));

    for (int i = 0; i <= len1; i++)
        d[i][0] = i;
    for (int j = 0; j <= len2; j++)
        d[0][j] = j;

    for (int i = 1; i <= len1; i++)
    {
        for (int j = 1; j <= len2; j++)
        {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = qMin(qMin(d[i - 1][j] + 1, d[i][j - 1] + 1), d[i - 1][j - 1] + cost);
        }
    }
    return d[len1][len2];
}

QString AssetDependencyScanner::typeName(CkId::Type type)
{
    switch (type)
    {
    case CkId::Type_Npc_:   return "NPC";
    case CkId::Type_Weap_:  return "Weapon";
    case CkId::Type_Armor_: return "Armor";
    case CkId::Type_Book_:  return "Book";
    case CkId::Type_Misc_:  return "Misc";
    case CkId::Type_Ingr_:  return "Ingredient";
    case CkId::Type_Alch_:  return "Alchemy";
    case CkId::Type_Cont_:  return "Container";
    case CkId::Type_Acti_:  return "Activator";
    case CkId::Type_Tree_:  return "Tree";
    case CkId::Type_Stat_:  return "Static";
    default:                return "Unknown";
    }
}

// ============================================================================
// findSimilarPaths
// ============================================================================

QStringList AssetDependencyScanner::findSimilarPaths(const QString& path, const QString& dataDir, int maxResults)
{
    if (path.isEmpty())
        return {};

    AssetResolver resolver(dataDir);
    return findSimilarPathsFrom(resolver, path, maxResults);
}

QStringList AssetDependencyScanner::findSimilarPathsFrom(const AssetResolver& resolver,
                                                         const QString& path, int maxResults)
{
    if (path.isEmpty())
        return {};

    const QStringList& fileIndex = resolver.allPaths();
    if (fileIndex.isEmpty())
        return {};

    QString lowerPath = path.toLower();
    QString fileName = QFileInfo(path).fileName().toLower();

    // Score each file by similarity
    QVector<QPair<int, QString>> scored;
    for (const auto& candidate : fileIndex)
    {
        QString lowerCandidate = candidate.toLower();
        QString candidateName = QFileInfo(candidate).fileName().toLower();

        int score = 0;

        // Exact filename match (just different directory)
        if (candidateName == fileName)
        {
            score = 1000;
        }
        else
        {
            // Levenshtein distance on filename
            int dist = levenshteinDistance(fileName, candidateName);
            int maxLen = qMax(fileName.size(), candidateName.size());
            score = maxLen - dist;

            // Bonus for same extension
            if (QFileInfo(path).suffix().toLower() == QFileInfo(candidate).suffix().toLower())
                score += 10;

            // Bonus for substring match
            if (lowerCandidate.contains(fileName))
                score += 50;
        }

        scored.append({score, candidate});
    }

    // Sort by score descending
    std::sort(scored.begin(), scored.end(),
              [](const QPair<int, QString>& a, const QPair<int, QString>& b) {
                  return a.first > b.first;
              });

    QStringList results;
    for (int i = 0; i < qMin(maxResults, scored.size()); i++)
    {
        if (scored[i].first > 0)
            results.append(scored[i].second);
    }

    return results;
}

// ============================================================================
// scanAll
// ============================================================================

QVector<AssetReference> AssetDependencyScanner::collectReferences(const Data& data)
{
    QVector<AssetReference> refs;
    const auto add = [&refs](const QString& recordId, CkId::Type type,
                             const QString& modelPath, const QString& iconPath) {
        if (!modelPath.isEmpty())
            refs.append({ recordId, type, modelPath, QStringLiteral("model") });
        if (!iconPath.isEmpty())
            refs.append({ recordId, type, iconPath, QStringLiteral("texture") });
    };

    const auto& statCollection = data.getStatCollection();
    for (int i = 0; i < statCollection.size(); i++)
    {
        const auto& rec = statCollection.getRecord(i).get();
        add(rec.editorId, CkId::Type_Stat_, rec.modelPath, rec.iconPath);
    }
    const auto& weapCollection = data.getWeaponCollection();
    for (int i = 0; i < weapCollection.size(); i++)
    {
        const auto& rec = weapCollection.getRecord(i).get();
        add(rec.editorId, CkId::Type_Weap_, rec.modelPath, rec.iconPath);
    }
    const auto& armorCollection = data.getArmorCollection();
    for (int i = 0; i < armorCollection.size(); i++)
    {
        const auto& rec = armorCollection.getRecord(i).get();
        add(rec.editorId, CkId::Type_Armor_, rec.modelPath, rec.iconPath);
    }
    const auto& bookCollection = data.getBookCollection();
    for (int i = 0; i < bookCollection.size(); i++)
    {
        const auto& rec = bookCollection.getRecord(i).get();
        add(rec.editorId, CkId::Type_Book_, rec.modelPath, rec.iconPath);
    }
    const auto& miscCollection = data.getMiscCollection();
    for (int i = 0; i < miscCollection.size(); i++)
    {
        const auto& rec = miscCollection.getRecord(i).get();
        add(rec.editorId, CkId::Type_Misc_, rec.modelPath, rec.iconPath);
    }
    const auto& ingrCollection = data.getIngrCollection();
    for (int i = 0; i < ingrCollection.size(); i++)
    {
        const auto& rec = ingrCollection.getRecord(i).get();
        add(rec.editorId, CkId::Type_Ingr_, rec.modelPath, rec.iconPath);
    }
    const auto& alchCollection = data.getAlchCollection();
    for (int i = 0; i < alchCollection.size(); i++)
    {
        const auto& rec = alchCollection.getRecord(i).get();
        add(rec.editorId, CkId::Type_Alch_, rec.modelPath, rec.iconPath);
    }
    const auto& contCollection = data.getContCollection();
    for (int i = 0; i < contCollection.size(); i++)
    {
        const auto& rec = contCollection.getRecord(i).get();
        add(rec.editorId, CkId::Type_Cont_, rec.modelPath, rec.iconPath);
    }
    const auto& actiCollection = data.getActiCollection();
    for (int i = 0; i < actiCollection.size(); i++)
    {
        const auto& rec = actiCollection.getRecord(i).get();
        add(rec.editorId, CkId::Type_Acti_, rec.modelPath, rec.iconPath);
    }
    const auto& treeCollection = data.getTreeCollection();
    for (int i = 0; i < treeCollection.size(); i++)
    {
        const auto& rec = treeCollection.getRecord(i).get();
        add(rec.editorId, CkId::Type_Tree_, rec.modelPath, rec.iconPath);
    }
    return refs;
}

AssetDependencyScanner::ScanResult AssetDependencyScanner::scanAll(const Data& data, const QString& dataDir)
{
    ScanResult result;
    AssetResolver resolver(dataDir);

    // Driven by the same walk the external-data collector uses, so the two cannot
    // disagree about which assets a plugin depends on.
    for (const AssetReference& ref : collectReferences(data))
    {
        result.totalPathsScanned++;
        if (resolver.contains(ref.assetPath))
            continue;
        MissingAsset missing;
        missing.recordId = ref.recordId;
        missing.recordType = ref.recordType;
        missing.assetPath = ref.assetPath;
        missing.assetType = ref.assetType;
        missing.suggestions = findSimilarPathsFrom(resolver, ref.assetPath, 5);
        result.missingAssets.append(missing);
        result.totalMissing++;
    }
    return result;
}

// ============================================================================
// relinkAsset
// ============================================================================

template<typename ESXRecord>
static bool relinkAssetImpl(Collection<ESXRecord>& collection, const QString& recordId,
                            const QString& oldPath, const QString& newPath)
{
    for (int i = 0; i < collection.size(); i++)
    {
        Record<ESXRecord>& rec = collection.getRecord(i);
        if (rec.isErased() || rec.isDeleted())
            continue;

        ESXRecord& data = rec.get();
        if (data.editorId == recordId)
        {
            if (data.modelPath == oldPath)
                data.modelPath = newPath;
            if (data.iconPath == oldPath)
                data.iconPath = newPath;
            rec.setModified(data);
            return true;
        }
    }
    return false;
}

bool AssetDependencyScanner::relinkAsset(Data& data, CkId::Type type, const QString& recordId,
                                         const QString& oldPath, const QString& newPath)
{
    BaseCollection* baseCol = data.getCollectionByType(type);
    if (!baseCol)
        return false;

    switch (type)
    {
    case CkId::Type_Stat_:
        return relinkAssetImpl<StatRecord>(*static_cast<Collection<StatRecord>*>(baseCol), recordId, oldPath, newPath);
    case CkId::Type_Weap_:
        return relinkAssetImpl<WeaponRecord>(*static_cast<Collection<WeaponRecord>*>(baseCol), recordId, oldPath, newPath);
    case CkId::Type_Armor_:
        return relinkAssetImpl<ArmorRecord>(*static_cast<Collection<ArmorRecord>*>(baseCol), recordId, oldPath, newPath);
    case CkId::Type_Book_:
        return relinkAssetImpl<BookRecord>(*static_cast<Collection<BookRecord>*>(baseCol), recordId, oldPath, newPath);
    case CkId::Type_Misc_:
        return relinkAssetImpl<MiscRecord>(*static_cast<Collection<MiscRecord>*>(baseCol), recordId, oldPath, newPath);
    case CkId::Type_Ingr_:
        return relinkAssetImpl<IngrRecord>(*static_cast<Collection<IngrRecord>*>(baseCol), recordId, oldPath, newPath);
    case CkId::Type_Alch_:
        return relinkAssetImpl<AlchRecord>(*static_cast<Collection<AlchRecord>*>(baseCol), recordId, oldPath, newPath);
    case CkId::Type_Cont_:
        return relinkAssetImpl<ContRecord>(*static_cast<Collection<ContRecord>*>(baseCol), recordId, oldPath, newPath);
    case CkId::Type_Acti_:
        return relinkAssetImpl<ActiRecord>(*static_cast<Collection<ActiRecord>*>(baseCol), recordId, oldPath, newPath);
    case CkId::Type_Tree_:
        return relinkAssetImpl<TreeRecord>(*static_cast<Collection<TreeRecord>*>(baseCol), recordId, oldPath, newPath);
    default:
        return false;
    }
}

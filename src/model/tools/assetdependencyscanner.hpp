#pragma once

#include "../world/data.hpp"
#include "../world/ckid.hpp"
#include "../../../libs/files/filepaths.hpp"

#include <QString>
#include <QStringList>
#include <QVector>

class AssetResolver;

// One asset path as written by one record. The shared output of the record walk,
// so dependency scanning and external-data collection cannot disagree about what
// a plugin references.
struct AssetReference
{
    QString recordId;
    CkId::Type recordType = CkId::Type_Stat_;
    QString assetPath;
    QString assetType;   // "model", "texture", ...
};

class AssetDependencyScanner
{
public:
    struct MissingAsset
    {
        QString recordId;
        CkId::Type recordType;
        QString assetPath;
        QString assetType;
        QStringList suggestions;
    };

    struct ScanResult
    {
        QVector<MissingAsset> missingAssets;
        int totalPathsScanned = 0;
        int totalMissing = 0;
    };

    static ScanResult scanAll(const Data& data, const QString& dataDir);

    // Every asset path every record references, in collection order and with
    // duplicates left in: a path referenced by four records is four references,
    // and callers that want it de-duplicated have to say so. Empty paths are
    // omitted, since a record with no model set references nothing.
    static QVector<AssetReference> collectReferences(const Data& data);
    static QStringList findSimilarPaths(const QString& path, const QString& dataDir, int maxResults = 5);
    static bool relinkAsset(Data& data, CkId::Type type, const QString& recordId,
                            const QString& oldPath, const QString& newPath);
    static QString typeName(CkId::Type type);

private:
    static int levenshteinDistance(const QString& s1, const QString& s2);
    static QStringList findSimilarPathsFrom(const AssetResolver& resolver, const QString& path,
                                            int maxResults);
};

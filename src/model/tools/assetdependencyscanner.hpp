#pragma once

#include "../world/data.hpp"
#include "../world/ckid.hpp"
#include "../../../libs/files/filepaths.hpp"

#include <QString>
#include <QStringList>
#include <QVector>

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
    static QStringList findSimilarPaths(const QString& path, const QString& dataDir, int maxResults = 5);
    static bool relinkAsset(Data& data, CkId::Type type, const QString& recordId,
                            const QString& oldPath, const QString& newPath);
    static QString typeName(CkId::Type type);

private:
    static int levenshteinDistance(const QString& s1, const QString& s2);
    static bool pathExistsInDir(const QString& assetPath, const QString& dataDir);
    static QStringList buildFileIndex(const QString& dataDir);
    static void checkPathsForRecord(const QString& recordId, CkId::Type type,
                                    const QString& modelPath, const QString& iconPath,
                                    const QString& dataDir, const QStringList& fileIndex,
                                    ScanResult& result);
};

#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>

// Tells callers whether a game asset path exists, looking both at loose files
// in the data directory and inside every BSA/BA2 archive found there. Asset
// validators and dependency scanners previously reported archived assets as
// missing; this closes that gap. Paths are matched case-insensitively and
// normalized to backslash separators (plugin paths use backslashes).
class AssetResolver
{
public:
    AssetResolver();
    explicit AssetResolver(const QString& dataDir);

    // (Re)scan dataDir: loose files plus all *.bsa / *.ba2 archives.
    void scan(const QString& dataDir);

    // True if the relative asset path exists loose or inside an archive.
    bool contains(const QString& relativePath) const;
    // True if the relative path is a loose file (not inside an archive).
    bool containsLoose(const QString& relativePath) const;
    // Absolute path for a loose file; empty when the path is only in archives.
    QString absoluteLoosePath(const QString& relativePath, const QString& dataDir) const;
    // Absolute path of an archive that holds this path, or empty when the path is
    // loose or unknown. Lets a caller extract the entry instead of only knowing
    // that it exists somewhere.
    QString archiveContaining(const QString& relativePath) const;

    // All known relative asset paths (normalized, backslash separators).
    const QStringList& allPaths() const { return mPaths; }
    int archiveCount() const { return mArchiveCount; }
    bool isEmpty() const { return mPaths.isEmpty(); }

private:
    void addLooseFiles(const QString& dataDir);
    void addBsa(const QString& path);
    void addBa2(const QString& path);
    static QString canonical(const QString& path);

    QStringList mPaths;
    QSet<QString> mPathSet;
    QSet<QString> mLooseSet;
    // canonical path -> absolute archive path, for paths only an archive provides.
    QMap<QString, QString> mArchiveOfPath;
    int mArchiveCount = 0;
};

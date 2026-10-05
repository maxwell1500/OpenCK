#include "assetresolver.hpp"
#include "logger.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include "../../../libs/files/ba2/bsaarchive.hpp"
#include "../../../libs/files/ba2/ba2archive.hpp"

AssetResolver::AssetResolver() = default;

AssetResolver::AssetResolver(const QString& dataDir)
{
    scan(dataDir);
}

QString AssetResolver::canonical(const QString& path)
{
    return path.toLower().replace(QLatin1Char('/'), QLatin1Char('\\'));
}

void AssetResolver::scan(const QString& dataDir)
{
    mPaths.clear();
    mPathSet.clear();
    mLooseSet.clear();
    mArchiveOfPath.clear();
    mArchiveCount = 0;

    QDir dir(dataDir);
    if (!dir.exists()) return;

    addLooseFiles(dataDir);

    // Top level first, then every subdirectory: almost every shipped archive sits
    // directly in the Data root and only DLC archives are nested. A single
    // recursive walk filtered on QDir::Dirs would visit only directories and miss
    // every archive in the root, which is what happened the first time this was
    // written.
    const auto openArchivesIn = [this](const QString& dir) {
        const QFileInfoList archives =
            QDir(dir).entryInfoList({ QStringLiteral("*.bsa"), QStringLiteral("*.ba2") },
                                    QDir::Files, QDir::Name);
        for (const QFileInfo& info : archives) {
            const QString full = info.absoluteFilePath();
            if (info.fileName().endsWith(QStringLiteral(".ba2"), Qt::CaseInsensitive))
                addBa2(full);
            else
                addBsa(full);
        }
    };

    openArchivesIn(dataDir);

    // Recursive, matching ArchiveBrowserDialog::findArchives(): an archive under
    // Data\DLC used to be invisible to asset resolution while the browser listed
    // the same directory.
    QDirIterator it(dataDir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        openArchivesIn(it.filePath());
    }
}

void AssetResolver::addLooseFiles(const QString& dataDir)
{
    QDirIterator it(dataDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        const QString rel = QDir(dataDir).relativeFilePath(it.filePath());
        const QString canon = canonical(rel);
        mLooseSet.insert(canon);
        if (!mPathSet.contains(canon))
        {
            mPathSet.insert(canon);
            mPaths.append(canon);
        }
    }
}

void AssetResolver::addBsa(const QString& path)
{
    BsaArchive bsa;
    if (!bsa.open(path))
    {
        LOG_WARNING(QString("AssetResolver: failed to open BSA %1").arg(path));
        return;
    }
    const auto& entries = bsa.entries();
    for (const auto& e : entries)
    {
        const QString canon = canonical(e.fullPath);
        if (!mPathSet.contains(canon))
        {
            mPathSet.insert(canon);
            mPaths.append(canon);
        }
        // First archive to claim a path wins, which is load order: a DLC archive
        // read before the base archive will shadow it. Collection only needs *an*
        // archive that has the file, and Bethesda's own override order is a
        // separate problem from knowing where to read a file from.
        if (!mArchiveOfPath.contains(canon) && !containsLoose(canon))
            mArchiveOfPath.insert(canon, path);
    }
    ++mArchiveCount;
}

void AssetResolver::addBa2(const QString& path)
{
    Ba2Archive ba2;
    if (!ba2.open(path))
    {
        LOG_WARNING(QString("AssetResolver: failed to open BA2 %1").arg(path));
        return;
    }
    const auto& entries = ba2.entries();
    for (const auto& e : entries)
    {
        const QString canon = canonical(e.relativePath);
        if (!mPathSet.contains(canon))
        {
            mPathSet.insert(canon);
            mPaths.append(canon);
        }
        if (!mArchiveOfPath.contains(canon) && !containsLoose(canon))
            mArchiveOfPath.insert(canon, path);
    }
    ++mArchiveCount;
}

QString AssetResolver::archiveContaining(const QString& relativePath) const
{
    if (relativePath.isEmpty()) return QString();
    return mArchiveOfPath.value(canonical(relativePath), QString());
}

bool AssetResolver::contains(const QString& relativePath) const
{
    if (relativePath.isEmpty()) return false;
    return mPathSet.contains(canonical(relativePath));
}

bool AssetResolver::containsLoose(const QString& relativePath) const
{
    if (relativePath.isEmpty()) return false;
    return mLooseSet.contains(canonical(relativePath));
}

QString AssetResolver::absoluteLoosePath(const QString& relativePath, const QString& dataDir) const
{
    if (!containsLoose(relativePath)) return QString();
    return QDir(dataDir).absoluteFilePath(relativePath);
}

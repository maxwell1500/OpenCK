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
    mArchiveCount = 0;

    QDir dir(dataDir);
    if (!dir.exists()) return;

    addLooseFiles(dataDir);

    const auto archives = dir.entryList({ "*.bsa", "*.ba2" }, QDir::Files, QDir::Name);
    for (const auto& archive : archives)
    {
        const QString full = dir.absoluteFilePath(archive);
        if (archive.endsWith(QStringLiteral(".ba2"), Qt::CaseInsensitive))
            addBa2(full);
        else
            addBsa(full);
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
    }
    ++mArchiveCount;
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

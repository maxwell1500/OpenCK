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

QString AssetResolver::resolveOrExtract(const QString& relativePath, const QString& dataDir)
{
    if (relativePath.isEmpty())
        return QString();

    if (QFileInfo::exists(relativePath))
        return relativePath;

    QString norm = relativePath;
    norm.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (norm.startsWith(QLatin1Char('/')))
        norm.remove(0, 1);

    if (dataDir.isEmpty())
        return QString();

    // 1. Loose file in dataDir
    const QString loose1 = dataDir + QLatin1Char('/') + norm;
    if (QFileInfo::exists(loose1))
        return loose1;

    if (!norm.startsWith(QLatin1String("meshes/"), Qt::CaseInsensitive) &&
        !norm.startsWith(QLatin1String("textures/"), Qt::CaseInsensitive))
    {
        const QString looseMesh = dataDir + QStringLiteral("/meshes/") + norm;
        if (QFileInfo::exists(looseMesh))
            return looseMesh;

        const QString looseTex = dataDir + QStringLiteral("/textures/") + norm;
        if (QFileInfo::exists(looseTex))
            return looseTex;
    }

    // 2. Extracted cache directory
    const QString cachePath = dataDir + QStringLiteral("/.openck_cache/") + norm;
    if (QFileInfo::exists(cachePath))
        return cachePath;

    // 3. On-demand search and extraction from archives (*.ba2, *.bsa)
    struct ArchiveEntryRef {
        QString archivePath;
        quint32 index = 0;
        bool isBa2 = false;
    };
    static QHash<QString, ArchiveEntryRef> s_index;
    static QString s_cachedDir;
    static bool s_scanned = false;

    if (s_cachedDir != dataDir) {
        s_index.clear();
        s_cachedDir = dataDir;
        s_scanned = false;
    }

    const QString canon = canonical(norm);

    // Archives are indexed lazily, one open() per archive, because a single
    // open() parses that archive's whole name table. The shipped install has
    // 107 BA2s and roughly 750k entries between them; indexing every one of
    // them on the first lookup blocks the UI thread for the length of a cell
    // load. The archive *names* are cheap, so cache those instead and open
    // each on demand, stopping at the first archive that holds the path.
    auto indexArchive = [canon, index = &s_index](const QString& fullArch, bool ba2) -> bool {
        if (s_index.contains(canon))
            return true;                        // already found
        if (ba2) {
            Ba2Archive a;
            if (!a.open(fullArch))
                return false;
            const auto& entries = a.entries();
            for (int i = 0; i < entries.size(); ++i) {
                const QString c = canonical(entries[i].relativePath);
                if (!s_index.contains(c))
                    s_index.insert(c, {fullArch, static_cast<quint32>(i), true});
            }
            if (a.isTexture()) {
                const auto& tx = a.textureEntries();
                for (int i = 0; i < tx.size(); ++i) {
                    const QString c = canonical(tx[i].relativePath);
                    if (!s_index.contains(c))
                        s_index.insert(c, {fullArch, static_cast<quint32>(i), true});
                }
            }
        } else {
            BsaArchive a;
            if (!a.open(fullArch))
                return false;
            const auto& entries = a.entries();
            for (int i = 0; i < entries.size(); ++i) {
                const QString c = canonical(entries[i].fullPath);
                if (!s_index.contains(c))
                    s_index.insert(c, {fullArch, static_cast<quint32>(i), false});
            }
        }
        return s_index.contains(canon);
    };

    ArchiveEntryRef ref = s_index.value(canon);
    if (ref.archivePath.isEmpty() && !s_scanned)
    {
        // Mesh archives first: a NIF never lands in a Textures*.ba2, and those
        // are the largest ones, so trying them first wastes the most time.
        QStringList searchDirs{dataDir};
        QDir baseDir(dataDir);
        if (baseDir.exists(QStringLiteral("Data")))
            searchDirs.append(baseDir.filePath(QStringLiteral("Data")));
        if (baseDir.exists(QStringLiteral("DLC")))
            searchDirs.append(baseDir.filePath(QStringLiteral("DLC")));

        for (const QString& sDir : searchDirs)
        {
            QDir dir(sDir);
            const QStringList archives = dir.entryList({QStringLiteral("*.ba2"), QStringLiteral("*.bsa")}, QDir::Files);
            for (const QString& archFile : archives)
            {
                const QString fullArch = dir.filePath(archFile);
                const bool ba2 = archFile.endsWith(QStringLiteral(".ba2"), Qt::CaseInsensitive);
                if (!ba2 && !archFile.endsWith(QStringLiteral(".bsa"), Qt::CaseInsensitive))
                    continue;
                if (indexArchive(fullArch, ba2))
                    break;                      // found; stop opening archives
            }
            if (s_index.contains(canon))
                break;
        }
        s_scanned = true;
        ref = s_index.value(canon);
    }

    if (ref.archivePath.isEmpty())
    {
        ref = s_index.value(canonical(QStringLiteral("meshes/") + norm));
        if (ref.archivePath.isEmpty())
            ref = s_index.value(canonical(QStringLiteral("textures/") + norm));
    }

    if (!ref.archivePath.isEmpty())
    {
        QDir().mkpath(QFileInfo(cachePath).absolutePath());
        if (ref.isBa2)
        {
            Ba2Archive ba2;
            if (ba2.open(ref.archivePath))
            {
                if (ba2.extract(ref.index, cachePath) && QFileInfo::exists(cachePath))
                    return cachePath;
            }
        }
        else
        {
            BsaArchive bsa;
            if (bsa.open(ref.archivePath))
            {
                if (bsa.extract(ref.index, cachePath) && QFileInfo::exists(cachePath))
                    return cachePath;
            }
        }
    }

    return QString();
}

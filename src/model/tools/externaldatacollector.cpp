#include "externaldatacollector.hpp"

#include "assetresolver.hpp"
#include "ba2/resourcearchiveconfig.hpp"
#include "assetdependencyscanner.hpp"
#include "../../../libs/files/ba2/ba2archive.hpp"
#include "../../../libs/files/ba2/bsaarchive.hpp"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace {
// Plugin paths use backslashes, archives use them too, but a path can reach us from
// an INI or a user with forward slashes, so matching has to tolerate both.
QString normalise(const QString& path)
{
    QString s = path.trimmed();
    s.replace(QLatin1Char('/'), QLatin1Char('\\'));
    while (s.startsWith(QLatin1Char('\\')))
        s.remove(0, 1);
    return s;
}

// Rejects anything that would let an archive entry name write outside the
// destination. Same reasoning as the archive browser's own extraction check: the
// name comes from the file, not from us.
bool containedDestination(const QString& root, const QString& relative, QString* out)
{
    if (relative.isEmpty())
        return false;
    if (relative.contains(QLatin1String("..")))
        return false;
    if (relative.contains(QLatin1Char(':')))
        return false;
    const QString dest = QDir(root).absoluteFilePath(relative);
    const QString cleanRoot = QDir::cleanPath(QDir(root).absolutePath());
    const QString cleanDest = QDir::cleanPath(dest);
    if (!cleanDest.startsWith(cleanRoot, Qt::CaseInsensitive))
        return false;
    if (out)
        *out = dest;
    return true;
}
} // namespace

ExternalDataCollector::Plan ExternalDataCollector::buildPlan(
    const QVector<AssetReference>& referenced, const AssetResolver& resolver,
    const Options& options, const ResourceArchiveConfig* config)
{
    Plan plan;
    plan.totalReferenced = referenced.size();

    // One entry per distinct asset path, with every record that wants it recorded,
    // so the report can say which records a collection is for.
    QMap<QString, Item> byPath;
    QStringList order;
    for (const AssetReference& ref : referenced)
    {
        const QString path = normalise(ref.assetPath);
        if (path.isEmpty())
            continue;
        const QString key = path.toLower();
        if (!byPath.contains(key)) {
            Item item;
            item.assetPath = path;
            byPath.insert(key, item);
            order.append(key);
        }
        Item& item = byPath[key];
        const QString who = QStringLiteral("%1 %2")
                                .arg(AssetDependencyScanner::typeName(ref.recordType),
                                     ref.recordId);
        if (!item.referencedBy.contains(who))
            item.referencedBy.append(who);
    }

    for (const QString& key : order)
    {
        Item item = byPath.value(key);

        if (resolver.containsLoose(item.assetPath)) {
            item.availability = Availability::Loose;
            plan.alreadyLoose.append(item);
            continue;
        }

        const QString archive = resolver.archiveContaining(item.assetPath);
        if (archive.isEmpty()) {
            item.availability = Availability::Unavailable;
            plan.unavailable.append(item);
            continue;
        }

        // Present, but only inside an archive. If the caller is shipping those
        // archives themselves, nothing needs collecting. Kept apart from
        // alreadyLoose: this file is not loose, it is merely covered.
        if (options.ignoreFilesInsideArchives) {
            item.availability = Availability::InArchive;
            item.sourceArchive = archive;
            plan.coveredByArchives.append(item);
            continue;
        }

        item.availability = Availability::InArchive;
        item.sourceArchive = archive;

        // Restricted to the game's own archives, a file that only exists inside
        // another mod's archive is not ours to ship. Reported, not collected,
        // because silently dropping it would look like success.
        //
        // The restriction only bites when there is a config to judge by. With
        // none, the honest answer is that we cannot tell a game archive from a
        // mod's -- and refusing to collect anything would make the whole feature
        // silently do nothing on an install with no tool INI.
        if (options.resourceArchivesOnly && config != nullptr && !config->namesArchive(archive)) {
            plan.inNonResourceArchive.append(item);
            continue;
        }
        plan.toCollect.append(item);
    }
    return plan;
}

ExternalDataCollector::Outcome ExternalDataCollector::collect(
    const Plan& plan, const QString& destination, bool (*isCancelled)())
{
    Outcome outcome;
    if (destination.isEmpty())
        return outcome;

    // Archives are opened once and kept for the run, and each one's path index is
    // built from that same object: a plan of a few hundred assets drawn from the
    // same handful of archives would otherwise re-read every header and name table
    // per file, and a probe opened separately could disagree with the archive
    // actually written from.
    QMap<QString, std::shared_ptr<BsaArchive>> bsa;
    QMap<QString, std::shared_ptr<Ba2Archive>> ba2;
    QMap<QString, QHash<QString, int>> index;

    const auto indexBsa = [&bsa, &index](const QString& path) {
        if (index.contains(path))
            return index.value(path);
        QHash<QString, int> map;
        auto archive = std::make_shared<BsaArchive>();
        if (archive->open(path)) {
            bsa.insert(path, archive);
            const auto& entries = archive->entries();
            for (int i = 0; i < entries.size(); ++i)
                map.insert(entries.at(i).fullPath.toLower(), i);
        }
        index.insert(path, map);
        return map;
    };
    const auto indexBa2 = [&ba2, &index](const QString& path) {
        if (index.contains(path))
            return index.value(path);
        QHash<QString, int> map;
        auto archive = std::make_shared<Ba2Archive>();
        if (archive->open(path)) {
            ba2.insert(path, archive);
            const auto& entries = archive->entries();
            for (int i = 0; i < entries.size(); ++i)
                map.insert(entries.at(i).relativePath.toLower(), i);
        }
        index.insert(path, map);
        return map;
    };

    for (const Item& item : plan.toCollect)
    {
        if (isCancelled && isCancelled())
            break;

        QString outPath;
        if (!containedDestination(destination, item.assetPath, &outPath)) {
            outcome.failures.append(QStringLiteral("unsafe path: %1").arg(item.assetPath));
            continue;
        }
        if (!QDir().mkpath(QFileInfo(outPath).absolutePath())) {
            outcome.failures.append(QStringLiteral("could not create folder for %1")
                                        .arg(item.assetPath));
            continue;
        }

        const QString key = item.assetPath.toLower();
        const bool isBsa = item.sourceArchive.endsWith(QStringLiteral(".bsa"),
                                                        Qt::CaseInsensitive);
        const int entry = (isBsa ? indexBsa(item.sourceArchive)
                                 : indexBa2(item.sourceArchive)).value(key, -1);
        if (entry < 0) {
            outcome.failures.append(QStringLiteral("%1 not found in %2")
                                        .arg(item.assetPath, QFileInfo(item.sourceArchive).fileName()));
            continue;
        }

        bool ok = false;
        if (isBsa) {
            auto it = bsa.constFind(item.sourceArchive);
            ok = it != bsa.constEnd() && (*it)->extract(static_cast<quint32>(entry), outPath);
        } else {
            auto it = ba2.constFind(item.sourceArchive);
            ok = it != ba2.constEnd() && (*it)->extract(static_cast<quint32>(entry), outPath);
        }
        if (ok) {
            ++outcome.written;
            outcome.writtenPaths.append(item.assetPath);
        } else {
            outcome.failures.append(QStringLiteral("failed to write %1").arg(item.assetPath));
        }
    }
    return outcome;
}
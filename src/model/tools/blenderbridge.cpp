#include "blenderbridge.hpp"

#include "../../../libs/files/nif/nifparser.hpp"
#include "../../../libs/files/nif/nifblockfile.hpp"
#include "../../files/log/logger.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QSet>

QString BlenderBridge::sLastCommitLog;

namespace {

// NifTools' importer keeps a skeleton root as the armature of a skinned
// mesh, but the skeleton NIF itself is an external file whose name the mesh
// records (NiSkinData / the root's name). When the parser cannot resolve it
// we fall back to the sibling conventions Blender/FO4 modders actually use.
QStringList candidateSkeletonNames(const QString& base)
{
    return {
        base + QStringLiteral("_skeleton.nif"),
        base + QStringLiteral("Skeleton.nif"),
        base + QStringLiteral("_skeleton.NIF"),
    };
}

QStringList collisionCandidates(const QFileInfo& mesh)
{
    // Same directory, same base, a collision marker, and a NIF extension.
    // Deliberately narrow: a wildcard sweep would pick up LODs and previews.
    const QStringList patterns = {
        mesh.completeBaseName() + QStringLiteral("*collision*.nif"),
        mesh.completeBaseName() + QStringLiteral("*collision*.NIF"),
        mesh.completeBaseName() + QStringLiteral("_cdx.nif"),
        mesh.completeBaseName() + QStringLiteral("_cdx.NIF"),
    };
    QDir dir(mesh.absolutePath());
    QStringList out;
    for (const QString& pattern : patterns)
    {
        const QStringList hits =
            dir.entryList(QStringList{ pattern }, QDir::Files);
        for (const QString& hit : hits)
        {
            const QString path = dir.absoluteFilePath(hit);
            if (path != mesh.absoluteFilePath() && !out.contains(path))
                out.append(path);
        }
    }
    return out;
}

bool isNif(const QString& path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    return ext == QStringLiteral("nif");
}

} // namespace

BlenderBridge::BlenderBridge(QObject* parent) : QObject(parent)
{
}

BlenderBridge::AssetContext BlenderBridge::discover(const QString& nifPath)
{
    AssetContext context;
    context.nifPath = nifPath;

    const QFileInfo mesh(nifPath);
    if (!mesh.exists())
    {
        LOG_ERROR(QString("BlenderBridge: mesh does not exist: %1").arg(nifPath));
        return context;
    }

    // The parser carries the real relationship (skin -> skeleton root). Use
    // it when the file parses; the sibling conventions are the fallback.
    Nif::NifParser parser;
    if (parser.load(nifPath))
    {
        // attachSkeleton is what the editor uses to merge an external
        // skeleton into a parsed scene; if the parser can find the file
        // itself, it is already the right one.
        for (const QString& candidate : candidateSkeletonNames(mesh.completeBaseName()))
        {
            const QString path = mesh.absolutePath() + "/" + candidate;
            if (QFileInfo::exists(path) && isNif(path))
            {
                context.skeletonPath = path;
                break;
            }
        }
    }
    else
    {
        LOG_WARNING(QString("BlenderBridge: %1 did not parse as a NIF; opening the "
                            "mesh without a resolved skeleton")
                        .arg(nifPath));
    }

    for (const QString& path : collisionCandidates(mesh))
    {
        if (isNif(path))
            context.collisionPaths.append(path);
    }
    return context;
}

QString BlenderBridge::defaultScriptPath()
{
    // scripts/blender/openck_livesync.py, resolved for both an installed
    // tree and a build tree. The application directory only answers when a
    // QApplication instance exists (it does not, in tests or CLI runs), so
    // fall back to searching upward from the working directory.
    QStringList roots;
    if (QCoreApplication::instance())
        roots << QCoreApplication::applicationDirPath();
    if (const QString env = qEnvironmentVariable("OPENCK_SCRIPTS_DIR"); !env.isEmpty())
        roots << env;

    QDir walk(QDir::current());
    for (int depth = 0; depth < 6 && walk.exists(); ++depth)
    {
        roots << walk.absolutePath();
        if (!walk.cdUp())
            break;
    }

    for (const QString& root : roots)
    {
        const QString candidate =
            QDir::cleanPath(root + QStringLiteral("/scripts/blender/openck_livesync.py"));
        if (QFileInfo::exists(candidate))
            return candidate;
    }
    return QString();
}

BlenderBridge::LaunchSpec BlenderBridge::buildLaunch(const QString& nifPath,
                                                     const QString& blenderPath,
                                                     const QString& scriptPath)
{
    LaunchSpec spec;
    spec.context = discover(nifPath);
    spec.blenderPath = blenderPath;
    spec.outputPath = QFileInfo(nifPath).absolutePath() + "/"
        + QFileInfo(nifPath).completeBaseName() + QStringLiteral("_opencklive.nif");

    const QString script = scriptPath.isEmpty() ? defaultScriptPath() : scriptPath;
    spec.scriptPath = script;
    if (script.isEmpty() || !QFileInfo::exists(script))
    {
        spec.error = QStringLiteral("openck_livesync.py not found");
        return spec;
    }

    QStringList args;
    args << QStringLiteral("--python") << script << QStringLiteral("--");
    args << QStringLiteral("--op=open");
    args << QStringLiteral("--nif=") + spec.context.nifPath;
    if (!spec.context.skeletonPath.isEmpty())
        args << QStringLiteral("--skeleton=") + spec.context.skeletonPath;
    for (const QString& collision : spec.context.collisionPaths)
        args << QStringLiteral("--collision=") + collision;
    args << QStringLiteral("--output=") + spec.outputPath;
    spec.blenderArgs = args;
    return spec;
}

bool BlenderBridge::armSession(const AssetContext& context, const QString& outputPath)
{
    stop();
    if (outputPath.isEmpty())
        return false;

    mContext = context;
    mOutputPath = outputPath;
    mLastExportStamp = QDateTime();
    mVerified = false;

    // Watch the directory, not the file: the export usually does not exist
    // yet when the session arms, and a watcher bound to a missing path
    // never fires. directoryChanged also catches the atomic-ish
    // truncate-then-write sequence editors use.
    mWatcher.addPath(QFileInfo(outputPath).absolutePath());
    connect(&mWatcher, &QFileSystemWatcher::directoryChanged, this,
            &BlenderBridge::onDirectoryChanged);
    return true;
}

bool BlenderBridge::start(const QString& nifPath, const QString& blenderPath,
                          QString* error)
{
    stop();

    const LaunchSpec spec = buildLaunch(nifPath, blenderPath);
    if (!spec.error.isEmpty())
    {
        if (error)
            *error = spec.error;
        return false;
    }

    const QFileInfo mesh(nifPath);
    if (!mesh.exists())
    {
        if (error)
            *error = QStringLiteral("mesh not found: %1").arg(nifPath);
        return false;
    }

    mBlenderPath = spec.blenderPath;
    mScriptPath = spec.scriptPath;
    armSession(spec.context, spec.outputPath);

    const bool launched = QProcess::startDetached(spec.blenderPath, spec.blenderArgs);
    if (!launched)
    {
        if (error)
            *error = QStringLiteral("could not start %1").arg(spec.blenderPath);
        stop();
        return false;
    }
    LOG_INFO(QString("BlenderBridge: live-sync session for %1 -> %2")
                 .arg(nifPath, spec.outputPath));
    return true;
}

void BlenderBridge::stop()
{
    if (!mWatcher.files().isEmpty())
        mWatcher.removePaths(mWatcher.files() + mWatcher.directories());
    disconnect(&mWatcher, &QFileSystemWatcher::directoryChanged, this,
               &BlenderBridge::onDirectoryChanged);
    mOutputPath.clear();
    mVerified = false;
}

QString BlenderBridge::verifyExport(const QString& path, QString* reason) const
{
    // The parser is the editor's real reader: it routes Gamebryo binaries,
    // NetImmerse files and the internal dialect itself, so "the export
    // parses" means "the editor can open what Blender wrote".
    Nif::NifParser parser;
    if (!parser.load(path))
    {
        if (reason)
            *reason = QStringLiteral(
                "the editor's NIF reader rejected the export");
        return QString();
    }

    // Gamebryo files additionally have to split into blocks the way real
    // game assets do; the container is what posts a truncated export
    // before it is half the size NifTools claims to have written.
    QFile probe(path);
    if (probe.open(QIODevice::ReadOnly))
    {
        const QByteArray head = probe.read(40);
        probe.close();
        if (head.startsWith("Gamebryo File Format"))
        {
            NifBlockFile blocks;
            if (!blocks.load(path))
            {
                if (reason)
                    *reason = QStringLiteral("NIF container is truncated or "
                                             "misaligned: %1")
                                  .arg(blocks.lastError());
                return QString();
            }
        }
    }
    else
    {
        if (reason)
            *reason = QStringLiteral("export could not be opened");
        return QString();
    }

    if (reason)
        reason->clear();
    return path;
}

void BlenderBridge::onDirectoryChanged(const QString& dir)
{
    if (mOutputPath.isEmpty())
        return;
    if (QFileInfo(mOutputPath).absolutePath() != QFileInfo(dir).absoluteFilePath())
        return;

    const QFileInfo info(mOutputPath);
    if (!info.exists())
        return;   // export removed/being rewritten; wait for the next event

    // Some editors write the export twice (truncate then write); the
    // second event carries the real bytes, and QDateTime's granularity is
    // coarse, so a size change also counts as a new export.
    const QDateTime stamp = info.lastModified();
    const qint64 size = info.size();
    const bool unchanged = (stamp == mLastExportStamp)
        && (size == mLastExportSize);
    mLastExportStamp = stamp;
    mLastExportSize = size;
    emit exportChanged(mOutputPath);
    if (unchanged)
        return;

    QString reason;
    if (!verifyExport(mOutputPath, &reason).isEmpty())
    {
        mVerified = true;
        emit exportVerified(mOutputPath, size);
    }
    else
    {
        mVerified = false;
        LOG_ERROR(QString("BlenderBridge: rejected export %1 (%2)")
                      .arg(mOutputPath, reason));
        emit exportRejected(mOutputPath, reason);
    }
}

bool BlenderBridge::commit(QString* error)
{
    if (!mVerified || mOutputPath.isEmpty())
    {
        if (error)
            *error = QStringLiteral("no verified export to commit");
        return false;
    }

    const QString asset = mContext.nifPath;
    const QFileInfo exportInfo(mOutputPath);
    if (!exportInfo.exists())
    {
        if (error)
            *error = QStringLiteral("export file vanished: %1").arg(mOutputPath);
        return false;
    }

    // Preserve the original as <name>.openckbak next to it, then swap the
    // export into place atomically. A failed commit must never leave a
    // half-written game asset behind.
    const QString backup = QFileInfo(asset).absolutePath() + "/"
        + QFileInfo(asset).completeBaseName() + QStringLiteral(".openckbak");
    if (QFile::exists(asset) && !QFile::exists(backup))
        QFile::copy(asset, backup);

    QSaveFile target(asset);
    if (!target.open(QIODevice::WriteOnly))
    {
        if (error)
            *error = QStringLiteral("cannot write %1: %2")
                         .arg(asset, target.errorString());
        return false;
    }
    QFile source(mOutputPath);
    if (!source.open(QIODevice::ReadOnly))
    {
        if (error)
            *error = QStringLiteral("cannot read %1: %2")
                         .arg(mOutputPath, source.errorString());
        target.cancelWriting();
        return false;
    }
    const QByteArray bytes = source.readAll();
    source.close();
    if (target.write(bytes) != bytes.size())
    {
        if (error)
            *error = QStringLiteral("short write to %1").arg(asset);
        target.cancelWriting();
        return false;
    }
    if (!target.commit())
    {
        if (error)
            *error = QStringLiteral("commit failed for %1: %2")
                         .arg(asset, target.errorString());
        return false;
    }

    // Re-read the committed bytes as the final word: what the user's plugin
    // now contains must parse, or the commit is rolled back to the backup.
    QString reason;
    if (verifyExport(asset, &reason).isEmpty())
    {
        if (QFile::exists(backup))
            QFile::remove(asset), QFile::copy(backup, asset);
        if (error)
            *error = QStringLiteral("committed asset failed verification: %1")
                         .arg(reason);
        return false;
    }

    sLastCommitLog = QStringLiteral("%1 <- %2 at %3")
                         .arg(asset, mOutputPath,
                              QDateTime::currentDateTime().toString(Qt::ISODate));
    mVerified = false;
    emit committed(asset, mOutputPath);
    LOG_INFO(QString("BlenderBridge: committed %1").arg(sLastCommitLog));
    return true;
}

QString BlenderBridge::lastCommitLog()
{
    return sLastCommitLog;
}

#ifndef BLENDERBRIDGE_HPP
#define BLENDERBRIDGE_HPP

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QObject>
#include <QStringList>

// Phase 12.2: one-click viewport live-sync with Blender.
//
// Opening a NIF in Blender is not enough for a mod workflow: the mesh must
// arrive with its skeleton (armature) and collision meshes attached, and
// what the artist saves in Blender must come back into the plugin without a
// manual re-export. This bridge owns that round trip:
//
//   1. discover(): resolve the mesh NIF, its skeleton (from the NIF's own
//      skin data, falling back to sibling naming) and collision siblings,
//      so the Blender session opens scene-complete.
//   2. start(): launch Blender with scripts/blender/openck_livesync.py and
//      watch the designated export file. The script exports back to that
//      path from a save_post handler, so pressing Ctrl-S in Blender is the
//      whole "export" step.
//   3. every change is verified through the same parsers the editor uses
//      (NifParser / NifBlockFile); a rejected export never overwrites the
//      plugin's asset.
//   4. commit(): atomically copy the verified export over the original
//      mesh path (QSaveFile), which is the only path that mutates game data.
//
// The bridge deliberately never edits the original NIF directly: the export
// is staged next to it, and only a verified export can be committed.
class BlenderBridge : public QObject
{
    Q_OBJECT

public:
    // Everything the Blender session needs for a scene-complete open.
    struct AssetContext
    {
        QString nifPath;                 // the mesh NIF to edit
        QString skeletonPath;            // armature NIF, empty when none
        QStringList collisionPaths;      // collision NIFs beside the mesh

        bool isValid() const { return !nifPath.isEmpty(); }
    };

    // The fully-resolved launch plan (also the unit under test).
    struct LaunchSpec
    {
        QString blenderPath;
        QString scriptPath;              // openck_livesync.py
        QStringList blenderArgs;         // argv after the executable
        QString outputPath;              // watched export file
        AssetContext context;
        QString error;
    };

    struct Session
    {
        AssetContext context;
        QString outputPath;
        QDateTime lastExport;
        bool verified = false;
        bool committed = false;
    };

    explicit BlenderBridge(QObject* parent = nullptr);

    // Resolves skeleton and collision siblings for a mesh NIF. Reads the
    // NIF through NifParser (skin blocks reference the skeleton); a mesh
    // whose NIF cannot be parsed at all still returns a context with the
    // mesh path, so the caller can report a concrete reason.
    static AssetContext discover(const QString& nifPath);

    static QString defaultScriptPath();

    // Builds the argv and output path. Does not launch anything, so the
    // plan can be asserted on machines without Blender installed.
    static LaunchSpec buildLaunch(const QString& nifPath, const QString& blenderPath,
                                  const QString& scriptPath = QString());

    // Launches Blender for nifPath and arms the export watcher. Returns
    // false when the launch could not be built; the reason is in error.
    bool start(const QString& nifPath, const QString& blenderPath, QString* error = nullptr);

    // Arms the export watcher for a context/output pair without launching
    // Blender: the session is live (changes verify, commits work) but no
    // editor is started. The app uses this when a Blender instance is
    // already running the bridge script; the tests use it to drive the
    // verification and commit rules without a Blender install.
    bool armSession(const AssetContext& context, const QString& outputPath);

    // Atomically copies the verified export over the original mesh.
    // Returns false (with error set) when there is no verified export.
    bool commit(QString* error = nullptr);

    void stop();

    bool hasSession() const { return !mOutputPath.isEmpty(); }
    QString outputPath() const { return mOutputPath; }
    const AssetContext& context() const { return mContext; }

    static QString lastCommitLog();

signals:
    // Emitted after an export file appeared or changed.
    void exportChanged(const QString& path);
    // Emitted when an export passed parser verification.
    void exportVerified(const QString& path, qint64 size);
    // Emitted when an export failed verification (path, reason). The
    // original asset is untouched.
    void exportRejected(const QString& path, const QString& reason);
    // Emitted after a verified export was committed over the original.
    void committed(const QString& assetPath, const QString& fromExport);

private slots:
    void onDirectoryChanged(const QString& dir);

private:
    QString verifyExport(const QString& path, QString* reason) const;

    AssetContext mContext;
    QString mBlenderPath;
    QString mScriptPath;
    QString mOutputPath;
    QDateTime mLastExportStamp;
    qint64 mLastExportSize = -1;
    bool mVerified = false;
    QFileSystemWatcher mWatcher;   // watches the export's directory

    static QString sLastCommitLog;
};

#endif // BLENDERBRIDGE_HPP

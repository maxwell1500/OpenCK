#ifndef MODDEPLOYMENTRESOLVER_HPP
#define MODDEPLOYMENTRESOLVER_HPP

#include <QString>
#include <QVector>

// Parses and resolves mod-manager *deployment* state: the file-level maps
// that record which mod provided each deployed file, in what priority, and
// how the copies were made. Asset resolution walks this map instead of
// asking users to flatten directories.
//
// Vortex schema is grounded on the real vortex.deployment.json a managed
// game writes into its data directory (Starfield install):
//   { "instance": "...", "version": 1,
//     "deploymentMethod": "hardlink_activator",
//     "gameId": "starfield", "deploymentTime": 1782351802311,
//     "stagingPath": "...\\Vortex\\starfield\\mods",
//     "targetPath": "...\\Starfield\\Content",
//     "files": [ { "relPath": "JetpackVanilla.txt",
//                  "source": "Jetpack Overhaul-569-1-05-1694446917",
//                  "target": "", "time": 1765686656000 } ] }
//
// MO2 has no manifest; priority lives in profiles/<profile>/modlist.txt,
// one mod per line, "+Name" enabled / "-Name" disabled, last line wins.
class ModDeploymentResolver
{
public:
    struct DeployedFile
    {
        QString relPath; // relative to targetPath, forward slashes
        QString source;  // mod folder name under stagingPath
        QString target;  // subdirectory under targetPath; usually empty
        qint64 time = 0; // ms epoch
    };

    struct VortexDeployment
    {
        QString instance;
        int version = 0;
        QString deploymentMethod; // e.g. "hardlink_activator"
        QString gameId;
        qint64 deploymentTime = 0;
        QString stagingPath;
        QString targetPath;
        QVector<DeployedFile> files;

        bool isValid() const
        {
            return !targetPath.isEmpty() && !files.isEmpty();
        }
    };

    // One line of an MO2 modlist.txt.
    struct ModEntry
    {
        QString name;
        bool enabled = false;
    };

    enum class LinkKind
    {
        SameFile,  // two names for one file (what hardlinks produce)
        Hardlink,  // NTFS hardlink (same volume, distinct name table entry)
        Symlink,   // reparse point
        Distinct,  // two separate files
        Unknown    // unreadable, missing, or not Windows
    };

    // --- Vortex ------------------------------------------------------------

    /// Parses a vortex.deployment.json payload. Pure; false on malformed
    /// input (wrong shape, bad JSON, or a files array that isn't one).
    static bool parseVortexDeployment(const QByteArray& json,
                                      VortexDeployment& out);
    /// Reads and parses a deployment manifest from disk.
    static bool readVortexDeployment(const QString& path,
                                     VortexDeployment& out);
    /// Looks for a Vortex deployment manifest in a game data directory:
    /// vortex.deployment.json (game root) or the per-folder
    /// vortex.deployment.<name>.json that a Data-folder deployment writes.
    /// Returns empty when none exists.
    static QString findVortexDeployment(const QString& gameDataDir);

    /// Absolute path of the deployed file, empty when the manifest has no
    /// entry for relPath.
    static QString deployedFilePath(const VortexDeployment& deployment,
                                    const QString& relPath);
    /// Absolute path of the mod's own copy under the staging path, empty
    /// when unknown.
    static QString sourceFilePath(const VortexDeployment& deployment,
                                  const QString& relPath);

    /// Compares two paths at the file-system level (NTFS file id plus
    /// reparse-point check).
    static LinkKind linkStatus(const QString& a, const QString& b);
    static QString linkKindName(LinkKind kind);

    /// "meshes\\x.nif" -> "meshes/x.nif" (deployment keys always use '/').
    static QString normalizeRelPath(const QString& relPath);

    // --- Mod Organizer 2 --------------------------------------------------

    /// Parses modlist.txt content: "+Name" enabled, "-Name" disabled, all
    /// other lines skipped.
    static bool parseMo2ModList(const QString& content,
                                QVector<ModEntry>& out);
    static bool readMo2ModList(const QString& path, QVector<ModEntry>& out);

    /// Enabled mods in file order; MO2 applies them so the last enabled
    /// line has the highest priority.
    static QStringList enabledMods(const QVector<ModEntry>& entries);

    /// First (highest-priority) enabled mod that provides relPath, with the
    /// resolved file path in fullPath when given.
    static QString winningMod(const QVector<ModEntry>& entries,
                              const QString& modsDir,
                              const QString& relPath,
                              QString* fullPath = nullptr);

    /// Every enabled mod that provides relPath, winner first.
    static QVector<QPair<QString, QString>> overrideChain(
        const QVector<ModEntry>& entries, const QString& modsDir,
        const QString& relPath);
};

#endif // MODDEPLOYMENTRESOLVER_HPP

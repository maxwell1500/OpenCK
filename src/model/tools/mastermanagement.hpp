#ifndef MASTERMANAGEMENT_H
#define MASTERMANAGEMENT_H

#include <QString>
#include <QVector>
#include <QMap>

// MasterManagement implements the plugin master-file management (MMS)
// features the real Creation Kit exposes through its [MMS] ini section and
// Tools > MMS dialog: choosing the master-update source for a plugin, and
// controlling how free form IDs are allocated (where the plugin starts
// assigning new local IDs, and whether it reuses holes left by deleted
// records).
class MasterManagement
{
public:
    // How the plugin picks up form-ID edits from its masters.
    enum class UpdateSource
    {
        MastersOnly,       // only load masters
        MastersAndMods,    // load masters plus other active plugins
        LocalOnly          // do not update from any file
    };

    static QString updateSourceToString(UpdateSource source);
    static UpdateSource stringToUpdateSource(const QString& text);

    // Allocation policy for new records.
    struct AllocationPolicy
    {
        bool reuseDeleted = false;       // fill gaps left by deleted records
        bool sequential = true;          // assign next free ID in order
        quint32 startLocalId = 1;        // first local ID to try (1..0xFFFFFF)
        int gapThreshold = 16;           // max gap to scan before advancing

        quint32 nextLocalId = 1;         // running counter
    };

    MasterManagement() = default;

    // Loads settings from a CreationKit-style ini file's [MMS] section.
    // Keys understood: MMS_UpdateMasterFromFile, MMS_UseLocalFileForUpdate,
    // MMS_ReuseDeletedRecordIDs, MMS_StartLocalID (case-insensitive).
    // Returns false if the file could not be read.
    bool loadIni(const QString& iniPath);

    // Applies a single [MMS]-style ini line (KEY=VALUE). Returns true if the
    // key was recognized.
    bool applyIniLine(const QString& key, const QString& value);

    // Reserves the next free local form ID, honoring the allocation policy.
    // 'usedLocalIds' holds IDs already allocated by the plugin; the result
    // is a local ID (master byte = 0) that is not in the used set.
    quint32 allocateLocalId(const QVector<quint32>& usedLocalIds);

    UpdateSource updateSource = UpdateSource::MastersAndMods;
    bool useLocalFileForUpdate = false;  // update from local files only
    QString updateFile;                  // the local file used for updates
    AllocationPolicy allocation;

    // Parsed [MMS] keys kept for reference/debugging.
    QMap<QString, QString> rawKeys;

    // Returns the keys this implementation recognizes.
    static QStringList knownKeys();
};

#endif // MASTERMANAGEMENT_H

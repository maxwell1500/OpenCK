#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

// What a game tool's own INI says about which archives belong to the game rather
// than to a mod. Those archives are "resource" archives: their contents ship with
// the game, so anything a plugin references that lives only in one of them has to
// be collected loose for the mod to work on a clean install.
//
// Without this distinction every archive looks alike, and external-data
// collection cannot tell "this file is already covered by the game" from "this
// file needs shipping with the mod".
class ResourceArchiveConfig
{
public:
    // Reads the archive lists and the default external codec id out of an INI
    // written by a game tool. Never fails: an absent or unreadable file yields an
    // empty configuration, which callers must treat as "nothing is known" rather
    // than "nothing is a resource archive".
    static ResourceArchiveConfig fromIni(const QString& iniPath);

    // Archives named in the resource list, as written. Case preserved.
    const QStringList& resourceArchives() const { return mResourceArchives; }
    // The separate texture/index archive list. Kept apart because it names a
    // different set: on a Starfield install the two lists do not overlap, and
    // merging them would claim 15 archives are resource archives that are not.
    const QStringList& resourceIndexArchives() const { return mResourceIndexArchives; }

    // Every archive named by either list, de-duplicated, which is what
    // "is this a game archive" needs to test.
    QStringList allNamedArchives() const;

    // Default external codec id, or -1 when the INI does not state one.
    int defaultExternalCodecId() const { return mDefaultExternalCodecId; }

    bool isEmpty() const { return mResourceArchives.isEmpty() && mResourceIndexArchives.isEmpty(); }

    // Resolves the configured names against the archives actually present under a
    // data directory. Returned in configured order, so a caller can report the
    // difference between "configured" and "installed" rather than only the
    // intersection.
    struct Resolution
    {
        // Name -> absolute path, for every configured archive found on disk.
        QMap<QString, QString> found;
        // Configured names with no matching file, which is the normal case for a
        // DLC archive that is not installed.
        QStringList missing;
    };
    static Resolution resolve(const QStringList& names, const QString& dataDir);

    // True when `archiveFileName` appears in either configured list. Matching is
    // on the file name alone, because that is all the INI carries.
    bool namesArchive(const QString& archiveFileName) const;

private:
    QStringList mResourceArchives;
    QStringList mResourceIndexArchives;
    int mDefaultExternalCodecId = -1;
};
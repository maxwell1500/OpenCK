#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

class AssetResolver;

// Gathers the assets a plugin depends on into a folder that can be shipped with a
// mod. The problem it solves: a plugin can reference a mesh, texture or sound
// that resolves fine on the author's machine because the game ships it inside a
// resource archive, and that file is simply absent from the mod. Asset validation
// reports no problem, because the file genuinely exists -- just not anywhere the
// mod can reach.
//
// The distinction this makes is loose versus archived, not present versus absent.
// An asset that is already loose is left alone; one that only exists inside an
// archive is extracted; one that exists nowhere is reported rather than invented.
class ExternalDataCollector
{
public:
    struct Options
    {
        // When true, an asset that exists inside any archive counts as covered and
        // is skipped. This is the "ignore files already present in archives"
        // behaviour: useful when the archives are being shipped alongside the mod,
        // and wrong when they are the game's own. Off by default, because for a
        // mod the game's archives are precisely what is not being shipped.
        bool ignoreFilesInsideArchives = false;

        // Restrict collection to archives the game tool's INI names as resource
        // archives. When false, any archive that provides the file will do --
        // which will happily extract a file out of another mod's archive.
        bool resourceArchivesOnly = true;
    };

    enum class Availability
    {
        Loose,        // already a file on disk next to the game data
        InArchive,    // only inside an archive, so it must be collected
        Unavailable,  // referenced but present nowhere we can see
    };

    struct Item
    {
        QString assetPath;       // as the plugin writes it
        Availability availability = Availability::Unavailable;
        QString sourceArchive;   // absolute, when collected from one
        QStringList referencedBy;// "Type 0001234" strings, for the report
    };

    struct Plan
    {
        QVector<Item> toCollect;
        QVector<Item> alreadyLoose;
        // Skipped because the caller is shipping the archives themselves. Not
        // the same as alreadyLoose: nothing is on disk for these.
        QVector<Item> coveredByArchives;
        QVector<Item> unavailable;
        // Collected for a plugin, but the only archive providing it is not one the
        // game ships -- it belongs to another mod. Reported, not collected.
        QVector<Item> inNonResourceArchive;
        // Files named by the resource-archive list with no matching file on disk,
        // which is ordinary for uninstalled DLC and worth surfacing rather than
        // silently treating as "not a resource archive".
        QStringList configuredArchivesMissing;
        int totalReferenced = 0;
    };

    // Builds the plan without touching the destination. Pure apart from reading
    // the resolver, so it can be shown to the user before anything is written.
    // `referenced` is what AssetDependencyScanner::collectReferences() returns.
    //
    // `config` is only consulted when Options::resourceArchivesOnly is set. Pass
    // null and every item found inside any archive is treated as collectable,
    // which is the right default for a data directory with no tool INI to consult.
    static Plan buildPlan(const QVector<class AssetReference>& referenced,
                          const AssetResolver& resolver, const Options& options,
                          const class ResourceArchiveConfig* config = nullptr);

    // Writes the plan's collectable items under destination, preserving each
    // asset's relative path. Returns the number written and appends the failures
    // with their reason, so a partial run is reportable rather than silent.
    struct Outcome
    {
        int written = 0;
        QStringList failures;
        QStringList writtenPaths;
    };
    static Outcome collect(const Plan& plan, const QString& destination,
                           bool (*isCancelled)() = nullptr);

    // Every asset path referenced by a loaded plugin comes from
    // AssetDependencyScanner::collectReferences(), which owns the record walk.
    // The collector deliberately does not have its own: two walks over the same
    // record fields would drift, and then validation and collection would
    // disagree about what a plugin depends on.
};
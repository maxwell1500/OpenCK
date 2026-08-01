#ifndef PLUGINCOMPACTOR_H
#define PLUGINCOMPACTOR_H

#include <QMap>
#include <QVector>
#include <cstdint>

class IRecordCollection;
struct RefrRecord;
struct CellRecord;

// PluginCompactor implements real plugin compaction: it builds a form-ID
// renumbering map (assigning dense sequential local IDs to every record
// defined in the active plugin, preserving each master's index byte) and
// then re-points references that target the renumbered records. This is
// the "form-ID renumbering + reference re-pointing" the real Creation Kit
// performs on small masters, as opposed to merely counting deleted records.
class PluginCompactor
{
public:
    // The renumbering result: old form IDs (defined in the plugin) mapped to
    // their new dense local IDs.
    struct RenumberMap
    {
        QMap<quint32, quint32> oldToNew;
        int renumbered = 0;
    };

    // Result of running a compaction pass over the plugin.
    struct Result
    {
        int totalRecords = 0;
        int renumbered = 0;
        int repointedReferences = 0;
        int skippedMasterOwned = 0;
    };

    // Collects every non-zero form ID from the given collections, deduped.
    static QVector<quint32> collectFormIds(const QVector<const IRecordCollection*>& collections);

    // Builds a renumbering map from the given (old) form IDs. Local IDs are
    // re-assigned densely starting at 1 while the master-index byte of each
    // ID is preserved (so references from and to other masters still work).
    static RenumberMap buildMap(const QVector<quint32>& formIds);

    // Returns the new form ID for an old one, or the unchanged value when the
    // ID is not part of the map (e.g. it is owned by a master).
    static quint32 remap(const RenumberMap& map, quint32 oldFormId);

    // Re-points the references in a REFR record (base object, owner, and any
    // attached scripts) that target renumbered records.
    static void repointRefr(RefrRecord& record, const RenumberMap& map,
                            Result& result);

    // Re-points the cell owner reference that targets a renumbered record.
    static void repointCell(CellRecord& record, const RenumberMap& map,
                            Result& result);

    // Convenience: renumbers a single form ID and records it in the result.
    static quint32 renumberId(const RenumberMap& map, quint32 oldFormId,
                              Result& result);
};

#endif // PLUGINCOMPACTOR_H

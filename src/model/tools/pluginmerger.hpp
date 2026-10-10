#ifndef PLUGINMERGER_HPP
#define PLUGINMERGER_HPP

#include "recordmerger.hpp"

#include <QString>

// Phase 10.3: applies a three-way merge plan to a new plugin file.
//
// The merge plan (RecordMergePlan) is record-level; this writes the chosen
// records verbatim (same subrecord payloads the branches carry) into a
// TES4 plugin through ESMWriter. Raw payload preservation means the merged
// plugin round-trips through the same reader as its inputs — no record
// re-serialization loss.
namespace openck {

struct MergeWriteReport
{
    QString outputPath;
    int recordsWritten = 0;
    int conflictsRetained = 0;   // conflicts kept from "mine"
    int skipped = 0;
    QStringList conflictReasons; // for the dialog's conflict list
    QString error;

    bool ok() const { return error.isEmpty(); }
};

class PluginMerger
{
public:
    /// Writes the merged plugin to outputPath. On any failure the output is
    /// not committed and the original file (if any) is left untouched.
    static MergeWriteReport mergeToFile(const QString& basePath,
                                        const QString& minePath,
                                        const QString& theirsPath,
                                        const QString& outputPath);
};

} // namespace openck

#endif // PLUGINMERGER_HPP

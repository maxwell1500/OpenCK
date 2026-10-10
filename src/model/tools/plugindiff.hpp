#ifndef PLUGINDIFF_HPP
#define PLUGINDIFF_HPP

#include "../../../libs/files/esm/subrecordsnapshot.hpp"

#include <QString>
#include <QVector>

// Phase 10.1: plugin-to-plugin record diffing.
//
// Snapshots are taken straight off disk (openck::collectRecordSnapshots), so
// the diff runs on any two plugin files without loading them into a Data
// instance, and it sees every record (including ones the editors do not
// model). Subrecords are aligned by name with an LCS walk; runs of the same
// name are paired positionally so an edit in the middle of a NAME block is
// reported as Modified rather than a delete+add cascade.

namespace openck {

enum class DiffStatus { Same, Added, Removed, Modified };

inline QString diffStatusName(DiffStatus s)
{
    switch (s)
    {
        case DiffStatus::Same:     return QStringLiteral("Same");
        case DiffStatus::Added:    return QStringLiteral("Added");
        case DiffStatus::Removed:  return QStringLiteral("Removed");
        case DiffStatus::Modified: return QStringLiteral("Modified");
    }
    return QString();
}

struct SubDiff
{
    NAME name = 0;
    DiffStatus status = DiffStatus::Same;
    QByteArray left;
    QByteArray right;
};

struct RecordDiffEntry
{
    NAME type = 0;
    quint32 formId = 0;
    DiffStatus status = DiffStatus::Same;
    QString editorId;            // EDID payload, for the label column
    QVector<SubDiff> subs;
};

struct PluginDiffReport
{
    QString leftPath;
    QString rightPath;
    QVector<RecordDiffEntry> records;   // differing records only (Same filtered)
    int sameRecords = 0;
    int addedRecords = 0;
    int removedRecords = 0;
    int modifiedRecords = 0;
    QString error;                      // non-empty when a side could not be read

    bool ok() const { return error.isEmpty(); }
};

class PluginDiffer
{
public:
    /// Compares two plugin files record-by-record. Only differing records
    /// land in the report; the counts carry the totals.
    static PluginDiffReport diff(const QString& leftPath, const QString& rightPath);
};

} // namespace openck

#endif // PLUGINDIFF_HPP

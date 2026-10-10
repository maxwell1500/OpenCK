#ifndef RECORDMERGER_HPP
#define RECORDMERGER_HPP

#include <QString>
#include <QVector>

#include "../../../libs/files/esm/subrecordsnapshot.hpp"

// Phase 10.3: semantic three-way plugin merge.
//
// Two modders editing the same plugin produce conflicting work when both
// change the same record, and subtler breakage when both allocate a new
// record at the same FormID for different objects. The merge runs on disk
// snapshots (openck::collectRecordSnapshots) so the common ancestor, the
// two branches and the base plugins never need to be loaded:
//
//   * records only one branch touched are taken from that branch;
//   * records both branches changed identically are taken once;
//   * records both branches changed differently become conflicts, reported
//     with a reason, and the left ("mine") side wins in the output — the
//     caller resolves them and re-runs the merge;
//   * a record the branch adds with a FormID the other branch already
//     owns (or that changed on the other side) is an ID conflict; the
//     merged plan marks it for re-allocation in the destination's space.
//
// The resulting plan is a pure description; writing it is the caller's job
// (via Document/Data), so the merge itself stays testable without a game
// installation.

namespace openck {

struct MergeAction
{
    enum Kind { Take, Conflict, Skip };
    Kind kind = Take;
    QString reason;
    // Snapshot chosen by the plan (the "mine" side for conflicts).
    RecordSnapshot record;
};

struct RecordMergePlan
{
    QVector<MergeAction> actions;   // every merged record, in (type,formId) order
    int conflicts = 0;
    int idConflicts = 0;
    int records = 0;
};

class RecordMerger
{
public:
    /// threeWay(base, mine, theirs). Branch names are only used for report
    /// text.
    static RecordMergePlan merge(const QString& basePath, const QString& minePath,
                                 const QString& theirsPath);
};

} // namespace openck

#endif // RECORDMERGER_HPP

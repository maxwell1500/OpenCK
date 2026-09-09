#ifndef FORMIDCOMPACTOR_H
#define FORMIDCOMPACTOR_H

#include <QString>
#include <QVector>
#include <QHash>

class Data;

/// Compacts a plugin's own record FormIDs into the ESL (light master) range
/// 0x000-0xFFF so the file can be saved as an .esl. Operates on the records
/// owned by the active document (State_Modified / State_ModifiedOnly).
///
/// Scope: rewrites each owned record's own formId plus the FormID reference
/// fields that record types expose as typed members, the known FormID-bearing
/// raw subrecords, and a generic fallback pass that rewrites any remaining
/// u32 in opaque raw subrecords that matches a remapped FormID. XPRM is
/// skipped (no FormIDs).
class FormIdCompactor
{
public:
    explicit FormIdCompactor(Data& data) : mData(data) {}

    /// Remap owned records into 0x000-0xFFF. Returns the number of records
    /// remapped, or -1 if the plugin owns more than 4096 records (the ESL
    /// ceiling). The high byte / master bits of each new FormID are preserved
    /// from the record's original value.
    int compact();

    /// Total records that own FormIDs eligible for compaction.
    int ownedRecordCount() const { return mOwned; }

    /// Number of records whose FormID changed.
    int remappedCount() const { return mRemapped; }

    /// Number of reference fields rewritten (typed members, raw subrecords,
    /// and component-held FormIDs).
    int rewrittenReferences() const { return mRewritten; }

    /// Always empty (retained for API compatibility; compaction no longer
    /// refuses on unhandled subrecords).
    QString refusalMessage() const { return mRefusalMessage; }

private:
    Data& mData;
    int mOwned = 0;
    int mRemapped = 0;
    int mRewritten = 0;
    QString mRefusalMessage;
};

#endif // FORMIDCOMPACTOR_H

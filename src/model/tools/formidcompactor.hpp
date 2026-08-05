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
/// fields that record types expose as typed members (e.g. RELA parent/child,
/// SHOU words, ECZN zone/location, IPDS impact list, HAZD image space, IPCT
/// effect). It also rewrites the known FormID-bearing raw subrecords of the
/// common record types (cell, refr, npc, dial, quest, alch/ingr/ench/spell/
/// magic EFID, and the KWDA keyword arrays on the keyword-using records) and
/// the simple FormID members of the TESEnchantableForm and
/// BGSPickupPutdownSounds components. Raw payloads whose FormID layout is
/// unknown or Starfield-specific (e.g. XPRM placement) are left untouched;
/// production compaction of those is the documented follow-up.
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

private:
    Data& mData;
    int mOwned = 0;
    int mRemapped = 0;
    int mRewritten = 0;
};

#endif // FORMIDCOMPACTOR_H

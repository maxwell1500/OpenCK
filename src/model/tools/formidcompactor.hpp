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
/// effect, LCTN parent/linked refs). It also rewrites the known FormID-bearing
/// raw subrecords of the common record types (cell, refr, npc, dial, quest,
/// alch/ingr/ench/spell/magic EFID, and the KWDA keyword arrays on the
/// keyword-using records), the FO4/Starfield placed-reference table (XAPR,
/// XLKR, XLRT, XTEL, XLYR, XMSP, XRFG, XMBR, XASP, XEZN, XLRL plus the
/// Starfield-only XLCN/XTNM/XPCK/XPCS/XLIB/XATR/XNDP/XSAD/XCZR/XCZC/TODD/
/// GNAM/HNAM/JNAM/XCOL/XLOC/XVL2/XLMS/XPDO/XPLK), the Starfield LCTN rebuild
/// arrays (ACPR/LCPR/RCPR/ACUR/LCUR/RCUR/ACUN/LCUN/RCUN/ACSR/LCSR/RCSR/
/// ACID/LCID/ACEP/LCEP/ACEC/LCEC/RCEC), and the simple FormID members of the
/// TESEnchantableForm and BGSPickupPutdownSounds components. XPRM carries no
/// FormIDs (a primitive descriptor of bounds, color and shape type), so no
/// rewrite is needed there. Other opaque raw payloads are checked after the
/// known rewrites run: if any still holds a value equal to a remapped FormID
/// (an unhandled reference), compaction is refused with -2 instead of saving
/// a desynced file.
class FormIdCompactor
{
public:
    explicit FormIdCompactor(Data& data) : mData(data) {}

    /// Remap owned records into 0x000-0xFFF. Returns the number of records
    /// remapped, -1 if the plugin owns more than 4096 records (the ESL
    /// ceiling), or -2 if an opaque raw payload still holds an unhandled
    /// FormID reference (compaction refused rather than corrupt the file).
    /// The high byte / master bits of each new FormID are preserved
    /// from the record's original value.
    int compact();

    /// Total records that own FormIDs eligible for compaction.
    int ownedRecordCount() const { return mOwned; }

    /// Number of records whose FormID changed.
    int remappedCount() const { return mRemapped; }

    /// Number of reference fields rewritten (typed members, raw subrecords,
    /// and component-held FormIDs).
    int rewrittenReferences() const { return mRewritten; }

    /// Full diagnostic when compact() returns -2: lists every unhandled
    /// opaque FormID reference found (record, subrecord name, byte offset,
    /// stale FormID). Empty when compaction succeeded or was refused for
    /// another reason (ESL ceiling).
    QString refusalMessage() const { return mRefusalMessage; }

private:
    Data& mData;
    int mOwned = 0;
    int mRemapped = 0;
    int mRewritten = 0;
    QString mRefusalMessage;
};

#endif // FORMIDCOMPACTOR_H

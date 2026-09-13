#ifndef TES3RECORD_H
#define TES3RECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;

// Generic Morrowind (TES3) record: the on-disk format is uniform enough
// (16-byte record header, 8-byte subrecord headers) that every record type
// can round-trip through this without a bespoke loader. The first NAME
// subrecord is the editor id; everything else is preserved positionally in
// rawSubRecords keyed by loadOrder.
//
// Components (Phase 2b) handle common subrecords (FULL, MODL, ICON, DATA)
// while unknown or type-specific subrecords remain as raw bytes for lossless
// round-trip.
struct Tes3Record {
    openck::FormComponents components;
    NAME code = 0;              // on-disk record type
    QString editorId;           // first NAME subrecord
    quint32 formId = 0;         // synthetic in-memory id (TES3 has none)
    quint32 flags = 0;          // record header flags
    quint32 unknownHeader = 0;  // record header unknown field
    int nameIndex = -1;         // index of the editor-id subrecord in loadOrder
    QByteArray nameRaw;         // exact on-disk NAME payload (may include NULs)
    QVector<RawSubRecord> rawSubRecords;
    QVector<NAME> loadOrder;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const Tes3Record& l, const Tes3Record& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.unknownHeader == r.unknownHeader && l.nameIndex == r.nameIndex
        && l.nameRaw == r.nameRaw
        && l.rawSubRecords == r.rawSubRecords && l.loadOrder == r.loadOrder;
}

inline bool operator!=(const Tes3Record& l, const Tes3Record& r)
{
    return !(l == r);
}
#endif

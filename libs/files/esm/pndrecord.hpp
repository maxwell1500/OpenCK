#ifndef PNDRECORD_H
#define PNDRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;

// Planet (PNDT) record. Starfield's planet editor target. The record leads
// with BFCB/BFCE component-marker blocks (keyword form, full name, model) and
// then the planet fields. Only the unambiguous fields are typed here; every
// subrecord's on-disk order is preserved via mOrder so PNDT records round-trip
// byte-for-byte against the real Starfield.esm. Unknown subrecords keep their
// exact bytes in rawSubRecords (in order); known names are re-emitted from the
// typed fields during save, positioned per mOrder.
struct PndRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;       // FNAM
    QString starSystem;      // ANAM
    float temperature = 0.0f; // TEMP
    float density = 1.0f;    // DENS
    float phase = 1.0f;      // PHLA
    quint32 resources = 0;   // RSCS
    QVector<RawSubRecord> rawSubRecords;
    QVector<quint32> mOrder; // subrecord names in on-disk order

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const PndRecord& l, const PndRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.starSystem == r.starSystem && l.temperature == r.temperature
        && l.density == r.density && l.phase == r.phase
        && l.resources == r.resources && l.rawSubRecords == r.rawSubRecords
        && l.mOrder == r.mOrder && l.components == r.components;
}
inline bool operator!=(const PndRecord& l, const PndRecord& r) { return !(l == r); }
#endif

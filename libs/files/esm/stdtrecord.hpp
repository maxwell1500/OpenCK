#ifndef StdtRecordRECORD_H
#define StdtRecordRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "baseformcomponents.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;

// BGSStarDataComponent_Component::DATA — the star-catalogue fields the galaxy
// map shows. Layout taken from xEdit wbDefinitionsSF1 (the DATA union's
// BGSStarDataComponent_Component branch) and verified against the shipped STDT
// records: two length-prefixed strings, four floats, then three uint32s (38
// bytes for the surveyed records).
struct StarDataComponent
{
    QString catalogueId;       // wbLenString
    QString spectralClass;     // wbLenString (e.g. "G5")
    float magnitude = 0.0f;
    float massSolarMasses = 0.0f;
    float innerHabitableZone = 0.0f;
    float outerHabitableZone = 0.0f;
    quint32 hip = 0;
    quint32 radius = 0;
    quint32 temperatureK = 0;
    bool valid = false;
};

// STDT (Star) — Starfield's galaxy-map star record. Subrecords are stored raw
// and replayed in order (so the record round-trips byte-for-byte); the typed
// accessors read the fields the galaxy view needs: display name (ANAM), the
// 3D parsec position (BNAM) used to lay out the map, system id (DNAM), colour
// (ENAM), and the sun-preset / binary-companion links. The catalogue fields
// live in the BGSStarDataComponent_Component base-form component.
struct StdtRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();

    const RawSubRecord* findSubrecord(NAME name) const;
    QString starName() const;                                  // ANAM
    bool hasParsecLocation() const;                            // BNAM present
    void parsecLocation(float& x, float& y, float& z) const;   // BNAM
    quint32 systemId() const;                                  // DNAM
    QByteArray color() const;                                  // ENAM
    quint32 binaryStar() const;                                // SNAM
    quint32 sunPreset() const;                                 // PNAM

    StarDataComponent starData() const;
};

inline bool operator==(const StdtRecord& l, const StdtRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const StdtRecord& l, const StdtRecord& r)
{
    return !(l == r);
}
#endif

#ifndef CELLRECORD_H
#define CELLRECORD_H

#include "common.hpp"
#include "records.hpp"
#include "../../components/formcomponents.hpp"

#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

struct CellRecord
{
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    // Width the DATA flags were stored with (Starfield writes 4 bytes,
    // older games 1); preserved so an untouched save is payload-identical.
    quint8 dataWidth = 1;
    // Bytes past the decoded prefix that a variant-width subrecord carried
    // (exterior XCLC is 12 bytes, not 8). Re-emitted verbatim on save.
    QByteArray dataExtra;
    quint32 cellX = 0;
    quint32 cellY = 0;
    QByteArray xclcExtra;
    quint32 owner = 0;
    quint32 lockLevel = 0;
    QString cellName;
    bool hasWaterHeight = false;
    float waterHeight = 0.0f;
    QVector<RawSubRecord> rawSubRecords;
    // Subrecord order seen at load, so an untouched save re-emits the
    // record payload-identical (the snapshot gate compares positionally).
    QVector<NAME> loadOrder;
    bool hasData = false;
    bool hasFull = false;
    bool hasXclc = false;
    bool hasOwner = false;
    bool hasLock = false;
    bool hasXclw = false;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const CellRecord& l, const CellRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.cellX == r.cellX && l.cellY == r.cellY && l.owner == r.owner
        && l.lockLevel == r.lockLevel && l.cellName == r.cellName
        && l.hasWaterHeight == r.hasWaterHeight && l.waterHeight == r.waterHeight
        && l.components == r.components
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const CellRecord& l, const CellRecord& r)
{
    return !(l == r);
}

#endif // CELLRECORD_H

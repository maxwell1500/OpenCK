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
    quint32 formId;
    quint8 flags;
    quint32 cellX;
    quint32 cellY;
    quint32 owner;
    quint32 lockLevel;
    QString cellName;
    QVector<RawSubRecord> rawSubRecords;

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
        && l.components == r.components
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const CellRecord& l, const CellRecord& r)
{
    return !(l == r);
}

#endif // CELLRECORD_H

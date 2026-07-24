#ifndef LOCATIONRECORD_H
#define LOCATIONRECORD_H

#include "common.hpp"
#include "records.hpp"

#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

struct LocationRecord
{
    QString editorId;
    quint32 formId;
    quint32 flags;
    QString locationName;
    quint32 parentId;
    quint32 x;
    quint32 y;
    quint32 z;
    QVector<RawSubRecord> rawSubRecords;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const LocationRecord& l, const LocationRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.locationName == r.locationName && l.parentId == r.parentId
        && l.x == r.x && l.y == r.y && l.z == r.z && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const LocationRecord& l, const LocationRecord& r)
{
    return !(l == r);
}

#endif // LOCATIONRECORD_H

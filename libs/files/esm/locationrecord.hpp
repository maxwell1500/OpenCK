#ifndef LOCATIONRECORD_H
#define LOCATIONRECORD_H

#include "common.hpp"
#include "records.hpp"
#include "../../components/formcomponents.hpp"

#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

struct LocationRecord
{
    openck::FormComponents components;
    QString editorId;
    quint32 formId;
    quint32 flags;
    QString locationName;
    quint32 parentId;
    quint32 x;
    quint32 y;
    quint32 z;
    QVector<RawSubRecord> rawSubRecords;

    // One linked-reference group (Skyrim LCTN): an XNAM subrecord holds the
    // LocationRefType (LCRT) form ID, followed by one LNAM subrecord per
    // linked location form ID.
    struct LinkedRef
    {
        quint32 refTypeId = 0;
        QVector<quint32> linkedIds;
    };
    QVector<LinkedRef> linkedRefs;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const LocationRecord::LinkedRef& l, const LocationRecord::LinkedRef& r)
{
    return l.refTypeId == r.refTypeId && l.linkedIds == r.linkedIds;
}

inline bool operator==(const LocationRecord& l, const LocationRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.locationName == r.locationName && l.parentId == r.parentId
        && l.x == r.x && l.y == r.y && l.z == r.z
        && l.components == r.components
        && l.linkedRefs == r.linkedRefs
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const LocationRecord& l, const LocationRecord& r)
{
    return !(l == r);
}

#endif // LOCATIONRECORD_H

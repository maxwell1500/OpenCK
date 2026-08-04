#ifndef EcznRECORD_H
#define EcznRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct EcznRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 zoneFormId = 0;
    quint32 locationFormId = 0;
    quint32 unusedFormId = 0;
    quint8 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const EcznRecord& l, const EcznRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.zoneFormId == r.zoneFormId && l.locationFormId == r.locationFormId
        && l.unusedFormId == r.unusedFormId && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const EcznRecord& l, const EcznRecord& r)
{
    return !(l == r);
}
#endif

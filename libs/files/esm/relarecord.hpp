#ifndef RelaRECORD_H
#define RelaRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct RelaRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 parentFormId = 0;
    quint32 childFormId = 0;
    quint16 rank = 0;
    quint16 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const RelaRecord& l, const RelaRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.parentFormId == r.parentFormId && l.childFormId == r.childFormId
        && l.rank == r.rank && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const RelaRecord& l, const RelaRecord& r)
{
    return !(l == r);
}
#endif

#ifndef ImadRECORD_H
#define ImadRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct ImadRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const ImadRecord& l, const ImadRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const ImadRecord& l, const ImadRecord& r)
{
    return !(l == r);
}
#endif

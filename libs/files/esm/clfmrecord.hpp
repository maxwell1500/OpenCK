#ifndef ClfmRECORD_H
#define ClfmRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct ClfmRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 colorRgba = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const ClfmRecord& l, const ClfmRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.colorRgba == r.colorRgba && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const ClfmRecord& l, const ClfmRecord& r)
{
    return !(l == r);
}
#endif

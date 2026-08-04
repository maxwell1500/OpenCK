#ifndef ArtvRECORD_H
#define ArtvRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct ArtvRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QString modelPath;
    quint8 category = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const ArtvRecord& l, const ArtvRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.modelPath == r.modelPath && l.category == r.category
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const ArtvRecord& l, const ArtvRecord& r)
{
    return !(l == r);
}
#endif

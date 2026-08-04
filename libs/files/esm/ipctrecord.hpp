#ifndef IpctRECORD_H
#define IpctRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct IpctRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QString modelPath;
    quint32 materialType = 0;
    quint32 flags = 0;
    quint32 effectFormId = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const IpctRecord& l, const IpctRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.modelPath == r.modelPath && l.materialType == r.materialType
        && l.flags == r.flags && l.effectFormId == r.effectFormId
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const IpctRecord& l, const IpctRecord& r)
{
    return !(l == r);
}
#endif

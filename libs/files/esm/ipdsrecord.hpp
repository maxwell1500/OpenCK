#ifndef IpdsRECORD_H
#define IpdsRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct IpdsRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QVector<quint32> impactFormIds;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const IpdsRecord& l, const IpdsRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.impactFormIds == r.impactFormIds && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const IpdsRecord& l, const IpdsRecord& r)
{
    return !(l == r);
}
#endif

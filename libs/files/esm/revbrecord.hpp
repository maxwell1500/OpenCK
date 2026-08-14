#ifndef RevbRECORD_H
#define RevbRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct RevbRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    // Raw reverb DATA payload (12 floats + flags); kept byte-exact so real
    // records round-trip without loss.
    QByteArray data;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const RevbRecord& l, const RevbRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.flags == r.flags && l.data == r.data
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const RevbRecord& l, const RevbRecord& r)
{
    return !(l == r);
}
#endif

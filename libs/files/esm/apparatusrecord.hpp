#ifndef AppaRECORD_H
#define AppaRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
class ESMReader;
class ESMWriter;

struct AppaRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    quint32 type = 0;
    float value = 0.0f;
    float weight = 0.0f;
    QVector<RawSubRecord> rawSubRecords;
    openck::FormComponents components;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const AppaRecord& l, const AppaRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.type == r.type && l.value == r.value && l.weight == r.weight
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const AppaRecord& l, const AppaRecord& r) { return !(l == r); }
#endif

#ifndef HairRECORD_H
#define HairRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
class ESMReader;
class ESMWriter;

struct HairRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    openck::FormComponents components;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const HairRecord& l, const HairRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const HairRecord& l, const HairRecord& r) { return !(l == r); }
#endif

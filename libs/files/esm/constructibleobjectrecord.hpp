#ifndef CobjRECORD_H
#define CobjRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
class ESMReader;
class ESMWriter;

struct CobjRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    openck::FormComponents components;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();

    // CNAM — the created-object FormID (a GBFM for ship parts). Starfield
    // stores it as a raw subrecord; this reads the first little-endian u32.
    quint32 createdObjectId() const;
};

inline bool operator==(const CobjRecord& l, const CobjRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const CobjRecord& l, const CobjRecord& r) { return !(l == r); }
#endif

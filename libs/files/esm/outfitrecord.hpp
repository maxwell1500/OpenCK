#ifndef OtftRECORD_H
#define OtftRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;

struct OutfitRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<quint32> itemFormIds;
    QVector<RawSubRecord> rawSubRecords;
    openck::FormComponents components;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const OutfitRecord& l, const OutfitRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.itemFormIds == r.itemFormIds
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const OutfitRecord& l, const OutfitRecord& r) { return !(l == r); }
#endif

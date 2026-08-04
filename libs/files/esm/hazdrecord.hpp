#ifndef HazdRECORD_H
#define HazdRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct HazdRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QString modelPath;
    quint8 limit = 0;
    float radius = 0.0f;
    float lifetime = 0.0f;
    quint32 imageSpace = 0;
    quint8 target = 0;
    quint8 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const HazdRecord& l, const HazdRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.modelPath == r.modelPath && l.limit == r.limit
        && l.radius == r.radius && l.lifetime == r.lifetime
        && l.imageSpace == r.imageSpace && l.target == r.target
        && l.flags == r.flags && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const HazdRecord& l, const HazdRecord& r)
{
    return !(l == r);
}
#endif

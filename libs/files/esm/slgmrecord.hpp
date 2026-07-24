#ifndef SlgmRECORD_H
#define SlgmRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include "../../components/tesfullname.hpp"
#include "../../components/tier1_components.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct SlgmRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString fullName;
    QString iconPath;
    QString modelPath;
    float weight = 0.0f;
    quint32 value = 0;
    quint32 capacity = 0;
    quint32 soul = 0;
    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const SlgmRecord& l, const SlgmRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.fullName == r.fullName && l.iconPath == r.iconPath
        && l.modelPath == r.modelPath && l.weight == r.weight
        && l.value == r.value && l.capacity == r.capacity && l.soul == r.soul
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}

inline bool operator!=(const SlgmRecord& l, const SlgmRecord& r)
{
    return !(l == r);
}
#endif

#ifndef AmmoRECORD_H
#define AmmoRECORD_H
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
struct AmmoRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString fullName;
    QString iconPath;
    QString modelPath;
    float speed = 0.0f;
    quint32 ammoFlags = 0;
    float weight = 0.0f;
    quint32 value = 0;
    float damage = 0.0f;
    float enchantmentCharge = 0.0f;
    int dataSize = 20;
    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const AmmoRecord& l, const AmmoRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.fullName == r.fullName && l.iconPath == r.iconPath
        && l.modelPath == r.modelPath && l.speed == r.speed
        && l.ammoFlags == r.ammoFlags && l.weight == r.weight
        && l.value == r.value && l.damage == r.damage
        && l.enchantmentCharge == r.enchantmentCharge
        && l.dataSize == r.dataSize
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}

inline bool operator!=(const AmmoRecord& l, const AmmoRecord& r)
{
    return !(l == r);
}
#endif

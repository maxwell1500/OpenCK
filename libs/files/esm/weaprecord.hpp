#ifndef WEAPRECORD_H
#define WEAPRECORD_H
#include "common.hpp"
#include "records.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct WeaponRecord {
    QString editorId;
    QString fullName;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    // Item-specific fields
    quint32 weaponType = 0;
    float damage = 0.0f;
    float speed = 0.0f;
    float reach = 0.0f;
    float weight = 0.0f;
    quint32 value = 0;
    quint32 enchantment = 0;
    QString iconPath;
    QString modelPath;
    quint32 magicSchool = 0;
    quint32 enchantLimit = 0;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const WeaponRecord& l, const WeaponRecord& r)
{
    return l.editorId == r.editorId && l.fullName == r.fullName && l.formId == r.formId && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords && l.weaponType == r.weaponType
        && l.damage == r.damage && l.speed == r.speed && l.reach == r.reach
        && l.weight == r.weight && l.value == r.value
        && l.enchantment == r.enchantment && l.iconPath == r.iconPath
        && l.modelPath == r.modelPath && l.magicSchool == r.magicSchool
        && l.enchantLimit == r.enchantLimit;
}

inline bool operator!=(const WeaponRecord& l, const WeaponRecord& r)
{
    return !(l == r);
}
#endif

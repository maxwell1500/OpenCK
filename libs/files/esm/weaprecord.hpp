#ifndef WEAPRECORD_H
#define WEAPRECORD_H
#include "common.hpp"
#include "records.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

#include <memory>
class ESMReader;
class ESMWriter;
struct WeaponRecord {
    openck::FormComponents components;
    QString editorId;
    QString fullName;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
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
    // Positional replay: loadOrder + loadIsRaw (1 = replay rawSubRecords in
    // order, 0 = emit the typed value/component). Presence spells the
    // subrecord the value came from so absent fields are never invented.
    QVector<NAME> loadOrder;
    QVector<quint8> loadIsRaw;
    bool hasEdid = false;
    bool hasFlags = false;
    NAME flagsName = NAME('FNAM');
    bool hasData = false;
    bool hasEamt = false;
    bool hasMdob = false;
    bool hasEnam = false;
    // Starfield packs these scalars at narrower widths (2-byte EAMT).
    quint8 eamtWidth = 4;
    quint8 mdobWidth = 4;
    quint8 enamWidth = 4;
    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<WeaponRecord> verbatimSnapshot;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const WeaponRecord& l, const WeaponRecord& r)
{
    return l.editorId == r.editorId && l.fullName == r.fullName && l.formId == r.formId && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords && l.weaponType == r.weaponType
        && l.damage == r.damage && l.speed == r.speed && l.reach == r.reach
        && l.weight == r.weight && l.value == r.value
        && l.enchantment == r.enchantment && l.iconPath == r.iconPath
        && l.modelPath == r.modelPath && l.magicSchool == r.magicSchool
        && l.enchantLimit == r.enchantLimit
        && l.components == r.components;
}

inline bool operator!=(const WeaponRecord& l, const WeaponRecord& r)
{
    return !(l == r);
}
#endif

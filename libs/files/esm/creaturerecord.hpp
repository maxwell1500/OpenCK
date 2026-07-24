#ifndef CreRECORD_H
#define CreRECORD_H
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

#pragma pack(push, 1)
struct CreaData {
    quint32 type = 0;
    quint16 acrobatics = 0;
    quint16 armorer = 0;
    quint16 athletics = 0;
    quint16 blade = 0;
    quint16 block = 0;
    quint16 blunt = 0;
    quint16 handToHand = 0;
    quint16 heavyArmor = 0;
    quint16 alchemy = 0;
    quint16 alteration = 0;
    quint16 conjuration = 0;
    quint16 destruction = 0;
    quint16 illusion = 0;
    quint16 mysticism = 0;
    quint16 restoration = 0;
    quint16 lightArmor = 0;
    quint16 marksman = 0;
    quint16 mercantile = 0;
    quint16 security = 0;
    quint16 sneak = 0;
    quint16 speechcraft = 0;
    quint16 health = 0;
    quint16 magicka = 0;
    quint16 fatigue = 0;
    quint16 damage = 0;
    quint16 strength = 0;
    quint16 intelligence = 0;
    quint16 willpower = 0;
    quint16 agility = 0;
    quint16 speed = 0;
    quint16 endurance = 0;
    quint16 personality = 0;
    quint16 luck = 0;
    quint16 xp = 0;
};
#pragma pack(pop)

struct CreatureRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    CreaData creaData = {};
    QString fullName;

    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const CreatureRecord& l, const CreatureRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && std::memcmp(&l.creaData, &r.creaData, sizeof(CreaData)) == 0
        && l.fullName == r.fullName
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const CreatureRecord& l, const CreatureRecord& r) { return !(l == r); }
#endif

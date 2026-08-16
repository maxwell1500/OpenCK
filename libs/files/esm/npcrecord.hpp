#ifndef NPCRECORD_H
#define NPCRECORD_H

#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"

#include <QString>
#include <QVector>
#include <cstring>

class ESMReader;
class ESMWriter;

#pragma pack(push, 1)
struct ACBS {
    quint32 flags = 0;
    quint16 baseSpell = 0;
    quint16 fatigue = 0;
    quint16 barterGold = 0;
    qint16 level = 0;
    quint16 calcMin = 0;
    quint16 calcMax = 0;
    quint16 speedMult = 0;
};
#pragma pack(pop)

struct NpcRecord
{
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    ACBS acbs = {};
    quint32 level = 0;

    QString fullName;

    quint32 race = 0;
    quint32 class_ = 0;
    quint32 faction = 0;
    quint32 sex = 0;

    quint32 health = 0;
    quint32 magicka = 0;
    quint32 stamina = 0;

    quint32 attack = 0;
    quint32 defense = 0;
    quint32 personality = 0;
    quint32 intelligence = 0;
    quint32 willpower = 0;
    quint32 agility = 0;
    quint32 luck = 0;
    quint32 disposition = 0;
    quint32 reputation = 0;

    QVector<quint32> spells;
    QVector<quint32> inventoryItems;
    QVector<quint32> relationships;
    QVector<quint32> factionIds;

    QVector<RawSubRecord> rawSubRecords;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const NpcRecord& l, const NpcRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && std::memcmp(&l.acbs, &r.acbs, sizeof(ACBS)) == 0
        && l.level == r.level && l.fullName == r.fullName
        && l.race == r.race && l.class_ == r.class_ && l.faction == r.faction
        && l.sex == r.sex
        && l.health == r.health && l.magicka == r.magicka && l.stamina == r.stamina
        && l.attack == r.attack && l.defense == r.defense
        && l.personality == r.personality && l.intelligence == r.intelligence
        && l.willpower == r.willpower && l.agility == r.agility
        && l.luck == r.luck && l.disposition == r.disposition
        && l.reputation == r.reputation
        && l.spells == r.spells && l.inventoryItems == r.inventoryItems
        && l.relationships == r.relationships
        && l.factionIds == r.factionIds
        && l.components == r.components
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const NpcRecord& l, const NpcRecord& r)
{
    return !(l == r);
}

#endif // NPCRECORD_H

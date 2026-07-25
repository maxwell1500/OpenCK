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

    quint32 aiIndex = 0;
    quint32 aiGlobal = 0;
    quint32 aiFacet = 0;
    quint32 aiRank = 0;
    quint32 aiFaction = 0;
    quint32 aiSound = 0;
    quint32 aiAlert = 0;
    quint32 aiCombat = 0;
    quint32 aiHazard = 0;
    quint32 aiClass = 0;
    quint32 aiRace = 0;
    quint32 aiCompany = 0;
    quint32 aiAggroRadius = 0;

    quint32 aiFactionRank = 0;
    quint32 aiFactionBase = 0;
    quint32 aiFactionMember = 0;
    quint32 aiFactionTarget = 0;
    quint32 aiFactionTargetRank = 0;
    quint32 aiFactionTargetBase = 0;
    quint32 aiFactionTargetMember = 0;
    quint32 aiFactionTargetClass = 0;
    quint32 aiFactionTargetRace = 0;
    quint32 aiFactionTargetCompany = 0;
    quint32 aiFactionTargetFacet = 0;
    quint32 aiFactionTargetSound = 0;
    quint32 aiFactionTargetAlert = 0;
    quint32 aiFactionTargetCombat = 0;
    quint32 aiFactionTargetHazard = 0;
    quint32 aiFactionTargetClassRank = 0;
    quint32 aiFactionTargetClassBase = 0;
    quint32 aiFactionTargetClassMember = 0;
    quint32 aiFactionTargetClassFacet = 0;
    quint32 aiFactionTargetClassSound = 0;
    quint32 aiFactionTargetClassAlert = 0;
    quint32 aiFactionTargetClassCombat = 0;
    quint32 aiFactionTargetClassHazard = 0;
    quint32 aiFactionTargetClassTarget = 0;
    quint32 aiFactionTargetClassTargetRank = 0;
    quint32 aiFactionTargetClassTargetBase = 0;
    quint32 aiFactionTargetClassTargetMember = 0;
    quint32 aiFactionTargetClassTargetFacet = 0;
    quint32 aiFactionTargetClassTargetSound = 0;
    quint32 aiFactionTargetClassTargetAlert = 0;
    quint32 aiFactionTargetClassTargetCombat = 0;
    quint32 aiFactionTargetClassTargetHazard = 0;

    QVector<quint32> spells;
    QVector<quint32> inventoryItems;
    QVector<quint32> relationships;

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
        && l.aiIndex == r.aiIndex && l.aiGlobal == r.aiGlobal
        && l.aiFacet == r.aiFacet && l.aiRank == r.aiRank
        && l.aiFaction == r.aiFaction && l.aiSound == r.aiSound
        && l.aiAlert == r.aiAlert && l.aiCombat == r.aiCombat
        && l.aiHazard == r.aiHazard && l.aiClass == r.aiClass
        && l.aiRace == r.aiRace && l.aiCompany == r.aiCompany
        && l.aiAggroRadius == r.aiAggroRadius
        && l.aiFactionRank == r.aiFactionRank && l.aiFactionBase == r.aiFactionBase
        && l.aiFactionMember == r.aiFactionMember
        && l.aiFactionTarget == r.aiFactionTarget
        && l.aiFactionTargetRank == r.aiFactionTargetRank
        && l.aiFactionTargetBase == r.aiFactionTargetBase
        && l.aiFactionTargetMember == r.aiFactionTargetMember
        && l.aiFactionTargetClass == r.aiFactionTargetClass
        && l.aiFactionTargetRace == r.aiFactionTargetRace
        && l.aiFactionTargetCompany == r.aiFactionTargetCompany
        && l.aiFactionTargetFacet == r.aiFactionTargetFacet
        && l.aiFactionTargetSound == r.aiFactionTargetSound
        && l.aiFactionTargetAlert == r.aiFactionTargetAlert
        && l.aiFactionTargetCombat == r.aiFactionTargetCombat
        && l.aiFactionTargetHazard == r.aiFactionTargetHazard
        && l.aiFactionTargetClassRank == r.aiFactionTargetClassRank
        && l.aiFactionTargetClassBase == r.aiFactionTargetClassBase
        && l.aiFactionTargetClassMember == r.aiFactionTargetClassMember
        && l.aiFactionTargetClassFacet == r.aiFactionTargetClassFacet
        && l.aiFactionTargetClassSound == r.aiFactionTargetClassSound
        && l.aiFactionTargetClassAlert == r.aiFactionTargetClassAlert
        && l.aiFactionTargetClassCombat == r.aiFactionTargetClassCombat
        && l.aiFactionTargetClassHazard == r.aiFactionTargetClassHazard
        && l.aiFactionTargetClassTarget == r.aiFactionTargetClassTarget
        && l.aiFactionTargetClassTargetRank == r.aiFactionTargetClassTargetRank
        && l.aiFactionTargetClassTargetBase == r.aiFactionTargetClassTargetBase
        && l.aiFactionTargetClassTargetMember == r.aiFactionTargetClassTargetMember
        && l.aiFactionTargetClassTargetFacet == r.aiFactionTargetClassTargetFacet
        && l.aiFactionTargetClassTargetSound == r.aiFactionTargetClassTargetSound
        && l.aiFactionTargetClassTargetAlert == r.aiFactionTargetClassTargetAlert
        && l.aiFactionTargetClassTargetCombat == r.aiFactionTargetClassTargetCombat
        && l.aiFactionTargetClassTargetHazard == r.aiFactionTargetClassTargetHazard
        && l.spells == r.spells && l.inventoryItems == r.inventoryItems
        && l.relationships == r.relationships
        && l.components == r.components
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const NpcRecord& l, const NpcRecord& r)
{
    return !(l == r);
}

#endif // NPCRECORD_H

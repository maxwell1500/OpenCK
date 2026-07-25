#include "npcrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void NpcRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'ACBS':
            {
                acbs = esm.readType<ACBS>();
                flags = acbs.flags;
                level = static_cast<quint32>(acbs.level);
                break;
            }
            case 'FULL': fullName = esm.readZString(); break;
            case 'RNAM': race = esm.readType<quint32>(); break;
            case 'CNAM': class_ = esm.readType<quint32>(); break;
            case 'ANAM': faction = esm.readType<quint32>(); break;
            case 'SPLO':
            {
                quint32 count = esm.readType<quint32>();
                spells.resize(count);
                for (int i = 0; i < count; i++)
                {
                    spells[i] = esm.readType<quint32>();
                }
                break;
            }
            case 'CNTO':
            {
                quint32 count = esm.readType<quint32>();
                inventoryItems.resize(count);
                for (int i = 0; i < count; i++)
                {
                    inventoryItems[i] = esm.readType<quint32>();
                }
                break;
            }
            default:
            {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                break;
            }
        }
    }
}

void NpcRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    ACBS saveAcbs = acbs;
    saveAcbs.flags = flags;
    saveAcbs.level = static_cast<qint16>(level);
    esm.writeSubData<ACBS>('ACBS', saveAcbs);
    esm.writeSubZString('FULL', fullName);
    esm.writeSubData<quint32>('RNAM', race);
    esm.writeSubData<quint32>('CNAM', class_);
    esm.writeSubData<quint32>('ANAM', faction);
    esm.startSubRecord('SPLO');
    esm.writeType<quint32>(spells.size());
    for (auto spell : spells)
    {
        esm.writeType<quint32>(spell);
    }
    esm.endSubRecord();
    esm.startSubRecord('CNTO');
    esm.writeType<quint32>(inventoryItems.size());
    for (auto item : inventoryItems)
    {
        esm.writeType<quint32>(item);
    }
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void NpcRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    acbs = {};
    level = 0;
    fullName = "";
    race = 0;
    class_ = 0;
    faction = 0;
    sex = 0;
    health = 0;
    magicka = 0;
    stamina = 0;
    attack = 0;
    defense = 0;
    personality = 0;
    intelligence = 0;
    willpower = 0;
    agility = 0;
    luck = 0;
    disposition = 0;
    reputation = 0;
    aiIndex = 0;
    aiGlobal = 0;
    aiFacet = 0;
    aiRank = 0;
    aiFaction = 0;
    aiSound = 0;
    aiAlert = 0;
    aiCombat = 0;
    aiHazard = 0;
    aiClass = 0;
    aiRace = 0;
    aiCompany = 0;
    aiAggroRadius = 0;
    aiFactionRank = 0;
    aiFactionBase = 0;
    aiFactionMember = 0;
    aiFactionTarget = 0;
    aiFactionTargetRank = 0;
    aiFactionTargetBase = 0;
    aiFactionTargetMember = 0;
    aiFactionTargetClass = 0;
    aiFactionTargetRace = 0;
    aiFactionTargetCompany = 0;
    aiFactionTargetFacet = 0;
    aiFactionTargetSound = 0;
    aiFactionTargetAlert = 0;
    aiFactionTargetCombat = 0;
    aiFactionTargetHazard = 0;
    aiFactionTargetClassRank = 0;
    aiFactionTargetClassBase = 0;
    aiFactionTargetClassMember = 0;
    aiFactionTargetClassFacet = 0;
    aiFactionTargetClassSound = 0;
    aiFactionTargetClassAlert = 0;
    aiFactionTargetClassCombat = 0;
    aiFactionTargetClassHazard = 0;
    aiFactionTargetClassTarget = 0;
    aiFactionTargetClassTargetRank = 0;
    aiFactionTargetClassTargetBase = 0;
    aiFactionTargetClassTargetMember = 0;
    aiFactionTargetClassTargetFacet = 0;
    aiFactionTargetClassTargetSound = 0;
    aiFactionTargetClassTargetAlert = 0;
    aiFactionTargetClassTargetCombat = 0;
    aiFactionTargetClassTargetHazard = 0;
    spells.clear();
    inventoryItems.clear();
    relationships.clear();
    rawSubRecords.clear();
}

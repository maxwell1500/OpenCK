#ifndef NPCVALIDATOR_H
#define NPCVALIDATOR_H

#include "validator.hpp"

#include "../world/idcollection.hpp"
#include "../world/record.hpp"
#include "../world/collection.hpp"
#include "../../../libs/files/esm/npcrecord.hpp"
#include "../../../libs/files/esm/racerecord.hpp"
#include "../../../libs/files/esm/factrecord.hpp"
#include "../../../libs/files/esm/spellrecord.hpp"

#include <QMap>

class NpcValidator : public Validator
{
public:
    QString name() const override { return "NPC Validation"; }

    void validate(const Data& data, Messages& messages) override
    {
        const auto& npcCollection = data.getNpcCollection();
        const auto& raceCollection = data.getRaceCollection();
        const auto& factCollection = data.getFactCollection();

        QMap<QString, int> editorIdMap;

        for (int i = 0; i < npcCollection.size(); ++i)
        {
            const Record<NpcRecord>& npcRec = npcCollection.getRecord(i);
            const NpcRecord& npc = npcRec.get();

            CkId ckId(CkId::Type_Npc_, npc.editorId);

            if (npc.editorId.isEmpty())
            {
                messages.append(ckId, "NPC has an empty EditorID.", "", Message::Error);
            }
            else
            {
                auto it = editorIdMap.find(npc.editorId);
                if (it != editorIdMap.end())
                {
                    messages.append(ckId,
                        "Duplicate EditorID '" + npc.editorId + "' found.",
                        "This EditorID already exists at index " + QString::number(it.value()),
                        Message::Error);
                }
                else
                {
                    editorIdMap.insert(npc.editorId, i);
                }
            }

            if (npc.race == 0)
            {
                messages.append(ckId,
                    "Race field is empty (formID 0).",
                    "Set a valid race reference.",
                    Message::Error);
            }
            else if (raceCollection.size() > 0)
            {
                bool raceExists = false;
                for (int ri = 0; ri < raceCollection.size(); ++ri)
                {
                    const Record<RaceRecord>& race = raceCollection.getRecord(ri);
                    if (race.get().formId == npc.race)
                    {
                        raceExists = true;
                        break;
                    }
                }
                if (!raceExists)
                {
                    messages.append(ckId,
                        "Race formID 0x" + QString::number(npc.race, 16) + " does not reference a valid race.",
                        "",
                        Message::Warning);
                }
            }

            if (npc.faction == 0)
            {
                messages.append(ckId,
                    "Faction field is empty (formID 0).",
                    "Set a valid faction reference.",
                    Message::Error);
            }
            else if (factCollection.size() > 0)
            {
                bool factionExists = false;
                for (int fi = 0; fi < factCollection.size(); ++fi)
                {
                    const Record<FactRecord>& faction = factCollection.getRecord(fi);
                    if (faction.get().formId == npc.faction)
                    {
                        factionExists = true;
                        break;
                    }
                }
                if (!factionExists)
                {
                    messages.append(ckId,
                        "Faction formID 0x" + QString::number(npc.faction, 16) + " does not reference a valid faction.",
                        "",
                        Message::Warning);
                }
            }

            if (npc.level > 9999)
            {
                messages.append(ckId,
                    "Level value " + QString::number(npc.level) + " is invalid. Must be 0-9999.",
                    "",
                    Message::Error);
            }

            for (int s = 0; s < npc.spells.size(); ++s)
            {
                quint32 spellId = npc.spells[s];
                if (spellId == 0)
                {
                    messages.append(ckId,
                        "Spell list at index " + QString::number(s) + " has an invalid formID (0).",
                        "Remove or set a valid spell reference.",
                        Message::Error);
                }
                else if (data.getSpellCollection().size() > 0)
                {
                    bool spellExists = false;
                    for (int spi = 0; spi < data.getSpellCollection().size(); ++spi)
                    {
                        const Record<SpellRecord>& spell = data.getSpellCollection().getRecord(spi);
                        if (spell.get().formId == spellId)
                        {
                            spellExists = true;
                            break;
                        }
                    }
                    if (!spellExists)
                    {
                        messages.append(ckId,
                            "Spell at index " + QString::number(s) + " references formID 0x" + QString::number(spellId, 16) + " which does not exist.",
                            "",
                            Message::Warning);
                    }
                }
            }

            for (int inv = 0; inv < npc.inventoryItems.size(); ++inv)
            {
                quint32 itemId = npc.inventoryItems[inv];
                if (itemId == 0)
                {
                    messages.append(ckId,
                        "Inventory item at index " + QString::number(inv) + " has an invalid formID (0).",
                        "Remove or set a valid item reference.",
                        Message::Error);
                }
            }
        }
    }
};

#endif // NPCVALIDATOR_H

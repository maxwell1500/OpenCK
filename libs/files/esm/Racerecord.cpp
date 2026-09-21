#include "Racerecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"

#include <QSet>

void RaceRecord::initComponents()
{
    components.clear();
    auto* flags = components.add<tescomponents::TESFlags_Component>();
    flags->setBitDefs({
        { "Playable", 0x01 },
        { "Child Race", 0x02 },
    });
    components.add<tescomponents::TESBodyParts_Component>();
}

void RaceRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub))
            {
                c->handleSubrecord(sub, esm);
                handled = true;
                break;
            }
        }
        if (handled) { loadIsRaw.append(0); continue; }

        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); hasEdid = true; loadIsRaw.append(0); break;
            default:
            {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                loadIsRaw.append(1);
                break;
            }
        }
    }
    if (auto* f = static_cast<tescomponents::TESFlags_Component*>(components.findByName(QStringLiteral("TESFlags")))) { flags = f->flags; }
}

void RaceRecord::save(ESMWriter& esm) const
{
    if (auto* f = static_cast<tescomponents::TESFlags_Component*>(const_cast<RaceRecord*>(this)->components.findByName(QStringLiteral("TESFlags")))) { f->flags = flags; }

    if (loadOrder.isEmpty())
    {
        if (hasEdid || !editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        components.saveAll(esm);
        for (const auto& raw : rawSubRecords)
            esm.writeRawSubRecord(raw);
        return;
    }

    QSet<NAME> seen;
    int rawCur = 0;
    tescomponents::TESFlags_Component* flagsComp =
        static_cast<tescomponents::TESFlags_Component*>(
            const_cast<RaceRecord*>(this)->components.findByName(QStringLiteral("TESFlags")));
    int flagsCur = 0;
    for (int p = 0; p < loadOrder.size(); ++p)
    {
        const NAME sub = loadOrder[p];
        seen.insert(sub);
        if (p >= loadIsRaw.size() || loadIsRaw[p] != 0)
        {
            if (rawCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            continue;
        }
        if (sub == NAME('EDID'))
        {
            esm.writeSubZString('EDID', editorId);
            continue;
        }
        if (sub == NAME('FNAM') || sub == NAME('FLAG'))
        {
            if (flagsComp && flagsCur >= 0 && flagsCur < flagsComp->flagsRaws.size() - 1)
                esm.writeRawSubRecord(RawSubRecord{ sub, flagsComp->flagsRaws[flagsCur] });
            else
                components.writeSubrecord(sub, esm);
            ++flagsCur;
            continue;
        }
        if (!components.writeSubrecord(sub, esm) && rawCur < rawSubRecords.size())
            esm.writeRawSubRecord(rawSubRecords[rawCur++]);
    }
    while (rawCur < rawSubRecords.size())
        esm.writeRawSubRecord(rawSubRecords[rawCur++]);
}

void RaceRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    raceFlags = 0;
    npcVariables.clear();
    faceData.clear();
    headData.clear();
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    rawSubRecords.clear();
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    initComponents();
}

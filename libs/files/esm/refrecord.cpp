#include "refrecord.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"

#include <QHash>

void RefrRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::BGSRefData_Component>();
}

void RefrRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
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
        if (handled) continue;

        switch (sub)
        {
        case 'EDID': editorId = esm.readZString(); hasEdid = true; break;
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
    auto* comp = static_cast<tescomponents::BGSRefData_Component*>(
        components.findByName(QStringLiteral("BGSRefData")));
    if (comp)
    {
        baseId = comp->baseId;
        posX = comp->posX; posY = comp->posY; posZ = comp->posZ;
        rotX = comp->rotX; rotY = comp->rotY; rotZ = comp->rotZ;
        scale = comp->scale;
        owner = comp->owner;
        lockLevel = comp->lockLevel;
        initiallyDisabled = comp->initiallyDisabled;
        scriptIds = comp->scriptIds;
    }
}

void RefrRecord::save(ESMWriter& esm) const
{
    auto* comp = static_cast<tescomponents::BGSRefData_Component*>(
        const_cast<RefrRecord*>(this)->components.findByName(QStringLiteral("BGSRefData")));
    if (comp)
    {
        comp->baseId = baseId;
        comp->posX = posX; comp->posY = posY; comp->posZ = posZ;
        comp->rotX = rotX; comp->rotY = rotY; comp->rotZ = rotZ;
        comp->scale = scale;
        comp->owner = owner;
        comp->lockLevel = lockLevel;
        comp->initiallyDisabled = initiallyDisabled;
        comp->scriptIds = scriptIds;
    }

    if (loadOrder.isEmpty())
    {
        if (!editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        if (comp)
            comp->save(esm);
        return;
    }

    // Placed references usually carry no EDID; writing an empty one emits
    // a 1-byte NUL subrecord that breaks payload-identical round-trips.
    // Subrecords replay in load order; values introduced after load (or
    // absent from it) are appended under the save() conditionals.
    QHash<NAME, QVector<int>> rawByName;
    for (int i = 0; i < rawSubRecords.size(); ++i)
        rawByName[rawSubRecords[i].name].append(i);
    QHash<NAME, int> rawCursor;

    bool wroteEdid = false, wroteName = false, wroteData = false,
        wroteXscl = false, wroteXown = false, wroteDnam = false,
        wroteXesp = false, wroteScri = false;

    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
        case 'EDID':
            if (!wroteEdid && (hasEdid || !editorId.isEmpty()))
            {
                esm.writeSubZString('EDID', editorId);
                wroteEdid = true;
            }
            break;
        case 'NAME':
            if (!wroteName && comp && comp->saveSubrecord(esm, sub, true))
                wroteName = true;
            break;
        case 'DATA':
            if (!wroteData && comp && comp->saveSubrecord(esm, sub, true))
                wroteData = true;
            break;
        case 'XSCL':
            if (!wroteXscl && comp && comp->saveSubrecord(esm, sub, true))
                wroteXscl = true;
            break;
        case 'XOWN':
            if (!wroteXown && comp && comp->saveSubrecord(esm, sub, true))
                wroteXown = true;
            break;
        case 'DNAM':
            if (!wroteDnam && comp && comp->saveSubrecord(esm, sub, true))
                wroteDnam = true;
            break;
        case 'XESP':
            if (!wroteXesp && comp && comp->saveSubrecord(esm, sub, true))
                wroteXesp = true;
            break;
        case 'SCRI':
            if (!wroteScri && comp && comp->saveSubrecord(esm, sub, true))
                wroteScri = true;
            break;
        default:
        {
            const QVector<int>& idx = rawByName[sub];
            int& cur = rawCursor[sub];
            if (cur < idx.size())
                esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
            break;
        }
        }
    }

    if (!wroteEdid && !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);
    if (comp)
    {
        if (!wroteName)
            comp->saveSubrecord(esm, NAME('NAME'), false);
        if (!wroteData)
            comp->saveSubrecord(esm, NAME('DATA'), false);
        if (!wroteXscl)
            comp->saveSubrecord(esm, NAME('XSCL'), false);
        if (!wroteXown)
            comp->saveSubrecord(esm, NAME('XOWN'), false);
        if (!wroteDnam)
            comp->saveSubrecord(esm, NAME('DNAM'), false);
        if (!wroteXesp)
            comp->saveSubrecord(esm, NAME('XESP'), false);
        if (!wroteScri)
            comp->saveSubrecord(esm, NAME('SCRI'), false);
    }

    // Leftover raws (no load order, e.g. assembled records) keep old order.
    for (int i = 0; i < rawSubRecords.size(); ++i)
    {
        const NAME sub = rawSubRecords[i].name;
        const QVector<int>& idx = rawByName[sub];
        int& cur = rawCursor[sub];
        if (cur < idx.size() && idx[cur] == i)
        {
            esm.writeRawSubRecord(rawSubRecords[i]);
            ++cur;
        }
    }
}

void RefrRecord::blank()
{
    editorId.clear();
    formId = 0;
    baseId = 0;
    posX = 0;
    posY = 0;
    posZ = 0;
    rotX = 0;
    rotY = 0;
    rotZ = 0;
    scale = 1.0f;
    owner = 0;
    lockLevel = 0;
    initiallyDisabled = false;
    scriptIds.clear();
    rawSubRecords.clear();
    loadOrder.clear();
    hasEdid = false;
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    initComponents();
}

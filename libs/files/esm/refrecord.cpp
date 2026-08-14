#include "refrecord.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"

void RefrRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::BGSRefData_Component>();
}

void RefrRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
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
        case 'EDID': editorId = esm.readZString(); break;
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

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
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
    initComponents();
}

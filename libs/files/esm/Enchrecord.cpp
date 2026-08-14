#include "Enchrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

void EnchRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
}

void EnchRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        }
        if (handled) continue;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'ENIT':
            {
                type = esm.readType<quint32>();
                charges = esm.readType<quint32>();
                costLimit = esm.readType<quint32>();
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
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) name = nameComp->fullName;
}

void EnchRecord::save(ESMWriter& esm) const
{
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(const_cast<EnchRecord*>(this)->components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) nameComp->fullName = name;

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.startSubRecord('ENIT');
    esm.writeType<quint32>(type);
    esm.writeType<quint32>(charges);
    esm.writeType<quint32>(costLimit);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void EnchRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    name.clear();
    costLimit = 0;
    charges = 0;
    enchantmentData = 0;
    charge = 0.0f;
    duration = 0;
    magnitude = 0.0f;
    type = 0;
    soulGem = 0;
    rawSubRecords.clear();
    initComponents();
}

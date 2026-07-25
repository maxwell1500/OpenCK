#include "wthrrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"

void WthrRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESWeatherData_Component>();
}

void WthrRecord::load(ESMReader& esm, bool)
{
    esm.readHeader();
    formId = esm.currentFormId();
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
    auto* comp = static_cast<tescomponents::TESWeatherData_Component*>(
        components.findByName(QStringLiteral("TESWeatherData")));
    if (comp)
    {
        sunTexture = comp->sunTexture;
        flags = comp->weatherFlags;
    }
}

void WthrRecord::save(ESMWriter& esm) const
{
    auto* comp = static_cast<tescomponents::TESWeatherData_Component*>(
        const_cast<WthrRecord*>(this)->components.findByName(QStringLiteral("TESWeatherData")));
    if (comp)
    {
        comp->sunTexture = sunTexture;
        comp->weatherFlags = flags;
    }

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void WthrRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    sunTexture.clear();
    rawSubRecords.clear();
    initComponents();
}

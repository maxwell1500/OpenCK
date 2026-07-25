#include "waterecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void WateRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    if (!components.findByName(QStringLiteral("TESFullName")))
        components.add<tescomponents::TESFullName_Component>();
    if (!components.findByName(QStringLiteral("TESTexture")))
        components.add<tescomponents::TESTexture_Component>();

    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        bool handled = false;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); handled = true; break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); handled = true; break;
            case 'ANAM': color = esm.readType<qint32>(); handled = true; break;
            case 'BNAM': windVel = esm.readType<float>(); handled = true; break;
            case 'CNAM': waveHeight = esm.readType<float>(); handled = true; break;
            case 'DNAM': damage = esm.readType<float>(); handled = true; break;
            case 'DATA':
            {
                waterFlags = esm.readType<quint32>();
                handled = true;
                break;
            }
            default: break;
        }
        if (handled) continue;

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

        RawSubRecord raw;
        raw.name = sub;
        esm.readRawSubData(raw.data);
        rawSubRecords.push_back(raw);
    }

    if (auto* n = static_cast<tescomponents::TESFullName_Component*>(
            components.findByName(QStringLiteral("TESFullName"))))
    {
        fullName = n->fullName;
    }
    if (auto* t = static_cast<tescomponents::TESTexture_Component*>(
            components.findByName(QStringLiteral("TESTexture"))))
    {
        iconPath = t->iconPath;
    }
}

void WateRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubData<quint32>('DATA', waterFlags);
    esm.writeSubData<qint32>('ANAM', color);
    if (windVel != 0.0f)
        esm.writeSubData<float>('BNAM', windVel);
    if (waveHeight != 0.0f)
        esm.writeSubData<float>('CNAM', waveHeight);
    if (damage != 0.0f)
        esm.writeSubData<float>('DNAM', damage);

    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void WateRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    fullName = "";
    iconPath = "";
    waterFlags = 0;
    color = 0;
    windVel = 0.0f;
    waveHeight = 0.0f;
    damage = 0.0f;
    rawSubRecords.clear();
    components.clear();
}

#include "keymrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier2_components.hpp"

void KeymRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    if (!components.findByName(QStringLiteral("TESFullName")))
        components.add<tescomponents::TESFullName_Component>();
    if (!components.findByName(QStringLiteral("TESModel")))
        components.add<tescomponents::TESModel_Component>();
    if (!components.findByName(QStringLiteral("TESTexture")))
        components.add<tescomponents::TESTexture_Component>();
    if (!components.findByName(QStringLiteral("BGSPickupPutdownSounds")))
        components.add<tescomponents::BGSPickupPutdownSounds_Component>();

    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        bool handled = false;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); handled = true; break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); handled = true; break;
            case 'DATA':
            {
                value = esm.readType<quint32>();
                weight = esm.readType<float>();
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
    if (auto* m = static_cast<tescomponents::TESModel_Component*>(
            components.findByName(QStringLiteral("TESModel"))))
    {
        modelPath = m->modelPath;
    }
    if (auto* t = static_cast<tescomponents::TESTexture_Component*>(
            components.findByName(QStringLiteral("TESTexture"))))
    {
        iconPath = t->iconPath;
    }
}

void KeymRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.startSubRecord('DATA');
    esm.writeType<quint32>(value);
    esm.writeType<float>(weight);
    esm.endSubRecord();

    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void KeymRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    fullName = "";
    iconPath = "";
    modelPath = "";
    weight = 0.0f;
    value = 0;
    rawSubRecords.clear();
    components.clear();
}

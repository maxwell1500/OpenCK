#include "ammorecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier2_components.hpp"

void AmmoRecord::load(ESMReader& esm, bool)
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
    if (!components.findByName(QStringLiteral("TESEnchantableForm")))
        components.add<tescomponents::TESEnchantableForm_Component>();

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
                speed = esm.readType<float>();
                ammoFlags = esm.readType<quint32>();
                weight = esm.readType<float>();
                value = esm.readType<quint32>();
                dataSize = 16;
                damage = 0.0f;
                if (esm.subLeft() >= 2)
                {
                    damage = static_cast<float>(esm.readType<quint16>());
                    dataSize += 2;
                }
                if (esm.subLeft() >= 2)
                {
                    esm.skip(2);
                    dataSize += 2;
                }
                if (esm.subLeft() > 0)
                {
                    dataSize += static_cast<int>(esm.subLeft());
                    esm.skip(static_cast<int>(esm.subLeft()));
                }
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

void AmmoRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.startSubRecord('DATA');
    esm.writeType<float>(speed);
    esm.writeType<quint32>(ammoFlags);
    esm.writeType<float>(weight);
    esm.writeType<quint32>(value);
    if (dataSize >= 18)
        esm.writeType<quint16>(static_cast<quint16>(damage));
    if (dataSize >= 20)
        esm.writeType<quint16>(0);
    esm.endSubRecord();

    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void AmmoRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    fullName = "";
    iconPath = "";
    modelPath = "";
    speed = 0.0f;
    ammoFlags = 0;
    weight = 0.0f;
    value = 0;
    damage = 0.0f;
    enchantmentCharge = 0.0f;
    dataSize = 20;
    rawSubRecords.clear();
    components.clear();
}

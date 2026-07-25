#include "lighrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void LighRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    if (!components.findByName(QStringLiteral("TESFullName")))
        components.add<tescomponents::TESFullName_Component>();
    if (!components.findByName(QStringLiteral("TESModel")))
        components.add<tescomponents::TESModel_Component>();
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
            case 'FNDS': fade = esm.readType<float>(); handled = true; break;
            case 'DATA':
            {
                time = esm.readType<qint32>();
                radius = esm.readType<quint32>();
                color = esm.readType<quint32>();
                lightFlags = esm.readType<quint32>();
                falloff = esm.readType<float>();
                fov = esm.readType<float>();
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

void LighRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.startSubRecord('DATA');
    esm.writeType<qint32>(time);
    esm.writeType<quint32>(radius);
    esm.writeType<quint32>(color);
    esm.writeType<quint32>(lightFlags);
    esm.writeType<float>(falloff);
    esm.writeType<float>(fov);
    esm.endSubRecord();
    if (fade != 0.0f)
        esm.writeSubData<float>('FNDS', fade);

    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void LighRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    fullName = "";
    iconPath = "";
    modelPath = "";
    time = 0;
    radius = 0;
    color = 0;
    lightFlags = 0;
    falloff = 0.0f;
    fov = 0.0f;
    fade = 0.0f;
    value = 0;
    weight = 0.0f;
    rawSubRecords.clear();
    components.clear();
}

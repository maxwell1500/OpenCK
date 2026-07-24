#include "msttrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void MsttRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();

    if (!components.findByName(QStringLiteral("TESModel")))
        components.add<tescomponents::TESModel_Component>();

    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;

        bool handled = false;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); handled = true; break;
            case 'FNAM': flags = esm.readType<quint32>(); handled = true; break;
            case 'DATA': msttFlags = esm.readType<quint32>(); handled = true; break;
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

    if (auto* m = static_cast<tescomponents::TESModel_Component*>(
            components.findByName(QStringLiteral("TESModel"))))
    {
        modelPath = m->modelPath;
    }
}

void MsttRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubData<quint32>('DATA', msttFlags);

    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void MsttRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    modelPath = "";
    msttFlags = 0;
    rawSubRecords.clear();
    components.clear();
}

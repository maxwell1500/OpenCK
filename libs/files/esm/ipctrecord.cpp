#include "ipctrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void IpctRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESModel_Component>();
}

void IpctRecord::load(ESMReader& esm, bool)
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
            case 'DATA':
            {
                if (esm.subLeft() >= 16)
                {
                    materialType = esm.readType<quint32>();
                    flags = esm.readType<quint32>();
                    esm.skip(4);
                    effectFormId = esm.readType<quint32>();
                }
                else
                {
                    RawSubRecord raw;
                    raw.name = sub;
                    esm.readRawSubData(raw.data);
                    rawSubRecords.push_back(raw);
                }
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
    auto* model = static_cast<tescomponents::TESModel_Component*>(components.findByName(QStringLiteral("TESModel")));
    if (model) modelPath = model->modelPath;
}

void IpctRecord::save(ESMWriter& esm) const
{
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<IpctRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);
    esm.startSubRecord('DATA');
    esm.writeRawData(reinterpret_cast<const char*>(&materialType), 4);
    esm.writeRawData(reinterpret_cast<const char*>(&flags), 4);
    static const quint32 zero = 0;
    esm.writeRawData(reinterpret_cast<const char*>(&zero), 4);
    esm.writeRawData(reinterpret_cast<const char*>(&effectFormId), 4);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void IpctRecord::blank()
{
    editorId.clear();
    formId = 0;
    modelPath.clear();
    materialType = 0;
    flags = 0;
    effectFormId = 0;
    rawSubRecords.clear();
    initComponents();
}

#include "hazdrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

void HazdRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
    components.add<tescomponents::TESModel_Component>();
}

void HazdRecord::load(ESMReader& esm, bool)
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
                const quint32 size = static_cast<quint32>(esm.subLeft());
                if (size >= 18)
                {
                    limit = esm.readType<quint8>();
                    esm.skip(1);
                    radius = esm.readType<float>();
                    lifetime = esm.readType<float>();
                    imageSpace = esm.readType<quint32>();
                    target = esm.readType<quint8>();
                    esm.skip(1);
                    flags = esm.readType<quint8>();
                    esm.skip(1);
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

void HazdRecord::save(ESMWriter& esm) const
{
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<HazdRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);

    QByteArray data;
    data.append(static_cast<char>(limit));
    data.append('\0');
    data.append(reinterpret_cast<const char*>(&radius), 4);
    data.append(reinterpret_cast<const char*>(&lifetime), 4);
    data.append(reinterpret_cast<const char*>(&imageSpace), 4);
    data.append(static_cast<char>(target));
    data.append('\0');
    data.append(static_cast<char>(flags));
    data.append('\0');
    esm.startSubRecord('DATA');
    esm.writeRawData(data.constData(), data.size());
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void HazdRecord::blank()
{
    editorId.clear();
    formId = 0;
    modelPath.clear();
    limit = 0;
    radius = 0.0f;
    lifetime = 0.0f;
    imageSpace = 0;
    target = 0;
    flags = 0;
    rawSubRecords.clear();
    initComponents();
}

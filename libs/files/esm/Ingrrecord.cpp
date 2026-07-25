#include "Ingrrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tier2_components.hpp"

void IngrRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
    components.add<tescomponents::BGSPickupPutdownSounds_Component>();
}

void IngrRecord::load(ESMReader& esm, bool)
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
            case 'DATA': {
                weight = esm.readType<float>();
                value = esm.readType<quint32>();
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
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(components.findByName(QStringLiteral("TESTexture")));
    if (tex) iconPath = tex->iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(components.findByName(QStringLiteral("TESModel")));
    if (model) modelPath = model->modelPath;
}

void IngrRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<IngrRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<IngrRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    components.saveAll(esm);
    esm.startSubRecord('DATA');
    esm.writeType<float>(weight);
    esm.writeType<quint32>(value);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void IngrRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    iconPath.clear();
    modelPath.clear();
    weight = 0.0f;
    value = 0;
    rawSubRecords.clear();
    initComponents();
}

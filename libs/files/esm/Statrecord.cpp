#include "Statrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

void StatRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
}

void StatRecord::load(ESMReader& esm, bool)
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
            case 'FNAM': flags = esm.readType<quint32>(); break;
            case 'RNAM': lodFlags = esm.readType<quint32>(); break;
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
    if (model) {
        modelPath = model->modelPath;
        lodModelPath = model->lodModelPath;
    }
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(components.findByName(QStringLiteral("TESTexture")));
    if (tex) {
        iconPath = tex->iconPath;
    }
}

void StatRecord::save(ESMWriter& esm) const
{
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<StatRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) {
        model->modelPath = modelPath;
        model->lodModelPath = lodModelPath;
    }
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<StatRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) {
        tex->iconPath = iconPath;
    }

    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    components.saveAll(esm);
    if (lodFlags != 0) {
        esm.writeSubData<quint32>('RNAM', lodFlags);
    }

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void StatRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    iconPath.clear();
    modelPath.clear();
    lodModelPath.clear();
    lodFlags = 0;
    rawSubRecords.clear();
    initComponents();
}

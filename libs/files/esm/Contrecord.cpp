#include "Contrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void ContRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
    components.add<tescomponents::TESContainer_Component>();
}

void ContRecord::load(ESMReader& esm, bool)
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
            case 'DATA': flags = esm.readType<quint32>(); break;
            case 'COCT': inventoryControl = esm.readType<quint32>(); break;
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

void ContRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<ContRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<ContRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);
    esm.writeSubData<quint32>('DATA', flags);
    esm.writeSubData<quint32>('COCT', inventoryControl);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void ContRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    iconPath.clear();
    modelPath.clear();
    contents = 0;
    inventoryControl = 0;
    weight = 0.0f;
    value = 0;
    rawSubRecords.clear();
    initComponents();
}

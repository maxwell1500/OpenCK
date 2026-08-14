#include "materialrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void MaterialRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
}

void MaterialRecord::load(ESMReader& esm, bool)
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
            case 'BKMN': materialName = esm.readZString(); break;
            case 'BNAM': bnam = esm.readZString(); break;
            case 'CNAM': cnam = esm.readZString(); break;
            case 'MNAM': texturePath = esm.readZString(); break;
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
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(components.findByName(QStringLiteral("TESTexture")));
    if (tex) iconPath = tex->iconPath;
}

void MaterialRecord::save(ESMWriter& esm) const
{
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<MaterialRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<MaterialRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;

    esm.writeSubZString('EDID', editorId);
    esm.writeSubZString('BKMN', materialName);
    components.saveAll(esm);
    esm.writeSubZString('BNAM', bnam);
    esm.writeSubZString('CNAM', cnam);
    esm.writeSubZString('MNAM', texturePath);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void MaterialRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    materialName.clear();
    name.clear();
    description.clear();
    iconPath.clear();
    modelPath.clear();
    bnam.clear();
    cnam.clear();
    texturePath.clear();
    materialType = 0;
    value = 0;
    weight = 0;
    health = 0;
    magicka = 0;
    stamina = 0;
    level = 0;
    race = 0;
    faction = 0;
    stage = 0;
    difficulty = 0;
    rawSubRecords.clear();
    initComponents();
}

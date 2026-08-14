#include "weaprecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tier2_components.hpp"
#include "../../components/tesfullname.hpp"

void WeaponRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
    components.add<tescomponents::TESModel_Component>();
    components.add<tescomponents::TESTexture_Component>();
    components.add<tescomponents::BGSPickupPutdownSounds_Component>();
}

void WeaponRecord::load(ESMReader& esm, bool)
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
                weaponType = esm.readType<quint32>();
                speed = esm.readType<float>();
                reach = esm.readType<float>();
                flags = esm.readType<quint32>();
                value = esm.readType<quint32>();
                weight = esm.readType<float>();
                break;
            }
            case 'DNAM': {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                break;
            }
            case 'EAMT': enchantment = esm.readType<quint32>(); break;
            case 'MDOB': magicSchool = esm.readType<quint32>(); break;
            case 'ENAM': enchantLimit = esm.readType<quint32>(); break;
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
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) fullName = nameComp->fullName;
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(components.findByName(QStringLiteral("TESTexture")));
    if (tex) iconPath = tex->iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(components.findByName(QStringLiteral("TESModel")));
    if (model) modelPath = model->modelPath;
}

void WeaponRecord::save(ESMWriter& esm) const
{
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(const_cast<WeaponRecord*>(this)->components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) nameComp->fullName = fullName;
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<WeaponRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;
    auto* model = static_cast<tescomponents::TESModel_Component*>(const_cast<WeaponRecord*>(this)->components.findByName(QStringLiteral("TESModel")));
    if (model) model->modelPath = modelPath;

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.startSubRecord('DATA');
    esm.writeType<quint32>(weaponType);
    esm.writeType<float>(speed);
    esm.writeType<float>(reach);
    esm.writeType<quint32>(flags);
    esm.writeType<quint32>(value);
    esm.writeType<float>(weight);
    esm.endSubRecord();
    esm.writeSubData<quint32>('EAMT', enchantment);
    esm.writeSubData<quint32>('MDOB', magicSchool);
    esm.writeSubData<quint32>('ENAM', enchantLimit);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void WeaponRecord::blank()
{
    editorId.clear();
    fullName.clear();
    formId = 0;
    flags = 0;
    weaponType = 0;
    damage = 0.0f;
    speed = 0.0f;
    reach = 0.0f;
    weight = 0.0f;
    value = 0;
    enchantment = 0;
    iconPath.clear();
    modelPath.clear();
    magicSchool = 0;
    enchantLimit = 0;
    rawSubRecords.clear();
    initComponents();
}

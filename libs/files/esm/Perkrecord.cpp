#include "Perkrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void PerkRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESTexture_Component>();
}

void PerkRecord::load(ESMReader& esm, bool)
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
            case 'DESC': description = esm.readZString(); break;
            case 'CTDA':
            {
                quint32 count = esm.readType<quint32>();
                conditions.resize(count);
                for (int i = 0; i < count; i++)
                    conditions[i] = esm.readType<quint32>();
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
}

void PerkRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<PerkRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;

    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubZString('DESC', description);
    components.saveAll(esm);
    esm.startSubRecord('CTDA');
    esm.writeType<quint32>(conditions.size());
    for (auto cond : conditions)
        esm.writeType<quint32>(cond);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void PerkRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    description.clear();
    requirements.clear();
    iconPath.clear();
    conditions.clear();
    rawSubRecords.clear();
    initComponents();
}

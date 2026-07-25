#include "Factrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void FactRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESTexture_Component>();
}

void FactRecord::load(ESMReader& esm, bool)
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
            case 'FULL': factionName = esm.readZString(); break;
            case 'XNAM':
            {
                quint32 count = esm.readType<quint32>();
                relations.resize(count);
                for (int i = 0; i < count; i++)
                    relations[i] = esm.readType<quint32>();
                break;
            }
            case 'RNAM':
            {
                quint32 count = esm.readType<quint32>();
                ranks.resize(count);
                for (int i = 0; i < count; i++)
                    ranks[i] = esm.readZString();
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

void FactRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<FactRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;

    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubZString('FULL', factionName);
    components.saveAll(esm);
    esm.startSubRecord('RNAM');
    esm.writeType<quint32>(ranks.size());
    for (auto rank : ranks)
        esm.writeZString(rank);
    esm.endSubRecord();
    esm.startSubRecord('XNAM');
    esm.writeType<quint32>(relations.size());
    for (auto rel : relations)
        esm.writeType<quint32>(rel);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void FactRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    factionName.clear();
    description.clear();
    iconPath.clear();
    ranks.clear();
    relations.clear();
    rawSubRecords.clear();
    initComponents();
}

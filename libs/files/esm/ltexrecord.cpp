#include "ltexrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void LtexRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESTexture_Component>();
}

void LtexRecord::load(ESMReader& esm, bool)
{
    esm.readHeader();
    formId = esm.currentFormId();
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
            case 'HNAM': havokMaterial = esm.readType<quint32>(); break;
            case 'SNAM':
            {
                while (esm.isSubLeft())
                    grassFormIds.append(esm.readType<quint32>());
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

void LtexRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<LtexRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;

    esm.writeSubZString('EDID', editorId);
    if (flags != 0)
        esm.writeSubData<quint32>('FNAM', flags);
    components.saveAll(esm);
    if (havokMaterial != 0)
        esm.writeSubData<quint32>('HNAM', havokMaterial);
    if (!grassFormIds.isEmpty())
    {
        esm.startSubRecord('SNAM');
        for (quint32 id : grassFormIds)
            esm.writeType<quint32>(id);
        esm.endSubRecord();
    }

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void LtexRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    iconPath.clear();
    havokMaterial = 0;
    grassFormIds.clear();
    rawSubRecords.clear();
    initComponents();
}

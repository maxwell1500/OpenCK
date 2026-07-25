#include "sounrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"

void SounRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::BGSSoundDescriptor_Component>();
}

void SounRecord::load(ESMReader& esm, bool)
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
    auto* comp = static_cast<tescomponents::BGSSoundDescriptor_Component*>(
        components.findByName(QStringLiteral("BGSSoundDescriptor")));
    if (comp)
    {
        soundFile = comp->soundFile;
        flags = comp->soundFlags;
    }
}

void SounRecord::save(ESMWriter& esm) const
{
    auto* comp = static_cast<tescomponents::BGSSoundDescriptor_Component*>(
        const_cast<SounRecord*>(this)->components.findByName(QStringLiteral("BGSSoundDescriptor")));
    if (comp)
    {
        comp->soundFile = soundFile;
        comp->soundFlags = flags;
    }

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void SounRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    soundFile.clear();
    rawSubRecords.clear();
    initComponents();
}

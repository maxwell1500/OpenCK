#include "Dialrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"

void DialRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFlags_Component>();
}

void DialRecord::load(ESMReader& esm, bool)
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
    if (auto* f = static_cast<tescomponents::TESFlags_Component*>(components.findByName(QStringLiteral("TESFlags")))) { flags = f->flags; }
}

void DialRecord::save(ESMWriter& esm) const
{
    if (auto* f = static_cast<tescomponents::TESFlags_Component*>(const_cast<DialRecord*>(this)->components.findByName(QStringLiteral("TESFlags")))) { f->flags = flags; }

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void DialRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    topicName = "";
    responseIds.clear();
    conditionIds.clear();
    animationIds.clear();
    emotionIds.clear();
    rawSubRecords.clear();
    initComponents();
}

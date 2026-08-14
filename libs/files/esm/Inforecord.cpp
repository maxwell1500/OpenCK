#include "Inforecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier3_components.hpp"

void InfoRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFlags_Component>();
}

void InfoRecord::load(ESMReader& esm, bool)
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
            case 'CNAM': responseText = esm.readZString(); break;
            case 'CTDA':
            {
                // Each CTDA is a 32-byte condition struct. There is no
                // parsed condition model yet, so preserve the bytes for
                // a lossless round-trip instead of misparsing them.
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                break;
            }
            case 'TLOI':
            {
                if (esm.subLeft() >= 4)
                    targetId = esm.readType<quint32>();
                if (esm.subLeft() > 0)
                    esm.skip(static_cast<int>(esm.subLeft()));
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
    auto* flagsComp = static_cast<tescomponents::TESFlags_Component*>(components.findByName(QStringLiteral("TESFlags")));
    if (flagsComp) {
        flags = flagsComp->flags;
    }
}

void InfoRecord::save(ESMWriter& esm) const
{
    auto* flagsComp = static_cast<tescomponents::TESFlags_Component*>(const_cast<InfoRecord*>(this)->components.findByName(QStringLiteral("TESFlags")));
    if (flagsComp) {
        flagsComp->flags = flags;
    }

    components.saveAll(esm);

    esm.writeSubZString('CNAM', responseText);
    esm.writeSubData<quint32>('TLOI', targetId);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void InfoRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    responseText.clear();
    voiceFile.clear();
    conditionIds.clear();
    conditions.clear();
    scriptFragment.clear();
    targetId = 0;
    scriptIds.clear();
    rawSubRecords.clear();
    initComponents();
}

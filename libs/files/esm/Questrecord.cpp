#include "Questrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

void QuestRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
}

void QuestRecord::load(ESMReader& esm, bool)
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
        if (sub == 'FULL')
        {
            auto* fn = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
            if (fn) fn->fullName = esm.readZString();
            continue;
        }
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'DNAM': questDesc = esm.readZString(); break;
            case 'DATA': questType = esm.readType<quint32>(); break;
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
    auto* fn = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
    if (fn) questName = fn->fullName;
}

void QuestRecord::save(ESMWriter& esm) const
{
    auto* fn = const_cast<QuestRecord*>(this)->components.findByName(QStringLiteral("TESFullName"));
    if (fn) static_cast<tescomponents::TESFullName_Component*>(fn)->fullName = questName;

    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    components.saveAll(esm);
    esm.writeSubZString('FULL', questName);
    esm.writeSubZString('DNAM', questDesc);
    esm.writeSubData<quint32>('DATA', questType);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void QuestRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    questName = "";
    questDesc = "";
    questType = 0;
    stageIds.clear();
    stageDescriptions.clear();
    objectiveIds.clear();
    aliasIds.clear();
    dialogueView = "";
    scriptIds.clear();
    rawSubRecords.clear();
    initComponents();
}

#include "Questrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void QuestRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': flags = esm.readType<quint32>(); break;
            case 'FULL': questName = esm.readZString(); break;
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
}

void QuestRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
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
}

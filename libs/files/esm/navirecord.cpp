#include "navirecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void NaviRecord::initComponents()
{
    components.clear();
}

void NaviRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    hasEdid = false;
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);
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
            case 'EDID': editorId = esm.readZString(); hasEdid = true; break;
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

void NaviRecord::save(ESMWriter& esm) const
{
    if (loadOrder.isEmpty())
    {
        if (hasEdid || !editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        for (const auto& raw : rawSubRecords)
            esm.writeRawSubRecord(raw);
        return;
    }
    int rawCur = 0;
    for (NAME sub : loadOrder)
    {
        if (sub == NAME('EDID'))
        {
            if (hasEdid)
                esm.writeSubZString('EDID', editorId);
        }
        else if (rawCur < rawSubRecords.size())
        {
            esm.writeRawSubRecord(rawSubRecords[rawCur++]);
        }
    }
    while (rawCur < rawSubRecords.size())
        esm.writeRawSubRecord(rawSubRecords[rawCur++]);
}

void NaviRecord::blank()
{
    editorId.clear();
    formId = 0;
    loadOrder.clear();
    hasEdid = false;
    rawSubRecords.clear();
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    initComponents();
}

#include "achrrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void AchrRecord::initComponents()
{
    components.clear();
}

void AchrRecord::load(ESMReader& esm, bool)
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

void AchrRecord::save(ESMWriter& esm) const
{
    if (hasEdid || !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void AchrRecord::blank()
{
    editorId.clear();
    formId = 0;
    rawSubRecords.clear();
    hasEdid = false;
    initComponents();
}
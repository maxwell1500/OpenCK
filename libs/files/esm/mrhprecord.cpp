#include "mrhprecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void MrhpRecord::initComponents()
{
    components.clear();
}

void MrhpRecord::load(ESMReader& esm, bool)
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
            case 'TCMP': morphPath = esm.readZString(); break;
            case 'MOBC': mobcFlags = esm.readType<quint32>(); break;
            case 'TMPP': templatePath = esm.readZString(); break;
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

void MrhpRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    if (!morphPath.isEmpty())
        esm.writeSubZString('TCMP', morphPath);
    esm.writeSubData<quint32>('MOBC', mobcFlags);
    if (!templatePath.isEmpty())
        esm.writeSubZString('TMPP', templatePath);

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void MrhpRecord::blank()
{
    editorId.clear();
    formId = 0;
    morphPath.clear();
    mobcFlags = 0xFF;
    templatePath.clear();
    rawSubRecords.clear();
    initComponents();
}

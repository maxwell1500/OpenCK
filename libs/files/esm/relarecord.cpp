#include "relarecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void RelaRecord::initComponents()
{
    components.clear();
}

void RelaRecord::load(ESMReader& esm, bool)
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
            case 'DATA':
            {
                parentFormId = esm.readType<quint32>();
                childFormId = esm.readType<quint32>();
                rank = esm.readType<quint16>();
                flags = esm.readType<quint16>();
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
}

void RelaRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.startSubRecord('DATA');
    esm.writeRawData(reinterpret_cast<const char*>(&parentFormId), 4);
    esm.writeRawData(reinterpret_cast<const char*>(&childFormId), 4);
    esm.writeRawData(reinterpret_cast<const char*>(&rank), 2);
    esm.writeRawData(reinterpret_cast<const char*>(&flags), 2);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void RelaRecord::blank()
{
    editorId.clear();
    formId = 0;
    parentFormId = 0;
    childFormId = 0;
    rank = 0;
    flags = 0;
    rawSubRecords.clear();
    initComponents();
}

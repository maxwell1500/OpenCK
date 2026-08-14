#include "ecznrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

void EcznRecord::initComponents()
{
    components.clear();
}

void EcznRecord::load(ESMReader& esm, bool)
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
                zoneFormId = esm.readType<quint32>();
                locationFormId = esm.readType<quint32>();
                unusedFormId = esm.readType<quint32>();
                flags = esm.readType<quint8>();
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

void EcznRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.startSubRecord('DATA');
    esm.writeRawData(reinterpret_cast<const char*>(&zoneFormId), 4);
    esm.writeRawData(reinterpret_cast<const char*>(&locationFormId), 4);
    esm.writeRawData(reinterpret_cast<const char*>(&unusedFormId), 4);
    esm.writeRawData(reinterpret_cast<const char*>(&flags), 1);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.writeRawSubRecord(raw);
    }
}

void EcznRecord::blank()
{
    editorId.clear();
    formId = 0;
    zoneFormId = 0;
    locationFormId = 0;
    unusedFormId = 0;
    flags = 0;
    rawSubRecords.clear();
    initComponents();
}

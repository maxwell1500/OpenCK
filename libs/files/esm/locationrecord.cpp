#include "locationrecord.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void LocationRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
        case 'EDID': editorId = esm.readZString(); break;
        case 'FNAM': flags = esm.readType<quint32>(); break;
        case 'FULL': locationName = esm.readZString(); break;
        case 'PNAM': parentId = esm.readType<quint32>(); break;
        case 'DATA':
        {
            x = esm.readType<quint32>();
            y = esm.readType<quint32>();
            z = esm.readType<quint32>();
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

void LocationRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubZString('FULL', locationName);
    esm.writeSubData<quint32>('PNAM', parentId);
    esm.startSubRecord('DATA');
    esm.writeType<quint32>(x);
    esm.writeType<quint32>(y);
    esm.writeType<quint32>(z);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void LocationRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    locationName = "";
    parentId = 0;
    x = 0;
    y = 0;
    z = 0;
    rawSubRecords.clear();
}

#include "worldspacerecord.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void WorldspaceRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
        case 'EDID': editorId = esm.readZString(); break;
        case 'FULL': name = esm.readZString(); break;
        case 'ICON': iconPath = esm.readZString(); break;
        case 'XNAM': waterType = esm.readType<quint32>(); break;
        case 'TNAM': templ = esm.readType<quint32>(); break;
        case 'WNAM': terrain = esm.readType<quint32>(); break;
        case 'MNAM': mapImage = esm.readZString(); break;
        case 'LODN': lodNoise = esm.readZString(); break;
        case 'BNAM': billboardTexture = esm.readZString(); break;
        case 'RNAM': music = esm.readType<quint32>(); break;
        case 'DNAM': dnam = esm.readType<quint32>(); break;
        case 'DATA':
        {
            dataMinX = esm.readType<qint32>();
            dataMinY = esm.readType<qint32>();
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

void WorldspaceRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubZString('FULL', name);
    esm.writeSubZString('ICON', iconPath);
    esm.writeSubData<quint32>('XNAM', waterType);
    esm.writeSubData<quint32>('TNAM', templ);
    esm.writeSubData<quint32>('WNAM', terrain);
    esm.writeSubZString('MNAM', mapImage);
    esm.writeSubZString('LODN', lodNoise);
    esm.writeSubZString('BNAM', billboardTexture);
    esm.writeSubData<quint32>('RNAM', music);
    esm.writeSubData<quint32>('DNAM', dnam);
    esm.startSubRecord('DATA');
    esm.writeType<qint32>(dataMinX);
    esm.writeType<qint32>(dataMinY);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void WorldspaceRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    name = "";
    iconPath = "";
    waterType = 0;
    climateId = 0;
    lightingId = 0;
    mapSize = 0;
    templ = 0;
    terrain = 0;
    mapImage = "";
    lodNoise = "";
    billboardTexture = "";
    music = 0;
    dnam = 0;
    dataMinX = 0;
    dataMinY = 0;
    cellIds.clear();
    navPointIds.clear();
    rawSubRecords.clear();
}

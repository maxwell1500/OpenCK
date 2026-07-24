#include "refrecord.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void RefrRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
        case 'NAME': baseId = esm.readType<quint32>(); break;
        case 'DATA':
        {
            posX = esm.readType<float>();
            posY = esm.readType<float>();
            posZ = esm.readType<float>();
            rotX = esm.readType<float>();
            rotY = esm.readType<float>();
            rotZ = esm.readType<float>();
            scale = esm.readType<float>();
            break;
        }
        case 'XOWN': owner = esm.readType<quint32>(); break;
        case 'DNAM': lockLevel = esm.readType<quint32>(); break;
        case 'XESP': initiallyDisabled = (esm.readType<quint32>() != 0); break;
        case 'SCRI':
        {
            quint32 count = esm.readType<quint32>();
            scriptIds.resize(count);
            for (int i = 0; i < count; i++)
            {
                scriptIds[i] = esm.readType<quint32>();
            }
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

void RefrRecord::save(ESMWriter& esm) const
{
    esm.writeSubData<quint32>('NAME', baseId);
    esm.startSubRecord('DATA');
    esm.writeType<float>(posX);
    esm.writeType<float>(posY);
    esm.writeType<float>(posZ);
    esm.writeType<float>(rotX);
    esm.writeType<float>(rotY);
    esm.writeType<float>(rotZ);
    esm.writeType<float>(scale);
    esm.endSubRecord();
    esm.writeSubData<quint32>('XOWN', owner);
    esm.writeSubData<quint32>('DNAM', lockLevel);
    esm.writeSubData<quint32>('XESP', initiallyDisabled ? 1 : 0);
    esm.startSubRecord('SCRI');
    esm.writeType<quint32>(scriptIds.size());
    for (auto id : scriptIds)
    {
        esm.writeType<quint32>(id);
    }
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void RefrRecord::blank()
{
    formId = 0;
    baseId = 0;
    posX = 0;
    posY = 0;
    posZ = 0;
    rotX = 0;
    rotY = 0;
    rotZ = 0;
    scale = 1.0;
    owner = 0;
    lockLevel = 0;
    initiallyDisabled = false;
    scriptIds.clear();
    rawSubRecords.clear();
}

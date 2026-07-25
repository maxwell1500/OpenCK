#include "Alchrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void AlchRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': flags = esm.readType<quint32>(); break;
            case 'ICON': iconPath = esm.readZString(); break;
            case 'MODL': modelPath = esm.readZString(); break;
            case 'DATA': {
                weight = esm.readType<float>();
                value = esm.readType<quint32>();
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

void AlchRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubZString('ICON', iconPath);
    esm.writeSubZString('MODL', modelPath);
    esm.startSubRecord('DATA');
    esm.writeType<float>(weight);
    esm.writeType<quint32>(value);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void AlchRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    iconPath = "";
    modelPath = "";
    weight = 0.0f;
    value = 0;
    rawSubRecords.clear();
}

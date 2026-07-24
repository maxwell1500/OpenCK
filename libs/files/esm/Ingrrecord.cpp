#include "Ingrrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void IngrRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'ITM2': iconPath = esm.readZString(); break;
            case 'ODIT': modelPath = esm.readZString(); break;
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

void IngrRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FLAG', flags);
    esm.writeSubZString('ITM2', iconPath);
    esm.writeSubZString('ODIT', modelPath);
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

void IngrRecord::blank()
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

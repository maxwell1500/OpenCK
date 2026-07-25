#include "Contrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void ContRecord::load(ESMReader& esm, bool)
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
            case 'DATA': flags = esm.readType<quint8>(); break;
            case 'COCT': inventoryControl = esm.readType<quint32>(); break;
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

void ContRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubZString('ICON', iconPath);
    esm.writeSubZString('MODL', modelPath);
    esm.writeSubData<quint8>('DATA', flags);
    esm.writeSubData<quint32>('COCT', inventoryControl);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void ContRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    iconPath = "";
    modelPath = "";
    contents = 0;
    inventoryControl = 0;
    weight = 0.0f;
    value = 0;
    rawSubRecords.clear();
}

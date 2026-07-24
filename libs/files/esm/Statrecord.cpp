#include "Statrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void StatRecord::load(ESMReader& esm, bool)
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
            case 'MNAM': lodModelPath = esm.readZString(); break;
            case 'RNAM': lodFlags = esm.readType<quint32>(); break;
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

void StatRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FLAG', flags);
    esm.writeSubZString('ITM2', iconPath);
    esm.writeSubZString('ODIT', modelPath);
    if (!lodModelPath.isEmpty()) {
        esm.writeSubZString('MNAM', lodModelPath);
    }
    if (lodFlags != 0) {
        esm.writeSubData<quint32>('RNAM', lodFlags);
    }

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void StatRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    iconPath = "";
    modelPath = "";
    lodModelPath = "";
    lodFlags = 0;
    rawSubRecords.clear();
}

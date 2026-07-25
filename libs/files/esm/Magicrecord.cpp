#include "Magicrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void MagicRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': flags = esm.readType<quint32>(); break;
            case 'MDOB': schools = esm.readType<quint32>(); break;
            case 'SNAM': castingSound = esm.readType<quint32>(); break;
            case 'ICON': iconPath = esm.readZString(); break;
            case 'MODL': modelPath = esm.readZString(); break;
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

void MagicRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubData<quint32>('MDOB', schools);
    esm.writeSubData<quint32>('SNAM', castingSound);
    esm.writeSubZString('ICON', iconPath);
    esm.writeSubZString('MODL', modelPath);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void MagicRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    schools = 0;
    damageType = 0;
    castingSound = 0;
    iconPath = "";
    modelPath = "";
    effects.clear();
    rawSubRecords.clear();
}

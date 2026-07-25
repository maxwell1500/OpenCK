#include "Armorrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void ArmorRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'FULL': fullName = esm.readZString(); break;
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'DNAM': armorRating = esm.readType<quint32>(); break;
            case 'ICON': iconPath = esm.readZString(); break;
            case 'MODL': modelPath = esm.readZString(); break;
            case 'DATA': {
                value = esm.readType<quint32>();
                health = esm.readType<quint16>();
                weight = esm.readType<float>();
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

void ArmorRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('FULL', fullName);
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubData<quint32>('DNAM', armorRating);
    esm.writeSubZString('ICON', iconPath);
    esm.writeSubZString('MODL', modelPath);
    esm.startSubRecord('DATA');
    esm.writeType<quint32>(value);
    esm.writeType<quint16>(static_cast<quint16>(health));
    esm.writeType<float>(weight);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void ArmorRecord::blank()
{
    editorId = "";
    fullName = "";
    formId = 0;
    flags = 0;
    armorRating = 0;
    weight = 0.0f;
    value = 0;
    iconPath = "";
    modelPath = "";
    health = 0.0f;
    rawSubRecords.clear();
}

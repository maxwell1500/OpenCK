#include "weaprecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void WeaponRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'FULL': fullName = esm.readZString(); break;
            case 'EDID': editorId = esm.readZString(); break;
            case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'DATA': {
                weaponType = esm.readType<quint32>();
                weight = esm.readType<float>();
                value = esm.readType<quint32>();
                break;
            }
            case 'DNAM': {
                damage = esm.readType<float>();
                speed = esm.readType<float>();
                reach = esm.readType<float>();
                break;
            }
            case 'ITM2': iconPath = esm.readZString(); break;
            case 'ODIT': modelPath = esm.readZString(); break;
            case 'EAMT': enchantment = esm.readType<quint32>(); break;
            case 'MDOB': magicSchool = esm.readType<quint32>(); break;
            case 'ENAM': enchantLimit = esm.readType<quint32>(); break;
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

void WeaponRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('FULL', fullName);
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FLAG', flags);
    esm.startSubRecord('DATA');
    esm.writeType<quint32>(weaponType);
    esm.writeType<float>(weight);
    esm.writeType<quint32>(value);
    esm.endSubRecord();
    esm.startSubRecord('DNAM');
    esm.writeType<float>(damage);
    esm.writeType<float>(speed);
    esm.writeType<float>(reach);
    esm.endSubRecord();
    esm.writeSubZString('ITM2', iconPath);
    esm.writeSubZString('ODIT', modelPath);
    esm.writeSubData<quint32>('EAMT', enchantment);
    esm.writeSubData<quint32>('MDOB', magicSchool);
    esm.writeSubData<quint32>('ENAM', enchantLimit);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void WeaponRecord::blank()
{
    editorId = "";
    fullName = "";
    formId = 0;
    flags = 0;
    weaponType = 0;
    damage = 0.0f;
    speed = 0.0f;
    reach = 0.0f;
    weight = 0.0f;
    value = 0;
    enchantment = 0;
    iconPath = "";
    modelPath = "";
    magicSchool = 0;
    enchantLimit = 0;
    rawSubRecords.clear();
}

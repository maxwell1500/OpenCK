#include "materialrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void MaterialRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'BKMN': materialName = esm.readZString(); break;
            case 'BNAM': bnam = esm.readZString(); break;
            case 'CNAM': cnam = esm.readZString(); break;
            case 'MNAM': texturePath = esm.readZString(); break;
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

void MaterialRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubZString('BKMN', materialName);
    esm.writeSubZString('BNAM', bnam);
    esm.writeSubZString('CNAM', cnam);
    esm.writeSubZString('MNAM', texturePath);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void MaterialRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    materialName = "";
    name = "";
    description = "";
    iconPath = "";
    modelPath = "";
    bnam = "";
    cnam = "";
    texturePath = "";
    materialType = 0;
    value = 0;
    weight = 0;
    health = 0;
    magicka = 0;
    stamina = 0;
    level = 0;
    race = 0;
    faction = 0;
    stage = 0;
    difficulty = 0;
    rawSubRecords.clear();
}

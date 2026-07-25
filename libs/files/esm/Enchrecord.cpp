#include "Enchrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void EnchRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'FULL': name = esm.readZString(); break;
            case 'ENIT':
            {
                type = esm.readType<quint32>();
                charges = esm.readType<quint32>();
                costLimit = esm.readType<quint32>();
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

void EnchRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubZString('FULL', name);
    esm.startSubRecord('ENIT');
    esm.writeType<quint32>(type);
    esm.writeType<quint32>(charges);
    esm.writeType<quint32>(costLimit);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void EnchRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    name = "";
    costLimit = 0;
    charges = 0;
    enchantmentData = 0;
    charge = 0.0f;
    duration = 0;
    magnitude = 0.0f;
    type = 0;
    soulGem = 0;
    rawSubRecords.clear();
}

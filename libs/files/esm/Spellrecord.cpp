#include "Spellrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void SpellRecord::load(ESMReader& esm, bool)
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
            case 'SPIT': cost = esm.readType<quint32>(); break;
            case 'SNAM': castingSound = esm.readType<quint32>(); break;
            case 'SPDT':
            {
                quint32 count = esm.readType<quint32>();
                effects.resize(count);
                for (int i = 0; i < count; i++)
                {
                    effects[i] = esm.readType<quint32>();
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

void SpellRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('FULL', fullName);
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubData<quint32>('SPIT', cost);
    esm.writeSubData<quint32>('SNAM', castingSound);
    esm.startSubRecord('SPDT');
    esm.writeType<quint32>(effects.size());
    for (auto effect : effects)
    {
        esm.writeType<quint32>(effect);
    }
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void SpellRecord::blank()
{
    editorId = "";
    fullName = "";
    formId = 0;
    flags = 0;
    cost = 0;
    castingSound = 0;
    effects.clear();
    enchantment = 0;
    rawSubRecords.clear();
}

#include "Inforecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void InfoRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'CNAM': responseText = esm.readZString(); break;
            case 'CTDA':
            {
                quint32 count = esm.readType<quint32>();
                conditionIds.resize(count);
                for (int i = 0; i < count; i++)
                    conditionIds[i] = esm.readType<quint32>();
                break;
            }
            case 'TLOI': targetId = esm.readType<quint32>(); break;
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

void InfoRecord::save(ESMWriter& esm) const
{
    esm.writeSubData<quint32>('FLAG', flags);
    esm.writeSubZString('CNAM', responseText);
    esm.startSubRecord('CTDA');
    esm.writeType<quint32>(conditionIds.size());
    for (quint32 id : conditionIds)
        esm.writeType<quint32>(id);
    esm.endSubRecord();
    esm.writeSubData<quint32>('TLOI', targetId);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

#include "Perkrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void PerkRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': flags = esm.readType<quint32>(); break;
            case 'DESC': description = esm.readZString(); break;
            case 'ICON': iconPath = esm.readZString(); break;
            case 'CTDA':
            {
                quint32 count = esm.readType<quint32>();
                conditions.resize(count);
                for (int i = 0; i < count; i++)
                {
                    conditions[i] = esm.readType<quint32>();
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

void PerkRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubZString('DESC', description);
    esm.writeSubZString('ICON', iconPath);
    esm.startSubRecord('CTDA');
    esm.writeType<quint32>(conditions.size());
    for (auto cond : conditions)
    {
        esm.writeType<quint32>(cond);
    }
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void PerkRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    description = "";
    requirements = "";
    iconPath = "";
    conditions.clear();
    rawSubRecords.clear();
}

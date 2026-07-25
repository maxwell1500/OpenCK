#include "Factrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void FactRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': flags = esm.readType<quint32>(); break;
            case 'FULL': factionName = esm.readZString(); break;
            case 'XNAM':
            {
                quint32 count = esm.readType<quint32>();
                relations.resize(count);
                for (int i = 0; i < count; i++)
                {
                    relations[i] = esm.readType<quint32>();
                }
                break;
            }
            case 'ICON': iconPath = esm.readZString(); break;
            case 'RNAM':
            {
                quint32 count = esm.readType<quint32>();
                ranks.resize(count);
                for (int i = 0; i < count; i++)
                {
                    ranks[i] = esm.readZString();
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

void FactRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubZString('FULL', factionName);
    esm.writeSubZString('ICON', iconPath);
    esm.startSubRecord('RNAM');
    esm.writeType<quint32>(ranks.size());
    for (auto rank : ranks)
    {
        esm.writeZString(rank);
    }
    esm.endSubRecord();
    esm.startSubRecord('XNAM');
    esm.writeType<quint32>(relations.size());
    for (auto rel : relations)
    {
        esm.writeType<quint32>(rel);
    }
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void FactRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    factionName = "";
    description = "";
    iconPath = "";
    ranks.clear();
    relations.clear();
    rawSubRecords.clear();
}

#include "ltexrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void LtexRecord::load(ESMReader& esm, bool)
{
    esm.readHeader();
    formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': flags = esm.readType<quint32>(); break;
            case 'ICON': iconPath = esm.readZString(); break;
            case 'HNAM': havokMaterial = esm.readType<quint32>(); break;
            case 'SNAM':
            {
                while (esm.isSubLeft())
                    grassFormIds.append(esm.readType<quint32>());
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

void LtexRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    if (flags != 0)
        esm.writeSubData<quint32>('FNAM', flags);
    if (!iconPath.isEmpty())
        esm.writeSubZString('ICON', iconPath);
    if (havokMaterial != 0)
        esm.writeSubData<quint32>('HNAM', havokMaterial);
    if (!grassFormIds.isEmpty())
    {
        esm.startSubRecord('SNAM');
        for (quint32 id : grassFormIds)
            esm.writeType<quint32>(id);
        esm.endSubRecord();
    }

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void LtexRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    iconPath.clear();
    havokMaterial = 0;
    grassFormIds.clear();
    rawSubRecords.clear();
}

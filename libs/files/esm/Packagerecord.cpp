#include "Packagerecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void PackageRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'PKDT': packageType = esm.readType<quint32>(); break;
            case 'PLDT': targetType = esm.readType<quint32>(); break;
            case 'PTDT':
            {
                quint32 count = esm.readType<quint32>();
                targetIds.resize(count);
                for (int i = 0; i < count; i++)
                    targetIds[i] = esm.readType<quint32>();
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

void PackageRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubData<quint32>('PKDT', packageType);
    esm.writeSubData<quint32>('PLDT', targetType);
    esm.startSubRecord('PTDT');
    esm.writeType<quint32>(targetIds.size());
    for (quint32 id : targetIds)
        esm.writeType<quint32>(id);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void PackageRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    packageType = 0;
    targetType = 0;
    targetIds.clear();
    parameters.clear();
    rawSubRecords.clear();
}

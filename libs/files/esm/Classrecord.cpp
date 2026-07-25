#include "Classrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void ClassRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'FULL': className = esm.readZString(); break;
            case 'DESC': description = esm.readZString(); break;
            case 'CNAM': serviceFlags = esm.readType<quint32>(); break;
            case 'ICON': iconPath = esm.readZString(); break;
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

void ClassRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubZString('FULL', className);
    esm.writeSubZString('DESC', description);
    esm.writeSubData<quint32>('CNAM', serviceFlags);
    esm.writeSubZString('ICON', iconPath);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void ClassRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    className = "";
    description = "";
    serviceFlags = 0;
    iconPath = "";
    rawSubRecords.clear();
}

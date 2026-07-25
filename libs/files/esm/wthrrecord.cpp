#include "wthrrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void WthrRecord::load(ESMReader& esm, bool)
{
    esm.readHeader();
    formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'SNAM': sunTexture = esm.readZString(); break;
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

void WthrRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    if (flags != 0)
        esm.writeSubData<quint32>('FNAM', flags);
    if (!sunTexture.isEmpty())
        esm.writeSubZString('SNAM', sunTexture);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void WthrRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    sunTexture.clear();
    rawSubRecords.clear();
}

#include "sounrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void SounRecord::load(ESMReader& esm, bool)
{
    esm.readHeader();
    formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': soundFile = esm.readZString(); break;
            case 'SNDD':
            case 'SNDX':
            {
                flags = esm.readType<quint32>();
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

void SounRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    if (!soundFile.isEmpty())
        esm.writeSubZString('FNAM', soundFile);
    if (flags != 0)
        esm.writeSubData<quint32>('SNDX', flags);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void SounRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    soundFile.clear();
    rawSubRecords.clear();
}

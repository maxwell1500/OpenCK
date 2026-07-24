#include "Bookrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"

void BookRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'DATA':
            {
                pages = esm.readZString();
                break;
            }
            case 'ITM2': iconPath = esm.readZString(); break;
            case 'ODIT': modelPath = esm.readZString(); break;
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

void BookRecord::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FLAG', flags);
    esm.writeSubZString('DATA', pages);
    esm.writeSubZString('ITM2', iconPath);
    esm.writeSubZString('ODIT', modelPath);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void BookRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    pageCount = 0;
    pages = "";
    iconPath = "";
    modelPath = "";
    rawSubRecords.clear();
}

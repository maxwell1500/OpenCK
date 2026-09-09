#include "lcrt.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void LocationRefType::load(ESMReader& esm, bool)
{
    esm.readHeader();
    formId = esm.currentFormId();
    // Subrecord order varies across games; walk generically instead of
    // demanding EDID-then-CNAM (readSubData throws on any other name).
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0)
            break;
        switch (sub)
        {
        case 'EDID':
            editorId = esm.readZString();
            break;
        case 'CNAM':
            if (esm.subLeft() >= static_cast<qint64>(sizeof(Color)))
                color = esm.readType<Color>();
            else if (esm.subLeft() > 0)
                esm.skip(static_cast<int>(esm.subLeft()));
            break;
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

void LocationRefType::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<Color>('CNAM', color);
    for (const auto& raw : rawSubRecords)
        esm.writeRawSubRecord(raw);
}

void LocationRefType::blank()
{
    formId = 0;
    editorId = "";
    color = 0;
}

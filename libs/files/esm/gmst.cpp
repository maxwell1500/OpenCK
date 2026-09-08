#include "gmst.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void GameSetting::load(ESMReader& esm, bool)
{
    esm.readHeader();
    formId = esm.currentFormId();
    editorId = esm.readSubZString('EDID');

    value.load(esm, Variant::Format_GMST, editorId);

    // Starfield TSMG records carry extra subrecords after the value (14
    // bytes observed). Drain them losslessly so the record ends exactly
    // consumed and following records stay aligned.
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0)
            break;
        RawSubRecord raw;
        raw.name = sub;
        esm.readRawSubData(raw.data);
        rawSubRecords.push_back(raw);
    }
}

void GameSetting::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);

    esm.startSubRecord('DATA');
    value.write(esm, Variant::Format_GMST);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
        esm.writeRawSubRecord(raw);
}

void GameSetting::blank()
{
    editorId = "";
    value.setType(VariantType::Var_None);
}

bool operator==(const GameSetting& l, const GameSetting& r)
{
    return l.value == r.value;
}

#include "gmst.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void GameSetting::load(ESMReader& esm, bool)
{
    esm.readHeader();
    formId = esm.currentFormId();
    editorId = esm.readSubZString('EDID');

    value.load(esm, Variant::Format_GMST, editorId);
}

void GameSetting::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);

    esm.startSubRecord('DATA');
    value.write(esm, Variant::Format_GMST);
    esm.endSubRecord();
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

#include "glob.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void GlobalVariable::load(ESMReader& esm, bool)
{
    constant = esm.readHeader().flags.test(GlobalVariable::Constant);

    // Skyrim GLOB is EDID+FNAM(type char)+FLTV(value). Starfield GLOBs carry
    // a different subrecord set, so walk generically: pick up the subrecords
    // we know and keep everything else verbatim instead of throwing on the
    // first unexpected name (which skipped 2600+ real records).
    bool haveType = false;
    unsigned char typeChar = 'f';
    bool haveValue = false;
    float floatValue = 0.0f;

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
        case 'FNAM':
        case 'FLAG':
            if (esm.subLeft() >= 1)
            {
                typeChar = esm.readType<quint8>();
                haveType = true;
            }
            break;
        case 'FLTV':
            if (esm.subLeft() >= 4)
            {
                floatValue = esm.readType<float>();
                haveValue = true;
            }
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

    if (!haveType || typeChar == 'f')
        value.setFloat(floatValue);
    else if (typeChar == 's')
        value.setShort(static_cast<quint16>(floatValue));
    else
        value.setInt(static_cast<quint32>(floatValue));
}

void GlobalVariable::save(ESMWriter& esm) const
{
    esm.writeSubZString('EDID', editorId);
    value.write(esm, Variant::Format_GLOB);

    for (const auto& raw : rawSubRecords)
        esm.writeRawSubRecord(raw);
}

void GlobalVariable::blank()
{
    editorId = "";
    value.setType(VariantType::Var_None);
    constant = false;
    rawSubRecords.clear();
}

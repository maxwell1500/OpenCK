#include "tesfullname.hpp"

#include "../files/esm/esmreader.hpp"
#include "../files/esm/esmwriter.hpp"
#include "../files/esm/records.hpp"

namespace tescomponents {

bool TESFullName_Component::canHandle(quint32 subrecordName) const
{
    return subrecordName == NAME('FULL');
}

void TESFullName_Component::handleSubrecord(quint32 subrecordName, ESMReader& esm)
{
    if (subrecordName == NAME('FULL'))
    {
        fullName = esm.readZString();
    }
}

void TESFullName_Component::save(ESMWriter& esm) const
{
    if (!fullName.isEmpty())
    {
        esm.writeSubZString(NAME('FULL'), fullName);
    }
    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

} // namespace tescomponents

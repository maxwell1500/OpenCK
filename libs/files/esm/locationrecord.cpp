#include "locationrecord.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tesfullname.hpp"

void LocationRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
}

void LocationRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        bool handled = false;
        for (auto& c : components.all())
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        switch (sub)
        {
        case 'EDID': editorId = esm.readZString(); break;
        case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); break;
        case 'PNAM': parentId = esm.readType<quint32>(); break;
        case 'DATA':
        {
            x = esm.readType<quint32>();
            y = esm.readType<quint32>();
            z = esm.readType<quint32>();
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
    auto* fn = static_cast<tescomponents::TESFullName_Component*>(
        components.findByName(QStringLiteral("TESFullName")));
    if (fn) locationName = fn->fullName;
}

void LocationRecord::save(ESMWriter& esm) const
{
    auto* fn = const_cast<LocationRecord*>(this)->components.findByName(QStringLiteral("TESFullName"));
    if (fn) static_cast<tescomponents::TESFullName_Component*>(fn)->fullName = locationName;

    esm.writeSubZString('EDID', editorId);
    components.saveAll(esm);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.writeSubData<quint32>('PNAM', parentId);
    esm.startSubRecord('DATA');
    esm.writeType<quint32>(x);
    esm.writeType<quint32>(y);
    esm.writeType<quint32>(z);
    esm.endSubRecord();

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
}

void LocationRecord::blank()
{
    editorId = "";
    formId = 0;
    flags = 0;
    locationName = "";
    parentId = 0;
    x = 0;
    y = 0;
    z = 0;
    rawSubRecords.clear();
    initComponents();
}

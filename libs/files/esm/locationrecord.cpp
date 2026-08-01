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
        case 'XNAM':
        {
            // Start a new linked-reference group keyed by a ref-type form ID.
            LinkedRef group;
            group.refTypeId = esm.readType<quint32>();
            linkedRefs.append(group);
            break;
        }
        case 'LNAM':
        {
            // Append the linked location form ID to the current group.
            if (!linkedRefs.isEmpty())
                linkedRefs.last().linkedIds.append(esm.readType<quint32>());
            else
            {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
            }
            break;
        }
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

    // Linked-reference groups: XNAM starts a group, LNAM appends links.
    for (const LinkedRef& group : linkedRefs)
    {
        if (group.refTypeId == 0)
            continue;
        esm.writeSubData<quint32>('XNAM', group.refTypeId);
        for (quint32 linkedId : group.linkedIds)
            esm.writeSubData<quint32>('LNAM', linkedId);
    }

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
    linkedRefs.clear();
    rawSubRecords.clear();
    initComponents();
}

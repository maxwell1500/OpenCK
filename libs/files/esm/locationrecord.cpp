#include "locationrecord.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tesfullname.hpp"

#include <QHash>

namespace
{
// Starfield TRCL records carry flag/id subrecords in several widths
// (observed 1..8 bytes). Read exactly the declared number of bytes,
// little-endian, so the stream never advances past the record end.
quint32 readLE(ESMReader& esm)
{
    qint64 n = esm.subLeft();
    if (n <= 0)
        return 0;
    if (n > 4)
        n = 4;
    quint32 v = 0;
    for (qint64 i = 0; i < n; ++i)
        v |= quint32(esm.readType<quint8>()) << (8 * i);
    return v;
}
}

void LocationRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
}

void LocationRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);
        bool handled = false;
        for (auto& c : components.all())
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        if (handled) continue;
        switch (sub)
        {
        case 'EDID': editorId = esm.readZString(); break;
        case 'FULL':
        {
            // FULL is consumed here (not left as raw) so the location name
            // mirror actually reflects the file and round-trips through it.
            // A repeated FULL is pathological; keep extras verbatim.
            if (!hasFull)
            {
                locationName = esm.readZString();
                hasFull = true;
                auto* fn = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
                if (fn) fn->fullName = locationName;
            }
            else
            {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
            }
            break;
        }
        case 'FNAM': case 'FLAG':
            flags = readLE(esm);
            hasFlags = true;
            flagsSpelling = sub;
            break;
        case 'PNAM': parentId = readLE(esm); hasParent = true; break;
        case 'XNAM':
        {
            // Start a new linked-reference group keyed by a ref-type form ID.
            LinkedRef group;
            group.refTypeId = readLE(esm);
            linkedRefs.append(group);
            break;
        }
        case 'LNAM':
        {
            // Append the linked location form ID to the current group.
            if (!linkedRefs.isEmpty())
                linkedRefs.last().linkedIds.append(readLE(esm));
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
            x = esm.subLeft() >= 4 ? esm.readType<quint32>() : 0;
            y = esm.subLeft() >= 4 ? esm.readType<quint32>() : 0;
            z = esm.subLeft() >= 4 ? esm.readType<quint32>() : 0;
            if (esm.subLeft() > 0)
                esm.skip(static_cast<int>(esm.subLeft()));
            hasData = true;
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

    auto writeLinkedRefs = [&]() {
        // Linked-reference groups: XNAM starts a group, LNAM appends links.
        for (const LinkedRef& group : linkedRefs)
        {
            if (group.refTypeId == 0)
                continue;
            esm.writeSubData<quint32>('XNAM', group.refTypeId);
            for (quint32 linkedId : group.linkedIds)
                esm.writeSubData<quint32>('LNAM', linkedId);
        }
    };
    auto writeData = [&]() {
        esm.startSubRecord('DATA');
        esm.writeType<quint32>(x);
        esm.writeType<quint32>(y);
        esm.writeType<quint32>(z);
        esm.endSubRecord();
    };

    QHash<NAME, QVector<int>> rawByName;
    for (int i = 0; i < rawSubRecords.size(); ++i)
        rawByName[rawSubRecords[i].name].append(i);
    QHash<NAME, int> rawCursor;

    bool wroteEdid = false, wroteFlags = false, wroteFull = false,
        wroteParent = false, wroteLinkedRefs = false, wroteData = false;

    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
        case 'EDID':
            if (!wroteEdid)
            {
                esm.writeSubZString('EDID', editorId);
                wroteEdid = true;
            }
            break;
        case 'FULL':
        {
            if (!wroteFull && (hasFull || !locationName.isEmpty()))
                esm.writeSubZString('FULL', locationName);
            else
            {
                const QVector<int>& idx = rawByName[sub];
                int& cur = rawCursor[sub];
                if (cur < idx.size())
                    esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
            }
            wroteFull = true;
            break;
        }
        case 'FNAM': case 'FLAG':
            if (!wroteFlags && (hasFlags || flags != 0))
            {
                esm.writeSubData<quint32>(flagsSpelling, flags);
                wroteFlags = true;
            }
            break;
        case 'PNAM':
            if (!wroteParent && (hasParent || parentId != 0))
                esm.writeSubData<quint32>('PNAM', parentId);
            wroteParent = true;
            break;
        case 'XNAM': case 'LNAM':
            if (!wroteLinkedRefs)
            {
                writeLinkedRefs();
                wroteLinkedRefs = true;
            }
            break;
        case 'DATA':
            if (!wroteData && (hasData || x != 0 || y != 0 || z != 0))
                writeData();
            wroteData = true;
            break;
        default:
        {
            const QVector<int>& idx = rawByName[sub];
            int& cur = rawCursor[sub];
            if (cur < idx.size())
                esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
            break;
        }
        }
    }

    if (!wroteEdid && !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);
    if (!wroteFull && !locationName.isEmpty())
        esm.writeSubZString('FULL', locationName);
    if (!wroteFlags && (hasFlags || flags != 0))
        esm.writeSubData<quint32>(flagsSpelling, flags);
    if (!wroteParent && parentId != 0)
        esm.writeSubData<quint32>('PNAM', parentId);
    if (!wroteLinkedRefs)
        writeLinkedRefs();
    if (!wroteData && (x != 0 || y != 0 || z != 0))
        writeData();

    for (auto it = rawByName.constBegin(); it != rawByName.constEnd(); ++it)
    {
        const QVector<int>& idx = it.value();
        int& cur = rawCursor[it.key()];
        while (cur < idx.size())
            esm.writeRawSubRecord(rawSubRecords[idx[cur++]]);
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
    loadOrder.clear();
    hasFlags = false;
    flagsSpelling = NAME('FNAM');
    hasFull = false;
    hasParent = false;
    hasData = false;
    initComponents();
}

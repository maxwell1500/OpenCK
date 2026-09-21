#include "Factrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "../../components/tier1_components.hpp"

#include <QSet>

void FactRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESTexture_Component>();
}

void FactRecord::load(ESMReader& esm, bool)
{
    esm.readHeader(); formId = esm.currentFormId();
    initComponents();
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        if (sub == 0) break;
        loadOrder.append(sub);
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        }
        if (handled) { loadIsRaw.append(0); continue; }
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); hasEdid = true; loadIsRaw.append(0); break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); hasFlags = true; loadIsRaw.append(0); break;
            case 'FULL':
            {
                esm.readRawSubData(fullRaw);
                const int nul = fullRaw.indexOf('\0');
                factionName = QString::fromUtf8(fullRaw.constData(),
                    nul >= 0 ? nul : fullRaw.size());
                hasFull = true;
                loadIsRaw.append(0);
                break;
            }
            case 'XNAM':
            {
                // Fixed-size relation struct (faction id, reaction mod,
                // flags); no parsed model yet — preserve raw.
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                loadIsRaw.append(1);
                break;
            }
            case 'RNAM':
            {
                // Rank data; a zstring in Morrowind, a fixed struct in
                // Skyrim/Starfield. Preserve raw for lossless round-trip.
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                loadIsRaw.append(1);
                break;
            }
            default:
            {
                RawSubRecord raw;
                raw.name = sub;
                esm.readRawSubData(raw.data);
                rawSubRecords.push_back(raw);
                loadIsRaw.append(1);
                break;
            }
        }
    }
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(components.findByName(QStringLiteral("TESTexture")));
    if (tex) iconPath = tex->iconPath;
}

void FactRecord::save(ESMWriter& esm) const
{
    auto* tex = static_cast<tescomponents::TESTexture_Component*>(const_cast<FactRecord*>(this)->components.findByName(QStringLiteral("TESTexture")));
    if (tex) tex->iconPath = iconPath;

    const auto writeFull = [&] {
        if (!fullRaw.isEmpty() && factionName == QString::fromUtf8(fullRaw.constData(), fullRaw.indexOf('\0') >= 0 ? fullRaw.indexOf('\0') : fullRaw.size()))
            esm.writeRawSubRecord(RawSubRecord{ NAME('FULL'), fullRaw });
        else
            esm.writeSubZString('FULL', factionName);
    };

    if (loadOrder.isEmpty())
    {
        if (hasEdid || !editorId.isEmpty())
            esm.writeSubZString('EDID', editorId);
        if (hasFlags || flags != 0)
            esm.writeSubData<quint32>('FNAM', flags);
        if (hasFull || !factionName.isEmpty())
            writeFull();
        components.saveAll(esm);
        for (const auto& raw : rawSubRecords)
            esm.writeRawSubRecord(raw);
        return;
    }

    QSet<NAME> seen;
    int rawCur = 0;
    for (int p = 0; p < loadOrder.size(); ++p)
    {
        const NAME sub = loadOrder[p];
        seen.insert(sub);
        if (p >= loadIsRaw.size() || loadIsRaw[p] != 0)
        {
            if (rawCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            continue;
        }
        switch (sub)
        {
        case 'EDID': esm.writeSubZString('EDID', editorId); break;
        case 'FNAM': case 'FLAG': esm.writeSubData<quint32>(sub, flags); break;
        case 'FULL': if (hasFull) writeFull(); break;
        default:
            if (!components.writeSubrecord(sub, esm) && rawCur < rawSubRecords.size())
                esm.writeRawSubRecord(rawSubRecords[rawCur++]);
            break;
        }
    }

    if (!seen.contains(NAME('FNAM')) && !seen.contains(NAME('FLAG')) && (hasFlags || flags != 0))
        esm.writeSubData<quint32>('FNAM', flags);
    if (!seen.contains(NAME('FULL')) && (hasFull || !factionName.isEmpty()))
        writeFull();
    if (!seen.contains(NAME('ICON')) && !seen.contains(NAME('ICO2')))
    {
        const Component* t = components.findByName(QStringLiteral("TESTexture"));
        if (t) { t->writeSubrecord(NAME('ICON'), esm); t->writeSubrecord(NAME('ICO2'), esm); }
    }
    while (rawCur < rawSubRecords.size())
        esm.writeRawSubRecord(rawSubRecords[rawCur++]);
}

void FactRecord::blank()
{
    editorId.clear();
    formId = 0;
    flags = 0;
    factionName.clear();
    fullRaw.clear();
    loadOrder.clear();
    loadIsRaw.clear();
    hasEdid = false;
    description.clear();
    iconPath.clear();
    ranks.clear();
    relations.clear();
    rawSubRecords.clear();
    hasFlags = false;
    hasFull = false;
    verbatimBody.clear();
    verbatimFlags = 0;
    verbatimSnapshot.reset();
    initComponents();
}

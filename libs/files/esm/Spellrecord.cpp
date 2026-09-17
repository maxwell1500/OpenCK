#include "Spellrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "subrecordreplay.hpp"
#include "../../components/tier1_components.hpp"
#include "../../components/tesfullname.hpp"

void SpellRecord::initComponents()
{
    components.clear();
    components.add<tescomponents::TESFullName_Component>();
}

void SpellRecord::load(ESMReader& esm, bool)
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
        {
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        }
        if (handled) continue;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG':
                flags = esm.readSubU32(&flagsWidth);
                hasFlags = true;
                flagsSpelling = sub;
                break;
            case 'SPIT':
            {
                hasSpit = true;
                if (esm.subLeft() >= 4) spellType = esm.readType<quint32>();
                if (esm.subLeft() >= 4) cost = esm.readType<quint32>();
                if (esm.subLeft() >= 4) flags = esm.readType<quint32>();
                if (esm.subLeft() > 0)
                    esm.readRawSubData(spitExtra);
                break;
            }
            case 'SNAM':
                castingSound = esm.readSubU32();
                hasSnam = true;
                break;
            case 'SPDT':
            {
                // Morrowind-style spell data: type (s32), cost (s32),
                // flags (s32). Effects live in their own subrecords.
                // Preserve the original bytes for lossless round-trip.
                const qint32 spellType = esm.readType<qint32>();
                const qint32 spellCost = esm.readType<qint32>();
                const qint32 spellFlags = esm.readType<qint32>();
                cost = static_cast<quint32>(spellCost);
                flags = static_cast<quint32>(spellFlags);
                RawSubRecord raw;
                raw.name = sub;
                QByteArray bytes;
                QDataStream ds(&bytes, QIODevice::WriteOnly);
                ds.setByteOrder(QDataStream::LittleEndian);
                ds << spellType << spellCost << spellFlags;
                raw.data = bytes;
                rawSubRecords.push_back(raw);
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
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) fullName = nameComp->fullName;
}

void SpellRecord::save(ESMWriter& esm) const
{
    auto* nameComp = static_cast<tescomponents::TESFullName_Component*>(const_cast<SpellRecord*>(this)->components.findByName(QStringLiteral("TESFullName")));
    if (nameComp) nameComp->fullName = fullName;

    SubrecordReplay replay;
    replay.init(rawSubRecords);

    const auto writeFlags = [&]()
    {
        esm.startSubRecord(flagsSpelling);
        const quint8 w = flagsWidth == 0 ? 1 : flagsWidth;
        for (quint8 i = 0; i < w; ++i)
            esm.writeType<quint8>(static_cast<quint8>((flags >> (8 * i)) & 0xFF));
        esm.endSubRecord();
    };
    const auto writeSpit = [&]()
    {
        esm.startSubRecord('SPIT');
        esm.writeType<quint32>(spellType);
        esm.writeType<quint32>(cost);
        esm.writeType<quint32>(flags);
        if (!spitExtra.isEmpty())
            esm.writeRawData(spitExtra.constData(), spitExtra.size());
        esm.endSubRecord();
    };

    bool wroteEdid = false, wroteFlags = false, wroteSpit = false, wroteSnam = false;
    for (NAME sub : loadOrder)
    {
        switch (sub)
        {
            case 'EDID':
                if (!wroteEdid) { esm.writeSubZString('EDID', editorId); wroteEdid = true; }
                break;
            case 'FNAM': case 'FLAG':
                if (!wroteFlags && hasFlags) { writeFlags(); wroteFlags = true; }
                break;
            case 'SPIT':
                if (!wroteSpit && (hasSpit || spellType != 0 || cost != 0 || flags != 0))
                {
                    writeSpit();
                    wroteSpit = true;
                }
                break;
            case 'SNAM':
                if (!wroteSnam && (hasSnam || castingSound != 0))
                {
                    esm.writeSubData<quint32>('SNAM', castingSound);
                    wroteSnam = true;
                }
                break;
            default:
                if (!components.writeSubrecord(sub, esm))
                    replay.write(sub, esm);
                break;
        }
    }

    if (!wroteEdid && !editorId.isEmpty())
        esm.writeSubZString('EDID', editorId);
    if (!wroteFlags && hasFlags)
        writeFlags();
    if (!wroteSpit && (hasSpit || spellType != 0 || cost != 0 || flags != 0))
        writeSpit();
    if (!wroteSnam && (hasSnam || castingSound != 0))
        esm.writeSubData<quint32>('SNAM', castingSound);

    replay.writeLeftover(esm);
}

void SpellRecord::blank()
{
    editorId.clear();
    fullName.clear();
    formId = 0;
    flags = 0;
    cost = 0;
    spellType = 0;
    castingSound = 0;
    effects.clear();
    enchantment = 0;
    rawSubRecords.clear();
    loadOrder.clear();
    hasFlags = false;
    flagsSpelling = NAME('FNAM');
    flagsWidth = 4;
    hasSpit = false;
    spitExtra.clear();
    hasSnam = false;
    initComponents();
}

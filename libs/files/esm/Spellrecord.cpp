#include "Spellrecord.hpp"
#include "esmreader.hpp"
#include "esmwriter.hpp"
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
    while (esm.isRecLeft())
    {
        NAME sub = esm.readNSubHeader();
        bool handled = false;
        for (auto& c : components.all())
        {
            if (c->canHandle(sub)) { c->handleSubrecord(sub, esm); handled = true; break; }
        }
        if (handled) continue;
        switch (sub)
        {
            case 'EDID': editorId = esm.readZString(); break;
            case 'FNAM': case 'FLAG': flags = esm.readType<quint32>(); break;
            case 'SPIT':
            {
                spellType = esm.readType<quint32>();
                cost = esm.readType<quint32>();
                flags = esm.readType<quint32>();
                if (esm.subLeft() > 0)
                    esm.skip(static_cast<int>(esm.subLeft()));
                break;
            }
            case 'SNAM': castingSound = esm.readType<quint32>(); break;
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

    components.saveAll(esm);
    esm.writeSubZString('EDID', editorId);
    esm.writeSubData<quint32>('FNAM', flags);
    esm.startSubRecord('SPIT');
    esm.writeType<quint32>(spellType);
    esm.writeType<quint32>(cost);
    esm.writeType<quint32>(flags);
    static const char spitPad[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    esm.writeRawData(spitPad, sizeof(spitPad));
    esm.endSubRecord();
    esm.writeSubData<quint32>('SNAM', castingSound);

    for (const auto& raw : rawSubRecords)
    {
        esm.startSubRecord(raw.name);
        esm.writeRawData(raw.data.data(), raw.data.size());
        esm.endSubRecord();
    }
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
    initComponents();
}

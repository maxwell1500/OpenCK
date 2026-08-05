#include "formidcompactor.hpp"

#include "../world/data.hpp"
#include "../world/irecordcollection.hpp"
#include "../world/collection.hpp"
#include "../../../libs/files/esm/relarecord.hpp"
#include "../../../libs/files/esm/ecznrecord.hpp"
#include "../../../libs/files/esm/ipdsrecord.hpp"
#include "../../../libs/files/esm/ipctrecord.hpp"
#include "../../../libs/files/esm/hazdrecord.hpp"
#include "../../../libs/files/esm/shourecord.hpp"
#include "../../../libs/components/tier2_components.hpp"
#include "../../../libs/files/log/logger.hpp"

#include <algorithm>
#include <cstring>
#include <type_traits>
#include <utility>

namespace {

constexpr quint32 ESL_MASK = 0x00000FFF;  // 4096 local IDs (0x000-0xFFF)

// Rewrite the quint32 FormID stored at byte `offset` of a raw subrecord
// payload through the old->new map. Truncated payloads are left alone.
void rewriteRawFormId(QByteArray& data, int offset, const QHash<quint32, quint32>& map)
{
    if (offset < 0 || offset + 4 > data.size()) return;
    quint32 id;
    std::memcpy(&id, data.constData() + offset, 4);
    const quint32 mapped = map.value(id, id);
    if (mapped != id)
        std::memcpy(data.data() + offset, &mapped, 4);
}

// KWDA carries a plain array of keyword FormIDs, one per 4 bytes.
void rewriteKwdaArray(RawSubRecord& raw, const QHash<quint32, quint32>& map)
{
    for (int off = 0; off + 4 <= raw.data.size(); off += 4)
        rewriteRawFormId(raw.data, off, map);
}

// Rewrite a record's typed FormID-reference members through the old->new map.
// Each concrete record type that exposes FormID members as typed fields is
// handled here.
template <typename Rec>
void rewriteReferences(Rec&, const QHash<quint32, quint32>&) {}

template <>
void rewriteReferences<RelaRecord>(RelaRecord& rec, const QHash<quint32, quint32>& map)
{
    rec.parentFormId = map.value(rec.parentFormId, rec.parentFormId);
    rec.childFormId = map.value(rec.childFormId, rec.childFormId);
}

template <>
void rewriteReferences<EcznRecord>(EcznRecord& rec, const QHash<quint32, quint32>& map)
{
    rec.zoneFormId = map.value(rec.zoneFormId, rec.zoneFormId);
    rec.locationFormId = map.value(rec.locationFormId, rec.locationFormId);
    rec.unusedFormId = map.value(rec.unusedFormId, rec.unusedFormId);
}

template <>
void rewriteReferences<IpdsRecord>(IpdsRecord& rec, const QHash<quint32, quint32>& map)
{
    for (quint32& id : rec.impactFormIds)
        id = map.value(id, id);
}

template <>
void rewriteReferences<IpctRecord>(IpctRecord& rec, const QHash<quint32, quint32>& map)
{
    rec.effectFormId = map.value(rec.effectFormId, rec.effectFormId);
}

template <>
void rewriteReferences<HazdRecord>(HazdRecord& rec, const QHash<quint32, quint32>& map)
{
    rec.imageSpace = map.value(rec.imageSpace, rec.imageSpace);
}

template <>
void rewriteReferences<ShouRecord>(ShouRecord& rec, const QHash<quint32, quint32>& map)
{
    for (ShoutWord& w : rec.words)
    {
        w.wordFormId = map.value(w.wordFormId, w.wordFormId);
        w.spellFormId = map.value(w.spellFormId, w.spellFormId);
    }
}

// Rewrite FormIDs inside a record's opaque rawSubRecords. Only the known
// (record type, subrecord name) payload layouts are interpreted; anything
// else is preserved byte-for-byte because its FormID offsets are unknown.
template <typename Rec>
void rewriteRawSubRecords(Rec&, const QHash<quint32, quint32>&) {}

template <>
void rewriteRawSubRecords<CellRecord>(CellRecord& rec, const QHash<quint32, quint32>& map)
{
    for (RawSubRecord& raw : rec.rawSubRecords)
    {
        switch (raw.name)
        {
        case NAME('XEZN'):  // encounter zone
        case NAME('XLCN'):  // location
        case NAME('LTMP'):  // lighting template
        case NAME('XWEM'):  // water environment
            rewriteRawFormId(raw.data, 0, map);
            break;
        case NAME('XCLR'):  // region list: one FormID per 4 bytes
            for (int off = 0; off + 4 <= raw.data.size(); off += 4)
                rewriteRawFormId(raw.data, off, map);
            break;
        default:
            break;
        }
    }
}

template <>
void rewriteRawSubRecords<RefrRecord>(RefrRecord& rec, const QHash<quint32, quint32>& map)
{
    for (RawSubRecord& raw : rec.rawSubRecords)
    {
        switch (raw.name)
        {
        case NAME('XNAM'):  // enable parent
        case NAME('XEMI'):  // ambient emission
        case NAME('XLTW'):  // location weight override
        case NAME('XMBO'):  // music override
        case NAME('XCNT'):  // instance count (extra-data container)
        case NAME('XPRD'):  // idle timer
            rewriteRawFormId(raw.data, 0, map);
            break;
        case NAME('XTEL'):  // teleport: destination cell at 0, destination ref at 4 when present
            rewriteRawFormId(raw.data, 0, map);
            rewriteRawFormId(raw.data, 4, map);
            break;
        case NAME('XLKR'):  // linked ref list: u32 type at 0, FormIDs from 4
            for (int off = 4; off + 4 <= raw.data.size(); off += 4)
                rewriteRawFormId(raw.data, off, map);
            break;
        default:
            break;
        }
    }
}

template <>
void rewriteRawSubRecords<NpcRecord>(NpcRecord& rec, const QHash<quint32, quint32>& map)
{
    for (RawSubRecord& raw : rec.rawSubRecords)
    {
        switch (raw.name)
        {
        case NAME('VOIC'):  // voice type
            rewriteRawFormId(raw.data, 0, map);
            break;
        case NAME('LVLD'):  // template FormID at 12 (flags/min/max precede it)
            rewriteRawFormId(raw.data, 12, map);
            break;
        case NAME('HDPT'):  // head parts: u32 count at 0, then that many FormIDs
            if (raw.data.size() >= 4)
            {
                quint32 count = 0;
                std::memcpy(&count, raw.data.constData(), 4);
                for (quint32 i = 0; i < count; ++i)
                    rewriteRawFormId(raw.data, 4 + static_cast<int>(i) * 4, map);
            }
            break;
        case NAME('KWDA'):
            rewriteKwdaArray(raw, map);
            break;
        default:
            break;
        }
    }
}

template <>
void rewriteRawSubRecords<DialRecord>(DialRecord& rec, const QHash<quint32, quint32>& map)
{
    for (RawSubRecord& raw : rec.rawSubRecords)
        if (raw.name == NAME('QNAM'))  // quest FormID
            rewriteRawFormId(raw.data, 0, map);
}

template <>
void rewriteRawSubRecords<QuestRecord>(QuestRecord& rec, const QHash<quint32, quint32>& map)
{
    for (RawSubRecord& raw : rec.rawSubRecords)
        if (raw.name == NAME('QNAM'))  // quest FormID
            rewriteRawFormId(raw.data, 0, map);
}

template <>
void rewriteRawSubRecords<AlchRecord>(AlchRecord& rec, const QHash<quint32, quint32>& map)
{
    for (RawSubRecord& raw : rec.rawSubRecords)
        if (raw.name == NAME('EFID'))  // magic effect
            rewriteRawFormId(raw.data, 0, map);
}

template <>
void rewriteRawSubRecords<IngrRecord>(IngrRecord& rec, const QHash<quint32, quint32>& map)
{
    for (RawSubRecord& raw : rec.rawSubRecords)
        if (raw.name == NAME('EFID'))  // magic effect
            rewriteRawFormId(raw.data, 0, map);
}

template <>
void rewriteRawSubRecords<EnchRecord>(EnchRecord& rec, const QHash<quint32, quint32>& map)
{
    for (RawSubRecord& raw : rec.rawSubRecords)
        if (raw.name == NAME('EFID'))  // magic effect
            rewriteRawFormId(raw.data, 0, map);
}

template <>
void rewriteRawSubRecords<SpellRecord>(SpellRecord& rec, const QHash<quint32, quint32>& map)
{
    for (RawSubRecord& raw : rec.rawSubRecords)
    {
        if (raw.name == NAME('EFID'))
            rewriteRawFormId(raw.data, 0, map);
        else if (raw.name == NAME('KWDA'))
            rewriteKwdaArray(raw, map);
    }
}

template <>
void rewriteRawSubRecords<MagicRecord>(MagicRecord& rec, const QHash<quint32, quint32>& map)
{
    for (RawSubRecord& raw : rec.rawSubRecords)
    {
        if (raw.name == NAME('EFID'))
            rewriteRawFormId(raw.data, 0, map);
        else if (raw.name == NAME('KWDA'))
            rewriteKwdaArray(raw, map);
    }
}

#define OPENCK_KWDA_RAW_REWRITE(RecType) \
    template <> \
    void rewriteRawSubRecords<RecType>(RecType& rec, const QHash<quint32, quint32>& map) \
    { \
        for (RawSubRecord& raw : rec.rawSubRecords) \
            if (raw.name == NAME('KWDA')) \
                rewriteKwdaArray(raw, map); \
    }

OPENCK_KWDA_RAW_REWRITE(ArmorRecord)
OPENCK_KWDA_RAW_REWRITE(WeaponRecord)
OPENCK_KWDA_RAW_REWRITE(ActiRecord)
OPENCK_KWDA_RAW_REWRITE(DoorRecord)
OPENCK_KWDA_RAW_REWRITE(FlorRecord)
OPENCK_KWDA_RAW_REWRITE(LocationRecord)
OPENCK_KWDA_RAW_REWRITE(CreatureRecord)

#undef OPENCK_KWDA_RAW_REWRITE

// Rewrite FormIDs held by the component classes that store simple quint32
// FormID members. Other component classes keep opaque data (see the
// class-level scope note in formidcompactor.hpp).
void rewriteComponentFormIds(openck::FormComponents& components, const QHash<quint32, quint32>& map)
{
    if (auto* ench = static_cast<tescomponents::TESEnchantableForm_Component*>(
            components.findByName(QStringLiteral("TESEnchantableForm"))))
    {
        ench->enchantmentFormId = map.value(ench->enchantmentFormId, ench->enchantmentFormId);
    }
    if (auto* snd = static_cast<tescomponents::BGSPickupPutdownSounds_Component*>(
            components.findByName(QStringLiteral("BGSPickupPutdownSounds"))))
    {
        snd->pickupSound = map.value(snd->pickupSound, snd->pickupSound);
        snd->putdownSound = map.value(snd->putdownSound, snd->putdownSound);
    }
}

template <typename T, typename = void>
struct HasComponents : std::false_type {};

template <typename T>
struct HasComponents<T, std::void_t<decltype(std::declval<T>().components)>> : std::true_type {};

// Visit every record in the concrete Collection<T> and rewrite typed fields,
// raw-subrecord payloads, and component-held FormIDs.
template <typename Rec>
void rewriteTyped(IRecordCollection* col, const QHash<quint32, quint32>& map, int& rewritten)
{
    auto* typed = dynamic_cast<Collection<Rec>*>(col);
    if (!typed) return;
    for (int i = 0; i < typed->count(); ++i)
    {
        Rec rec = static_cast<Rec>(typed->getRecord(i).get());
        const Rec before = rec;
        rewriteReferences(rec, map);
        rewriteRawSubRecords(rec, map);
        if constexpr (HasComponents<Rec>::value)
            rewriteComponentFormIds(rec.components, map);
        if (!(rec == before))
        {
            typed->getRecord(i).get() = rec;
            ++rewritten;
        }
    }
}

} // namespace

int FormIdCompactor::compact()
{
    QVector<quint32> ownedFormIds;
    QVector<QPair<IRecordCollection*, int>> owned;

    for (const auto& tc : mData.allCollectionsWithTypes())
    {
        IRecordCollection* col = tc.collection;
        for (int i = 0; i < col->count(); ++i)
        {
            if (col->isRecordModified(i))
            {
                ownedFormIds.append(col->getFormId(i));
                owned.append({ col, i });
            }
        }
    }

    mOwned = ownedFormIds.size();
    if (mOwned > static_cast<int>(ESL_MASK + 1))
    {
        LOG_ERROR(QString("FormIdCompactor: plugin owns %1 records, exceeding the ESL ceiling of %2")
            .arg(mOwned).arg(ESL_MASK + 1));
        return -1;
    }

    // Deterministic compaction: sort by original FormID, assign local IDs
    // 0..N-1, preserving the high 16 bits (plugin / master index).
    QVector<quint32> sorted = ownedFormIds;
    std::sort(sorted.begin(), sorted.end());

    QHash<quint32, quint32> map;
    for (int i = 0; i < sorted.size(); ++i)
    {
        const quint32 oldId = sorted.at(i);
        const quint32 high = oldId & 0xFFFF0000u;
        map.insert(oldId, high | static_cast<quint32>(i));
    }

    mRemapped = 0;
    mRewritten = 0;
    for (const auto& entry : owned)
    {
        IRecordCollection* col = entry.first;
        const int idx = entry.second;
        const quint32 oldId = col->getFormId(idx);
        auto it = map.constFind(oldId);
        if (it != map.constEnd() && it.value() != oldId)
        {
            col->setFormId(idx, it.value());
            ++mRemapped;
        }
    }

    // Rewrite typed reference fields, raw-subrecord payloads, and component
    // FormIDs for the record types that carry them.
    for (const auto& tc : mData.allCollectionsWithTypes())
    {
        IRecordCollection* col = tc.collection;
        rewriteTyped<RelaRecord>(col, map, mRewritten);
        rewriteTyped<EcznRecord>(col, map, mRewritten);
        rewriteTyped<IpdsRecord>(col, map, mRewritten);
        rewriteTyped<IpctRecord>(col, map, mRewritten);
        rewriteTyped<HazdRecord>(col, map, mRewritten);
        rewriteTyped<ShouRecord>(col, map, mRewritten);
        rewriteTyped<CellRecord>(col, map, mRewritten);
        rewriteTyped<RefrRecord>(col, map, mRewritten);
        rewriteTyped<NpcRecord>(col, map, mRewritten);
        rewriteTyped<DialRecord>(col, map, mRewritten);
        rewriteTyped<QuestRecord>(col, map, mRewritten);
        rewriteTyped<AlchRecord>(col, map, mRewritten);
        rewriteTyped<IngrRecord>(col, map, mRewritten);
        rewriteTyped<EnchRecord>(col, map, mRewritten);
        rewriteTyped<SpellRecord>(col, map, mRewritten);
        rewriteTyped<MagicRecord>(col, map, mRewritten);
        rewriteTyped<ArmorRecord>(col, map, mRewritten);
        rewriteTyped<WeaponRecord>(col, map, mRewritten);
        rewriteTyped<ActiRecord>(col, map, mRewritten);
        rewriteTyped<DoorRecord>(col, map, mRewritten);
        rewriteTyped<FlorRecord>(col, map, mRewritten);
        rewriteTyped<LocationRecord>(col, map, mRewritten);
        rewriteTyped<CreatureRecord>(col, map, mRewritten);
    }

    LOG_INFO(QString("FormIdCompactor: remapped %1 of %2 owned records into the ESL range")
        .arg(mRemapped).arg(mOwned));
    return mRemapped;
}

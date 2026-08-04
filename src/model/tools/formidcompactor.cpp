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
#include "../../../libs/files/log/logger.hpp"

#include <algorithm>

namespace {

constexpr quint32 ESL_MASK = 0x00000FFF;  // 4096 local IDs (0x000-0xFFF)

// Rewrite a record's typed FormID-reference members through the old->new map.
// Each concrete record type that exposes FormID members as typed fields is
// handled here; records whose references live in opaque rawSubRecords are left
// untouched (documented scope limitation).
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

// Visit every record in the concrete Collection<T> and rewrite typed fields.
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

    // Rewrite typed reference fields in the record types that expose them.
    for (const auto& tc : mData.allCollectionsWithTypes())
    {
        IRecordCollection* col = tc.collection;
        rewriteTyped<RelaRecord>(col, map, mRewritten);
        rewriteTyped<EcznRecord>(col, map, mRewritten);
        rewriteTyped<IpdsRecord>(col, map, mRewritten);
        rewriteTyped<IpctRecord>(col, map, mRewritten);
        rewriteTyped<HazdRecord>(col, map, mRewritten);
        rewriteTyped<ShouRecord>(col, map, mRewritten);
    }

    LOG_INFO(QString("FormIdCompactor: remapped %1 of %2 owned records into the ESL range")
        .arg(mRemapped).arg(mOwned));
    return mRemapped;
}

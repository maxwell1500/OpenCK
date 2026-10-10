#include "recordmerger.hpp"

#include <QHash>
#include <QVector>
#include <algorithm>
#include <cstdint>

namespace openck {

namespace {

using Key = quint64;   // type << 32 | formId

Key keyOf(const RecordSnapshot& r)
{
    return (static_cast<quint64>(r.type) << 32) | r.formId;
}

QHash<Key, RecordSnapshot> indexByKey(const QVector<RecordSnapshot>& records)
{
    QHash<Key, RecordSnapshot> index;
    for (const RecordSnapshot& r : records)
        index.insert(keyOf(r), r);
    return index;
}

QString recordLabel(const RecordSnapshot& r)
{
    return QStringLiteral("%1 0x%2")
        .arg(snapshotName(r.type),
             QString::number(r.formId, 16).rightJustified(8, QChar('0')));
}

} // namespace

RecordMergePlan RecordMerger::merge(const QString& basePath, const QString& minePath,
                                    const QString& theirsPath)
{
    RecordMergePlan plan;

    const QVector<RecordSnapshot> base = collectRecordSnapshots(basePath);
    const QVector<RecordSnapshot> mine = collectRecordSnapshots(minePath);
    const QVector<RecordSnapshot> theirs = collectRecordSnapshots(theirsPath);

    const QHash<Key, RecordSnapshot> baseIndex = indexByKey(base);
    const QHash<Key, RecordSnapshot> mineIndex = indexByKey(mine);
    const QHash<Key, RecordSnapshot> theirsIndex = indexByKey(theirs);

    QVector<Key> keys;
    for (auto it = mineIndex.constBegin(); it != mineIndex.constEnd(); ++it)
        keys.append(it.key());
    for (auto it = theirsIndex.constBegin(); it != theirsIndex.constEnd(); ++it)
        keys.append(it.key());
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    for (Key key : keys)
    {
        const bool hasBase = baseIndex.contains(key);
        const bool hasMine = mineIndex.contains(key);
        const bool hasTheirs = theirsIndex.contains(key);

        MergeAction action;
        action.kind = MergeAction::Take;

        if (hasMine && hasTheirs)
        {
            const RecordSnapshot& m = mineIndex[key];
            const RecordSnapshot& t = theirsIndex[key];
            const bool mineChanged = !hasBase || !(m == baseIndex[key]);
            const bool theirsChanged = !hasBase || !(t == baseIndex[key]);

            if (!mineChanged && !theirsChanged)
            {
                // Both sides identical to base (or both added the same record).
                action.record = m;
            }
            else if (mineChanged && !theirsChanged)
            {
                action.record = m;
            }
            else if (!mineChanged && theirsChanged)
            {
                action.record = t;
            }
            else if (m == t)
            {
                // Same change made twice: not a conflict.
                action.record = m;
            }
            else
            {
                action.kind = MergeAction::Conflict;
                action.record = m;   // keep the working side; the caller resolves
                action.reason = hasBase
                    ? QStringLiteral("both branches changed %1").arg(recordLabel(m))
                    : QStringLiteral("both branches added different records at %1")
                          .arg(recordLabel(m));
                ++plan.conflicts;
                if (!hasBase)
                    ++plan.idConflicts;   // same FormID claimed by two branches
            }
        }
        else if (hasMine)
        {
            const RecordSnapshot& m = mineIndex[key];
            const bool mineChanged = !hasBase || !(m == baseIndex[key]);

            if (!hasBase)
            {
                action.record = m;   // added by mine only
            }
            else if (mineChanged)
            {
                action.kind = MergeAction::Conflict;
                action.record = m;
                action.reason = QStringLiteral("theirs deleted %1 while mine changed it")
                                    .arg(recordLabel(m));
                ++plan.conflicts;
            }
            else
            {
                action.kind = MergeAction::Skip;
                action.reason = QStringLiteral("deleted by theirs");
            }
        }
        else   // theirs only
        {
            const RecordSnapshot& t = theirsIndex[key];
            const bool theirsChanged = !hasBase || !(t == baseIndex[key]);

            if (!hasBase)
            {
                action.record = t;   // added by theirs only
            }
            else if (theirsChanged)
            {
                action.kind = MergeAction::Conflict;
                action.record = t;
                action.reason = QStringLiteral("mine deleted %1 while theirs changed it")
                                    .arg(recordLabel(t));
                ++plan.conflicts;
            }
            else
            {
                action.kind = MergeAction::Skip;
                action.reason = QStringLiteral("deleted by mine");
            }
        }

        if (action.kind != MergeAction::Skip)
            ++plan.records;
        plan.actions.append(action);
    }

    return plan;
}

} // namespace openck

#include "plugindiff.hpp"

#include <QByteArray>
#include <QFileInfo>
#include <QHash>
#include <QVector>
#include <algorithm>
#include <cstdint>

namespace openck {

namespace {

QString readEdid(const QVector<SubSnapshot>& subs)
{
    for (const SubSnapshot& s : subs)
    {
        if (s.name == NAME('EDID'))
        {
            // EDID is a NUL-terminated zString; the terminator is part of
            // the payload, not the ID.
            const int nul = s.payload.indexOf('\0');
            return QString::fromLatin1(
                nul < 0 ? s.payload : s.payload.left(nul));
        }
    }
    return QString();
}

// LCS over subrecord names yields the edit script; each maximal run of
// matched steps that share one name is paired positionally inside the run so
// that an edited payload mid-block reports as Modified instead of a
// delete+add cascade, and surplus same-name subs on either side report as
// Added/Removed.
QVector<SubDiff> alignSubs(const QVector<SubSnapshot>& left,
                           const QVector<SubSnapshot>& right)
{
    const int n = static_cast<int>(left.size());
    const int m = static_cast<int>(right.size());
    if (n == 0 && m == 0)
        return {};

    // dp[i][j] = LCS length of left[i..], right[j..].
    QVector<QVector<int>> dp(n + 1, QVector<int>(m + 1, 0));
    for (int i = n - 1; i >= 0; --i)
    {
        for (int j = m - 1; j >= 0; --j)
        {
            if (left[i].name == right[j].name)
                dp[i][j] = 1 + dp[i + 1][j + 1];
            else
                dp[i][j] = std::max(dp[i + 1][j], dp[i][j + 1]);
        }
    }

    // Walk the table into steps: (leftIdx, rightIdx) with an unmatched side
    // at -1.
    struct Step { int li; int ri; };
    QVector<Step> steps;
    int i = 0, j = 0;
    while (i < n && j < m)
    {
        if (left[i].name == right[j].name)
        {
            steps.append({ i, j });
            ++i; ++j;
        }
        else if (dp[i + 1][j] >= dp[i][j + 1])
        {
            steps.append({ i, -1 });
            ++i;
        }
        else
        {
            steps.append({ -1, j });
            ++j;
        }
    }
    while (i < n) { steps.append({ i, -1 }); ++i; }
    while (j < m) { steps.append({ -1, j }); ++j; }

    QVector<SubDiff> out;
    for (int k = 0; k < steps.size(); )
    {
        const Step& s = steps[k];
        if (s.li < 0 || s.ri < 0)
        {
            if (s.li >= 0)
            {
                SubDiff d;
                d.name = left[s.li].name;
                d.status = DiffStatus::Removed;
                d.left = left[s.li].payload;
                out.append(d);
            }
            else
            {
                SubDiff d;
                d.name = right[s.ri].name;
                d.status = DiffStatus::Added;
                d.right = right[s.ri].payload;
                out.append(d);
            }
            ++k;
            continue;
        }

        // Maximal run of matched steps sharing this name.
        const NAME name = left[s.li].name;
        int run = 0;
        while (k + run < steps.size()
               && steps[k + run].li >= 0 && steps[k + run].ri >= 0
               && left[steps[k + run].li].name == name)
            ++run;

        const int leftStart = s.li;
        const int rightStart = s.ri;
        // The run's left indices are contiguous by construction (steps are
        // emitted in order), so pairing is positional.
        for (int p = 0; p < run; ++p)
        {
            const SubSnapshot& l = left[leftStart + p];
            const SubSnapshot& r = right[rightStart + p];
            SubDiff d;
            d.name = l.name;
            d.left = l.payload;
            d.right = r.payload;
            d.status = (l.payload == r.payload) ? DiffStatus::Same : DiffStatus::Modified;
            out.append(d);
        }
        // Surplus same-name subs past the matched run on either side.
        for (int q = run; leftStart + q < n && left[leftStart + q].name == name; ++q)
        {
            SubDiff d;
            d.name = name;
            d.status = DiffStatus::Removed;
            d.left = left[leftStart + q].payload;
            out.append(d);
        }
        for (int q = run; rightStart + q < m && right[rightStart + q].name == name; ++q)
        {
            SubDiff d;
            d.name = name;
            d.status = DiffStatus::Added;
            d.right = right[rightStart + q].payload;
            out.append(d);
        }
        k += run;
        // The surplus loops already reported those steps; do not emit them
        // again when the walk reaches their (unmatched) step entries.
        while (k < steps.size()
               && ((steps[k].li >= 0 && steps[k].li < n
                       && left[steps[k].li].name == name && steps[k].ri < 0)
                   || (steps[k].ri >= 0 && steps[k].ri < m
                       && right[steps[k].ri].name == name && steps[k].li < 0)))
        {
            ++k;
        }
    }
    return out;
}

} // namespace

PluginDiffReport PluginDiffer::diff(const QString& leftPath, const QString& rightPath)
{
    PluginDiffReport report;
    report.leftPath = leftPath;
    report.rightPath = rightPath;

    const QVector<RecordSnapshot> left = collectRecordSnapshots(leftPath);
    const QVector<RecordSnapshot> right = collectRecordSnapshots(rightPath);

    // Key by (type, formId): a record keeps its identity only if both hold.
    QHash<quint64, int> leftIndex;
    for (int i = 0; i < left.size(); ++i)
    {
        const quint64 key = (static_cast<quint64>(left[i].type) << 32)
                            | left[i].formId;
        leftIndex.insert(key, i);
    }
    QHash<quint64, int> rightIndex;
    for (int i = 0; i < right.size(); ++i)
    {
        const quint64 key = (static_cast<quint64>(right[i].type) << 32)
                            | right[i].formId;
        rightIndex.insert(key, i);
    }

    QVector<quint64> keys;
    for (auto it = leftIndex.constBegin(); it != leftIndex.constEnd(); ++it)
        keys.append(it.key());
    for (auto it = rightIndex.constBegin(); it != rightIndex.constEnd(); ++it)
        if (!leftIndex.contains(it.key()))
            keys.append(it.key());
    std::sort(keys.begin(), keys.end());

    for (quint64 key : keys)
    {
        const bool hasLeft = leftIndex.contains(key);
        const bool hasRight = rightIndex.contains(key);
        const RecordSnapshot& l = hasLeft ? left[leftIndex[key]] : RecordSnapshot();
        const RecordSnapshot& r = hasRight ? right[rightIndex[key]] : RecordSnapshot();

        RecordDiffEntry entry;
        entry.type = static_cast<NAME>(key >> 32);
        entry.formId = static_cast<quint32>(key);

        if (!hasRight)
        {
            entry.status = DiffStatus::Removed;
            entry.editorId = readEdid(l.subs);
            ++report.removedRecords;
            report.records.append(entry);
            continue;
        }
        if (!hasLeft)
        {
            entry.status = DiffStatus::Added;
            entry.editorId = readEdid(r.subs);
            ++report.addedRecords;
            report.records.append(entry);
            continue;
        }

        const bool recordEqual = (l == r);
        const QVector<SubDiff> subs = recordEqual ? QVector<SubDiff>() : alignSubs(l.subs, r.subs);
        if (recordEqual)
        {
            ++report.sameRecords;
            continue;
        }
        entry.status = DiffStatus::Modified;
        entry.editorId = readEdid(l.subs);
        entry.subs = subs;
        ++report.modifiedRecords;
        report.records.append(entry);
    }

    return report;
}

} // namespace openck

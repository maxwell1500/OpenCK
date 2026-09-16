#include "../libs/files/esm/subrecordsnapshot.hpp"

#include <QString>
#include <QTextStream>
#include <QHash>

#include <algorithm>
#include <cstdio>

// Subrecord-diff tool (Phase 1.2). Compares two plugin files at the
// per-record, per-subrecord payload level and reports every difference.
// Run the untouched round-trip through the save path first, then:
//
//   test_subrecord_diff <source.esp> <roundtrip_saved.esp>
//
// Exit code 0 = payload-identical, 1 = differences found, 2 = usage error.

using namespace openck;

static void printUsage(QTextStream& err)
{
    err << "Usage: test_subrecord_diff <source-file> <saved-file>\n"
        << "  Compares two files record-by-record at the subrecord-payload level.\n"
        << "  To gate an untouched round-trip: save the loaded plugin, then diff\n"
        << "  the original against the saved copy. Exit 0 means payload-identical.\n";
}

int main(int argc, char** argv)
{
    QTextStream err(stderr);
    if (argc < 3)
    {
        printUsage(err);
        return 2;
    }

    const QString srcPath = QString::fromLocal8Bit(argv[1]);
    const QString savedPath = QString::fromLocal8Bit(argv[2]);
    if (!QFileInfo::exists(srcPath) || !QFileInfo::exists(savedPath))
    {
        err << "Both files must exist.\n";
        return 2;
    }

    const QVector<RecordSnapshot> a = collectRecordSnapshots(srcPath);
    const QVector<RecordSnapshot> b = collectRecordSnapshots(savedPath);

    int mismatch = 0;
    int shifted = 0;
    int shown = 0;
    QHash<QString, int> mismatchByType;
    QHash<QString, int> payloadDiffByType;
    const int n = qMin(a.size(), b.size());
    for (int i = 0; i < n; ++i)
    {
        const RecordSnapshot& ra = a.at(i);
        const RecordSnapshot& rb = b.at(i);
        if (ra == rb)
            continue;

        ++mismatch;
        const QString tn = snapshotName(ra.type);
        ++mismatchByType[tn];
        if (ra.type == rb.type && ra.formId == rb.formId)
            ++payloadDiffByType[tn];
        else
            ++shifted;
        if (mismatchByType[tn] <= 2 && shown < 200)
        {
            ++shown;
            printf("MISMATCH #%d idx %d: %s 0x%08X subs %lld -> subs %lld (flags 0x%X->0x%X, size %u->%u)\n",
                mismatch, i,
                snapshotName(ra.type).toUtf8().constData(), ra.formId,
                static_cast<long long>(ra.subs.size()), static_cast<long long>(rb.subs.size()),
                ra.flags, rb.flags, ra.size, rb.size);

            const int s = qMin(ra.subs.size(), rb.subs.size());
            for (int j = 0; j < s; ++j)
            {
                if (ra.subs[j].name != rb.subs[j].name
                    || ra.subs[j].payload != rb.subs[j].payload)
                {
                    printf("  sub %d: %s len %lld -> %s len %lld\n",
                        j,
                        snapshotName(ra.subs[j].name).toUtf8().constData(),
                        static_cast<long long>(ra.subs[j].payload.size()),
                        snapshotName(rb.subs[j].name).toUtf8().constData(),
                        static_cast<long long>(rb.subs[j].payload.size()));
                }
            }
            for (int j = s; j < ra.subs.size(); ++j)
                printf("  extra source sub %d: %s len %lld\n", j,
                    snapshotName(ra.subs[j].name).toUtf8().constData(),
                    static_cast<long long>(ra.subs[j].payload.size()));
            for (int j = s; j < rb.subs.size(); ++j)
                printf("  extra saved sub %d: %s len %lld\n", j,
                    snapshotName(rb.subs[j].name).toUtf8().constData(),
                    static_cast<long long>(rb.subs[j].payload.size()));
        }
    }
    if (a.size() != b.size())
    {
        printf("RECORD COUNT DIFFERS: source %lld, saved %lld\n",
            static_cast<long long>(a.size()), static_cast<long long>(b.size()));
        mismatch += (a.size() != b.size()) ? (b.size() > a.size() ? b.size() - a.size() : a.size() - b.size()) : 0;
    }

    QVector<QPair<QString, int>> hist;
    for (auto it = mismatchByType.constBegin(); it != mismatchByType.constEnd(); ++it)
        hist.append({ it.key(), it.value() });
    std::sort(hist.begin(), hist.end(),
        [](const QPair<QString, int>& l, const QPair<QString, int>& r) {
            return l.second > r.second;
        });
    printf("mismatch by type (total %d, positional-shift %d):\n", mismatch, shifted);
    for (const auto& h : hist)
    {
        const int payload = payloadDiffByType.value(h.first, 0);
        printf("  %-6s %6d (%d same-id payload)\n",
            h.first.toUtf8().constData(), h.second, payload);
    }

    printf("source records %lld, saved records %lld, mismatched records %d\n",
        static_cast<long long>(a.size()), static_cast<long long>(b.size()), mismatch);
    return mismatch ? 1 : 0;
}
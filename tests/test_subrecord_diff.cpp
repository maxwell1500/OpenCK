#include "../libs/files/esm/subrecordsnapshot.hpp"

#include <QString>
#include <QTextStream>

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
    const int n = qMin(a.size(), b.size());
    for (int i = 0; i < n; ++i)
    {
        const RecordSnapshot& ra = a.at(i);
        const RecordSnapshot& rb = b.at(i);
        if (ra == rb)
            continue;

        ++mismatch;
        if (mismatch <= 50)
        {
            printf("MISMATCH #%d idx %d: %s 0x%08X subs %d -> subs %d (flags 0x%X->0x%X, size %u->%u)\n",
                mismatch, i,
                snapshotName(ra.type).toUtf8().constData(), ra.formId,
                ra.subs.size(), rb.subs.size(),
                ra.flags, rb.flags, ra.size, rb.size);

            const int s = qMin(ra.subs.size(), rb.subs.size());
            for (int j = 0; j < s; ++j)
            {
                if (ra.subs[j].name != rb.subs[j].name
                    || ra.subs[j].payload != rb.subs[j].payload)
                {
                    printf("  sub %d: %s len %d -> %s len %d\n",
                        j,
                        snapshotName(ra.subs[j].name).toUtf8().constData(),
                        ra.subs[j].payload.size(),
                        snapshotName(rb.subs[j].name).toUtf8().constData(),
                        rb.subs[j].payload.size());
                }
            }
            for (int j = s; j < ra.subs.size(); ++j)
                printf("  extra source sub %d: %s len %d\n", j,
                    snapshotName(ra.subs[j].name).toUtf8().constData(),
                    ra.subs[j].payload.size());
            for (int j = s; j < rb.subs.size(); ++j)
                printf("  extra saved sub %d: %s len %d\n", j,
                    snapshotName(rb.subs[j].name).toUtf8().constData(),
                    rb.subs[j].payload.size());
        }
    }
    if (a.size() != b.size())
    {
        printf("RECORD COUNT DIFFERS: source %d, saved %d\n", a.size(), b.size());
        mismatch += (a.size() != b.size()) ? (b.size() > a.size() ? b.size() - a.size() : a.size() - b.size()) : 0;
    }

    printf("source records %d, saved records %d, mismatched records %d\n",
        a.size(), b.size(), mismatch);
    return mismatch ? 1 : 0;
}
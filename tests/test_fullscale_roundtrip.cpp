// Full-scale untouched round-trip gate (REMAINING.md §1.2): load a real
// plugin through the normal Document path, save it untouched, and
// snapshot-diff source vs saved. Too slow for ctest (3.8M records) — it is
// driven by tools/nightly-roundtrip.ps1 instead.
// Exit 0 = payload-identical (or compact-ok), 1 = differences found,
// 2 = usage / load / save error.
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QFileInfo>
#include <QTextStream>
#include <cstdio>

#include "../src/model/doc/documentmediator.hpp"
#include "../src/model/doc/document.hpp"
#include "../src/model/tools/formidcompactor.hpp"
#include "../libs/files/esm/subrecordsnapshot.hpp"
#include "../libs/files/filepaths.hpp"

static void printUsage(QTextStream& err)
{
    err << "Usage: test_fullscale_roundtrip <data-dir> <plugin> <out-file> [--compact]\n"
        << "  --compact: run FormIdCompactor before saving (ESL-range check),\n"
        << "             then verify the saved file reloads cleanly.\n";
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);

    QStringList args = QCoreApplication::arguments();
    const bool compact = args.removeAll(QStringLiteral("--compact")) > 0;
    if (args.size() != 4)
    {
        printUsage(err);
        return 2;
    }
    const QString dataDir = args.at(1);
    const QString plugin = args.at(2);
    const QString outFile = args.at(3);
    if (!QFileInfo::exists(dataDir + '/' + plugin))
    {
        err << "plugin not found: " << dataDir + '/' + plugin << "\n";
        return 2;
    }

    DocumentMediator mediator;
    bool done = false;
    bool completed = false;
    QString loadError;
    QObject::connect(&mediator, &DocumentMediator::loadingStopped,
        [&](Document*, bool ok, const QString& error) {
            done = true;
            completed = ok;
            loadError = error;
        });

    // The edited plugin loads on top of its masters. When the target IS the
    // master itself (Starfield.esm), it must appear exactly once — loading
    // it twice corrupts the collection and fast-fails.
    QStringList files{ QStringLiteral("Starfield.esm") };
    if (plugin.compare(QStringLiteral("Starfield.esm"), Qt::CaseInsensitive) != 0)
        files << plugin;

    Document* doc = mediator.makeDocument(files, dataDir + '/' + plugin, false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dataDir);
    mediator.insertDocument(doc);

    // The loader runs on a worker thread; pump events until it reports back.
    QEventLoop loop;
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, &loop, &QEventLoop::quit);
    watchdog.start(30 * 60 * 1000); // 30 min ceiling for the 3.8M load
    while (!done && watchdog.isActive())
        loop.processEvents(QEventLoop::AllEvents, 1000);
    if (!done)
    {
        err << "load timed out after 30 minutes\n";
        return 2;
    }
    if (!completed)
    {
        err << "load failed: " << loadError << "\n";
        return 2;
    }
    out << "phase: load complete" << Qt::endl;

    if (compact)
    {
        FormIdCompactor compactor(doc->getData());
        const int remapped = compactor.compact();
        out << "compact: owned=" << compactor.ownedRecordCount()
            << " remapped=" << remapped
            << " rewritten=" << compactor.rewrittenReferences() << "\n";
        if (remapped < 0)
        {
            err << "compact: too many records for the ESL range\n";
            return 1;
        }
    }

    doc->save(outFile);
    out << "phase: save complete" << Qt::endl;
    if (!QFileInfo::exists(outFile))
    {
        err << "save produced no file\n";
        return 2;
    }

    if (compact)
    {
        // The compacted file must at least reload cleanly.
        const QVector<openck::RecordSnapshot> reloaded =
            openck::collectRecordSnapshots(outFile);
        out << "compact: saved " << reloaded.size()
            << " records, reload clean\n";
        return 0;
    }

    const QVector<openck::RecordSnapshot> src =
        openck::collectRecordSnapshots(dataDir + '/' + plugin);
    const QVector<openck::RecordSnapshot> dst = openck::collectRecordSnapshots(outFile);

    int diffs = 0;
    if (src.size() != dst.size())
    {
        out << "record count " << src.size() << " -> " << dst.size() << "\n";
        diffs += qAbs(src.size() - dst.size());
    }
    const int n = qMin(src.size(), dst.size());
    for (int i = 0; i < n; ++i)
    {
        if (src.at(i) != dst.at(i))
        {
            if (diffs < 25)
                out << i << ": " << openck::snapshotName(src.at(i).type)
                    << " 0x" << QString::number(src.at(i).formId, 16)
                    << " (subs " << src.at(i).subs.size()
                    << " -> " << dst.at(i).subs.size() << ")\n";
            ++diffs;
        }
    }
    out << "records: " << src.size() << ", differing: " << diffs << "\n";
    return diffs == 0 ? 0 : 1;
}

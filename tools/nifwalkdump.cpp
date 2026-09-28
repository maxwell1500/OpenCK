// Dumps this build's idea of a NIF's block boundaries, one per line, as
// "index type offset" - including the blocks walked before a split failed.
//
// The point is to diff it against the reference reader (nifgen, under the
// bundled NifTools addon). A pre-20.2.0.5 container records no block lengths, so
// the boundaries are recovered by walking each block's fields, and a single
// wrong payload length shifts every block after it. lastWalkError() names the
// block the walk gave up on, which is not the wrong one: by then the position
// is already lost. Comparing the offsets below against the reference's io_start
// values finds the first block that actually disagrees, which is the one to fix.
//
// Output is meant to be compared, not read: the exit code is 0 even when the
// split failed, so a caller can tell "the tool ran" from "the file walked".

#include "nifblockfile.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() < 2) {
        QTextStream(stderr) << "usage: nifwalkdump <file.nif> [more.nif ...]\n";
        return 2;
    }

    QTextStream out(stdout);
    // An optional first argument of --save <dir> makes each file re-serialized
    // to <dir>, so the result can be compared against the original byte for
    // byte. Without it this is a read-only diagnostic.
    QString saveDir;
    int first = 1;
    if (args.size() > 3 && args.at(1) == QStringLiteral("--save")) {
        saveDir = args.at(2);
        first = 3;
    }
    for (int a = first; a < args.size(); ++a) {
        NifBlockFile file;
        QString error;
        if (!file.load(args.at(a))) {
            out << "FILE " << args.at(a) << " load-failed " << file.lastWalkError() << "\n";
            continue;
        }
        out << "FILE " << args.at(a) << " version " << file.headerVersion()
            << " bs " << file.bsVersion() << " blocks " << file.declaredBlockCount() << "\n";
        const int walked = file.walkedBlockCount();
        for (int i = 0; i < walked; ++i) {
            out << i << " " << file.walkedBlockType(i) << " " << file.walkedBlockOffset(i) << "\n";
        }
        if (!file.hasIndividualBlocks()) {
            out << "INCOMPLETE " << walked << " of " << file.declaredBlockCount()
                << " " << file.lastWalkError() << "\n";
        }
        if (!saveDir.isEmpty()) {
            QDir().mkpath(saveDir);
            const QString dest = saveDir + QLatin1Char('/')
                + QFileInfo(args.at(a)).fileName();
            file.save(dest);
        }
    }
    out.flush();
    return 0;
}

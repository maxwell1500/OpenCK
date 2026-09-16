// Diagnostic: find files in a BA2 archive by path substring and optionally
// extract them. Used by the §8.3 gate to resolve the external .mesh paths
// that shipped Starfield BSGeometry blocks carry.
// Usage: test_meshresolve <archive.ba2> [substring] [--extract <outdir>]
// Exit 0 = at least one match (or no substring given), 1 = no match, 2 = usage.
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <cstdio>

#include "../libs/files/ba2/ba2archive.hpp"

static void printUsage(QTextStream& err)
{
    err << "Usage: test_meshresolve <archive.ba2> [substring] [--extract <outdir>]\n";
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);

    QStringList args = QCoreApplication::arguments();
    QString outDir;
    const int xIdx = args.indexOf(QStringLiteral("--extract"));
    if (xIdx >= 0)
    {
        if (xIdx + 1 >= args.size())
        {
            printUsage(err);
            return 2;
        }
        outDir = args.at(xIdx + 1);
        args.removeAt(xIdx + 1);
        args.removeAt(xIdx);
    }
    if (args.size() != 2 && args.size() != 3)
    {
        printUsage(err);
        return 2;
    }
    const QString archive = args.at(1);
    const QString sub = args.size() == 3 ? args.at(2) : QString();

    Ba2Archive ba2;
    if (!ba2.open(archive))
    {
        err << "open failed: " << archive << "\n";
        return 2;
    }
    out << "archive: " << ba2.name() << " files=" << ba2.fileCount() << "\n";

    int matches = 0;
    for (quint32 i = 0; i < ba2.fileCount(); ++i)
    {
        const Ba2FileEntry& e = ba2.entries().at(i);
        if (!sub.isEmpty()
            && !e.relativePath.contains(sub, Qt::CaseInsensitive))
            continue;
        ++matches;
        if (matches <= 50)
            out << i << " " << e.relativePath << " (" << e.uncompressedSize
                << (e.compressed ? ", compressed" : "") << ")\n";
        if (!outDir.isEmpty())
        {
            QDir().mkpath(outDir);
            const QString dest = outDir + '/'
                + QFileInfo(e.relativePath).fileName();
            if (!ba2.extract(i, dest))
                err << "extract failed: " << e.relativePath << "\n";
            else if (matches <= 50)
                out << "  -> " << dest << "\n";
        }
    }
    out << "matches: " << matches << "\n";
    return matches > 0 ? 0 : 1;
}

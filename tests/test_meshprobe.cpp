#include <QCoreApplication>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QFile>

#include "ba2archive.hpp"

// Diagnostic: open a Starfield mesh BA2, report NIF entries + compression,
// and check whether a bhkPhysicsSystem / TAG0 block exists in one mesh.
static void probe(const QString& path, QTextStream& out)
{
    Ba2Archive ba2;
    if (!ba2.open(path))
    {
        out << "open failed: " << path << "\n";
        return;
    }
    out << "archive: " << ba2.name() << " files=" << ba2.fileCount() << "\n";
    int nifCount = 0, compressed = 0;
    int firstNif = -1;
    for (quint32 i = 0; i < ba2.fileCount(); ++i)
    {
        const auto& e = ba2.entries().at(i);
        if (e.relativePath.endsWith(".nif", Qt::CaseInsensitive))
        {
            if (nifCount == 0) firstNif = static_cast<int>(i);
            ++nifCount;
            if (e.compressed) ++compressed;
        }
    }
    out << "nif entries: " << nifCount << " (compressed " << compressed << ")\n";

    // Scan NIFs for one that carries a bhkPhysicsSystem / TAG0 collision block.
    int scanned = 0;
    for (quint32 i = 0; i < ba2.fileCount() && scanned < 200; ++i)
    {
        const auto& e = ba2.entries().at(i);
        if (!e.relativePath.endsWith(".nif", Qt::CaseInsensitive)) continue;
        ++scanned;
        QString tmp = QDir::tempPath() + "/openck_probe.nif";
        if (!ba2.extract(i, tmp)) continue;
        QFile f(tmp);
        if (!f.open(QIODevice::ReadOnly)) { QFile::remove(tmp); continue; }
        QByteArray data = f.readAll();
        f.close();
        QFile::remove(tmp);
        if (data.contains("bhkPhysicsSystem") || data.contains("TAG0"))
        {
            out << "first collision nif: " << e.relativePath
                << " (index " << i << ", scanned " << scanned << ")\n";
            out << "size=" << data.size() << " head: " << data.left(24).constData() << "\n";
            const int tag0 = data.indexOf("TAG0");
            out << "TAG0 at bytes: " << tag0 << "\n";
            const QByteArray region = data.mid(tag0 - 8, 320);
            out << "TAG0 region head (" << (tag0 - 8) << "): "
                << region.toHex(' ').constData() << "\n";
            for (const char* cc : { "SDKV", "DATA", "TYPE", "INDX", "ITEM", "PTCH" })
                out << cc << " at bytes: " << data.indexOf(cc, tag0) << "\n";
            return;
        }
    }
    out << "no collision NIF found in first " << scanned << " nif entries\n";
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1])
        : QStringLiteral("C:/XboxGames/Starfield/Content/Data/Starfield - Meshes01.ba2");
    probe(dir, out);
    out.flush();
    return 0;
}

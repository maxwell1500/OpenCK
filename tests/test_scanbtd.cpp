#include <QCoreApplication>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QString>

#include "ba2archive.hpp"
#include "bsaarchive.hpp"

// Diagnostic: scans every archive in a data directory for .btd entries so the
// real BTD binary layout can be validated against samples. Run:
//   test_scanbtd [dataDir]
static void scanDir(const QString& dir, QTextStream& out)
{
    QDir d(dir);
    if (!d.exists())
    {
        out << "no such dir: " << dir << "\n";
        return;
    }
    const auto archives = d.entryList({ "*.ba2", "*.bsa" }, QDir::Files, QDir::Name);
    int btdCount = 0;
    for (const auto& a : archives)
    {
        const QString full = d.absoluteFilePath(a);
        QByteArray sample;
        QStringList btd;
        if (a.endsWith(".ba2", Qt::CaseInsensitive))
        {
            Ba2Archive ba2;
            if (!ba2.open(full)) { out << "BA2 open failed: " << a << "\n"; continue; }
            for (const auto& e : ba2.entries())
                if (e.relativePath.endsWith(".btd", Qt::CaseInsensitive))
                    btd << e.relativePath;
        }
        else
        {
            BsaArchive bsa;
            if (!bsa.open(full)) { out << "BSA open failed: " << a << "\n"; continue; }
            for (const auto& e : bsa.entries())
                if (e.fullPath.endsWith(".btd", Qt::CaseInsensitive))
                    btd << e.fullPath;
        }
        if (!btd.isEmpty())
        {
            out << a << ": " << btd.size() << " .btd entries\n";
            for (const auto& p : btd) out << "  " << p << "\n";
            btdCount += btd.size();
        }
    }
    out << "total .btd entries in " << dir << ": " << btdCount << "\n";
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1])
        : QStringLiteral("C:/XboxGames/Starfield/Content/Data");
    scanDir(dir, out);
    out.flush();
    return 0;
}

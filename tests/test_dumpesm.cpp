#include <QCoreApplication>
#include <QTextStream>
#include <QMap>
#include <QSet>
#include <QFile>
#include <QFileInfo>

#include "esmreader.hpp"
#include "common.hpp"

static void dumpName(NAME n, QTextStream& out)
{
    out << QChar((n >> 24) & 0xFF) << QChar((n >> 16) & 0xFF)
        << QChar((n >> 8) & 0xFF) << QChar(n & 0xFF);
}

static int gScanned = 0;
static QString gBrokeReason;

static int countRecords(ESMReader& reader, NAME want, QTextStream& out)
{
    int found = 0;
    QMap<NAME, int> counts;
    QMap<NAME, int> firstPositions;
    while (reader.isLeft() && gScanned < 30000000)
    {
        qint64 pos = reader.filePos();
        NAME name = 0;
        try {
            name = reader.readName();
        } catch (...) {
            gBrokeReason = QString("readName threw at 0x%1").arg(pos, 0, 16);
            out << gBrokeReason << "\n";
            break;
        }
        if (name == 0)
        {
            gBrokeReason = "readName returned 0";
            break;
        }

        if (name == (NAME)'GRUP')
        {
            qint64 grupPos = pos;
            reader.skipGrupHeader();
            qint64 grupEnd = reader.grupEnd();
            if (grupEnd <= grupPos)
            {
                out << "BAD GRUP at 0x" << Qt::hex << grupPos << Qt::dec
                    << " end=" << grupEnd << "\n";
                break;
            }
            continue;
        }

        if (name == want)
        {
            RecHeader hdr = reader.readHeader();
            if (hdr.size < 0 || hdr.size > 100000000LL)
            {
                out << "BAD size for ";
                dumpName(name, out);
                out << " size=" << hdr.size << " at 0x" << Qt::hex << pos << Qt::dec << "\n";
                break;
            }

            if (found < 3)
            {
                out << "\n=== " << want << " record #" << found
                    << " at 0x" << Qt::hex << pos << Qt::dec
                    << " size=" << hdr.size << " flags=0x" << Qt::hex << hdr.flags.val << Qt::dec
                    << " formId=0x" << Qt::hex << hdr.id << Qt::dec << "\n";
            }
            int subCount = 0;
            while (reader.isRecLeft())
            {
                NAME sub = reader.readNSubHeader();
                if (sub == 0) break;
                qint64 sz = reader.subLeft();
                QByteArray data;
                reader.readRawSubData(data);
                if (found < 3)
                {
                    out << "  SUB ";
                    dumpName(sub, out);
                    out << " size=" << sz;
                    if (sz <= 24) {
                        out << " data=[";
                        for (qint64 i = 0; i < sz; ++i)
                            out << QString::number((quint8)data[i], 16).rightJustified(2, '0');
                        out << "]";
                    } else {
                        out << " first16=[";
                        for (int i = 0; i < 16; ++i)
                            out << QString::number((quint8)data[i], 16).rightJustified(2, '0');
                        out << "]";
                    }
                    out << "\n";
                }
                subCount++;
            }
            if (found < 3)
                out.flush();
            found++;

            // Drain any leftover compressed buffer so the walk stays aligned.
            reader.skipRemainingRecord();
        }
        else
        {
            if (!counts.contains(name))
                firstPositions.insert(name, (int)pos);
            counts[name] = counts[name] + 1;
            reader.skipRecord();
        }
        gScanned++;
        if (gScanned % 500000 == 0)
        {
            out << "...scanned " << gScanned << " records\n";
            out.flush();
        }
    }

    out << "\nDistinct record types seen (" << counts.size() << "):\n";
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
    {
        out << "  ";
        dumpName(it.key(), out);
        out << " x" << it.value() << " (first at 0x" << Qt::hex
            << firstPositions.value(it.key()) << Qt::dec << ")\n";
    }
    return found;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QString path = "C:/XboxGames/Starfield/Content/Data/Starfield.esm";
    NAME want = 0;
    if (argc > 1) {
        QByteArray tag = argv[1];
        if (tag.size() >= 4)
            want = ((quint32)(quint8)tag[0] << 24) | ((quint32)(quint8)tag[1] << 16)
                 | ((quint32)(quint8)tag[2] << 8) | (quint32)(quint8)tag[3];
    }
    else
    {
        // Diagnostic tool: requires a 4-char record tag. Exits immediately so
        // the generic test loop doesn't scan the 1.4GB ESM in Debug builds.
        fprintf(stdout, "usage: test_dumpesm PNDT [outfile]\n");
        return 0;
    }

    QFile outFile;
    if (argc > 2)
    {
        outFile.setFileName(QString::fromLocal8Bit(argv[2]));
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            qWarning("cannot open output file");
            return 1;
        }
    }
    else
    {
        outFile.open(stdout, QIODevice::WriteOnly);
    }
    QTextStream out(&outFile);

    ESMReader reader(path);
    try {
        reader.open();
    } catch (const std::exception& e) {
        out << "open failed: " << e.what() << "\n";
        out.flush();
        return 1;
    }
    int n = countRecords(reader, want, out);
    out << "\nTotal " << want << " records dumped: " << n << "\n";
    out << "Final file position: 0x" << Qt::hex << reader.filePos() << Qt::dec
        << " of " << QFileInfo(path).size() << "\n";
    out << "scanned " << gScanned << " records; loop broke after "
        << (gBrokeReason.isEmpty() ? "EOF/isLeft false" : gBrokeReason) << "\n";
    out.flush();
    return 0;
}

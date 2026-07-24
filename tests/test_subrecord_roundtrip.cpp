#include <QtTest>
#include <QFile>
#include <QBuffer>
#include <QByteArray>
#include <QVector>
#include <QDataStream>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/common.hpp"
#include "../../libs/files/esm/records.hpp"

struct RawSub {
    NAME name;
    QByteArray data;
};

struct CopiedRecord {
    NAME name;
    quint32 origSize;
    quint32 flags;
    quint32 formId;
    quint8 vcDay, vcMonth, vcLastUser, vcCurrUser;
    quint16 version;
    quint16 unknown;
    QVector<RawSub> subs;
};

static QString nameToQStr(NAME n)
{
    if (n == 0) return "NULL";
    return QString(QChar((n >> 24) & 0xFF)) +
           QString(QChar((n >> 16) & 0xFF)) +
           QString(QChar((n >> 8) & 0xFF)) +
           QString(QChar(n & 0xFF));
}

class TestSubrecordRoundtrip : public QObject
{
    Q_OBJECT

    QString mFilePath;
    int mTotalNonCompressed = 0;
    int mRoundtripOk = 0;
    int mRoundtripFail = 0;

    // Write a 4-byte NAME to stream in on-disk order (big-endian ASCII)
    static void writeNameLE(QDataStream& out, NAME name)
    {
        char buf[4];
        buf[0] = static_cast<char>((name >> 24) & 0xFF);
        buf[1] = static_cast<char>((name >> 16) & 0xFF);
        buf[2] = static_cast<char>((name >> 8) & 0xFF);
        buf[3] = static_cast<char>(name & 0xFF);
        out.writeRawData(buf, 4);
    }

    // Read raw record bytes from a QIODevice (assumes positioned at record name)
    static QByteArray readRawRecordBytes(QIODevice& dev, qint64 recStart)
    {
        dev.seek(recStart);
        // Read 24-byte header
        char header[24];
        qint64 r = dev.read(header, 24);
        if (r != 24) return {};

        // Parse size from bytes 4-7 (LE)
        quint32 dataSize;
        memcpy(&dataSize, header + 4, 4);

        // Read data
        qint64 total = 24 + dataSize;
        dev.seek(recStart);
        QByteArray raw;
        raw.resize(static_cast<int>(total));
        r = dev.read(raw.data(), static_cast<int>(total));
        if (r != total) return {};
        return raw;
    }

    // Serialize a CopiedRecord back to on-disk format
    static QByteArray serializeRecord(const CopiedRecord& rec)
    {
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);

        writeNameLE(out, rec.name);

        qint64 sizePos = data.size();
        out << (quint32)0; // placeholder size

        out << rec.flags;
        out << rec.formId;
        out << (quint8)rec.vcDay;
        out << (quint8)rec.vcMonth;
        out << (quint8)rec.vcLastUser;
        out << (quint8)rec.vcCurrUser;
        out << (quint16)rec.version;
        out << (quint16)rec.unknown;

        qint64 dataStart = data.size();

        for (const auto& sub : rec.subs) {
            writeNameLE(out, sub.name);
            out << (quint16)sub.data.size();
            out.writeRawData(sub.data.constData(), sub.data.size());
        }

        qint64 endPos = data.size();
        quint32 actualSize = static_cast<quint32>(endPos - dataStart);
        out.device()->seek(sizePos);
        out << actualSize;

        return data;
    }

private slots:
    void initTestCase()
    {
        mFilePath = "C:/XboxGames/Starfield/Content/Data/Starfield.esm";
        QVERIFY2(QFile::exists(mFilePath), "Starfield.esm not found");
    }

    void testRoundTripFirstGrupRecords()
    {
        QFile file(mFilePath);
        QVERIFY2(file.open(QIODevice::ReadOnly), "Cannot open Starfield.esm");

        // We need to navigate manually since ESMReader modifies the file state.
        // Strategy: read the GRUP structure manually at the file level.
        // Skip TES4 record (24 bytes + data size).
        char tes4Header[24];
        file.read(tes4Header, 24);
        quint32 tes4DataSize;
        memcpy(&tes4DataSize, tes4Header + 4, 4);
        file.skip(tes4DataSize);

        // Read first GRUP header
        char grupName[4];
        file.read(grupName, 4);
        if (memcmp(grupName, "GRUP", 4) != 0)
            QFAIL("First top-level element is not GRUP");

        char grupHeader[20];
        file.read(grupHeader, 20);
        quint32 grupSize;
        memcpy(&grupSize, grupHeader, 4); // bytes 0-3 of GRUP header after name = size
        qint64 grupStart = 24 + tes4DataSize; // position of GRUP name
        qint64 grupEnd = grupStart + 4 + grupSize;

        qDebug("TES4 header size: %u", tes4DataSize);
        qDebug("First GRUP size: %u at offset %lld", grupSize, grupStart);

        // Now iterate records in this GRUP
        QVector<CopiedRecord> records;
        file.seek(grupStart + 24); // skip GRUP header (4 name + 20 header fields)

        constexpr quint32 FLAG_COMPRESSED = 0x00040000;
        int maxRecords = 50;

        while (file.pos() < grupEnd && records.size() < maxRecords)
        {
            qint64 recStart = file.pos();

            // Read record name (4 bytes)
            char recNameBytes[4];
            if (file.read(recNameBytes, 4) != 4) break;
            // Check for nested GRUP
            if (memcmp(recNameBytes, "GRUP", 4) == 0) break;

            // Read record header (20 more bytes for total of 24)
            char recRest[20];
            if (file.read(recRest, 20) != 20) break;

            quint32 dataSize;
            memcpy(&dataSize, recRest, 4);

            // Check compressed flag (at offset 8 in header = byte 12 from recStart)
            quint32 flags;
            memcpy(&flags, recRest + 4, 4);

            bool compressed = (flags & FLAG_COMPRESSED) != 0;

            qint64 dataStart = file.pos();
            qint64 recEnd = dataStart + dataSize;

            if (!compressed)
            {
                // Read raw subrecords
                CopiedRecord rec;
                rec.name = (static_cast<NAME>(static_cast<unsigned char>(recNameBytes[0])) << 24) |
                           (static_cast<NAME>(static_cast<unsigned char>(recNameBytes[1])) << 16) |
                           (static_cast<NAME>(static_cast<unsigned char>(recNameBytes[2])) << 8) |
                           static_cast<NAME>(static_cast<unsigned char>(recNameBytes[3]));
                rec.origSize = dataSize;
                rec.flags = flags;
                memcpy(&rec.formId, recRest + 8, 4);
                rec.vcDay = recRest[12];
                rec.vcMonth = recRest[13];
                rec.vcLastUser = recRest[14];
                rec.vcCurrUser = recRest[15];
                memcpy(&rec.version, recRest + 16, 2);
                memcpy(&rec.unknown, recRest + 18, 2);

                while (file.pos() < recEnd)
                {
                    if (recEnd - file.pos() < 6) break;

                    char subNameBytes[4];
                    if (file.read(subNameBytes, 4) != 4) break;

                    unsigned char subSizeBytes[2];
                    if (file.read(reinterpret_cast<char*>(subSizeBytes), 2) != 2) break;
                    quint16 subSize = static_cast<quint16>(subSizeBytes[0]) |
                                     (static_cast<quint16>(subSizeBytes[1]) << 8);

                    RawSub sub;
                    sub.name = (static_cast<NAME>(static_cast<unsigned char>(subNameBytes[0])) << 24) |
                               (static_cast<NAME>(static_cast<unsigned char>(subNameBytes[1])) << 16) |
                               (static_cast<NAME>(static_cast<unsigned char>(subNameBytes[2])) << 8) |
                               static_cast<NAME>(static_cast<unsigned char>(subNameBytes[3]));
                    sub.data.resize(subSize);
                    if (subSize > 0)
                        file.read(sub.data.data(), subSize);

                    rec.subs.append(sub);
                }

                file.seek(recEnd);
                records.append(rec);
            }
            else
            {
                // Skip compressed record
                file.seek(recEnd);
            }
        }

        mTotalNonCompressed = records.size();
        qDebug("Found %d non-compressed records in first GRUP", mTotalNonCompressed);
        QVERIFY(mTotalNonCompressed > 0);

        // Now test round-trip: for each record, compare serialized vs original bytes
        int firstNFails = 0;
        for (int i = 0; i < records.size(); i++)
        {
            const auto& rec = records[i];

            // Read original raw bytes
            qint64 recStartInFile = -1;
            file.seek(grupStart + 24);
            int idx = 0;
            while (file.pos() < grupEnd && idx <= i)
            {
                qint64 pos = file.pos();
                char rn[4];
                file.read(rn, 4);
                char rr[20];
                file.read(rr, 20);
                quint32 sz;
                memcpy(&sz, rr, 4);
                if (idx == i)
                {
                    recStartInFile = pos;
                    break;
                }
                file.skip(sz);
                idx++;
            }
            QVERIFY(recStartInFile >= 0);

            QByteArray original = readRawRecordBytes(file, recStartInFile);
            QByteArray serialized = serializeRecord(rec);

            bool match = (original == serialized);

            if (!match)
            {
                firstNFails++;
                qDebug("MISMATCH record %d: %s size=%u subs=%d origLen=%d serLen=%d",
                    i, nameToQStr(rec.name).toUtf8().constData(),
                    rec.origSize, rec.subs.size(),
                    (int)original.size(), (int)serialized.size());

                if (firstNFails <= 3)
                {
                    // Find first differing byte
                    int minLen = qMin(original.size(), serialized.size());
                    for (int b = 0; b < minLen; b++)
                    {
                        if (original[b] != serialized[b])
                        {
                            qDebug("  First diff at byte %d: orig=0x%02x ser=0x%02x",
                                b, (unsigned char)original[b], (unsigned char)serialized[b]);
                            // Show context
                            int ctxStart = qMax(0, b - 4);
                            int ctxEnd = qMin(minLen, b + 8);
                            QByteArray origCtx = original.mid(ctxStart, ctxEnd - ctxStart);
                            QByteArray serCtx = serialized.mid(ctxStart, ctxEnd - ctxStart);
                            qDebug("  Orig bytes: %s", origCtx.toHex().constData());
                            qDebug("  Ser  bytes: %s", serCtx.toHex().constData());
                            break;
                        }
                    }
                }
            }
            else
            {
                mRoundtripOk++;
            }
        }

        mRoundtripFail = firstNFails;

        qDebug("\n=== ROUND-TRIP RESULTS ===");
        qDebug("Non-compressed records tested: %d", mTotalNonCompressed);
        qDebug("Byte-identical: %d", mRoundtripOk);
        qDebug("Mismatches: %d", mRoundtripFail);

        // Require at least 80% of records to round-trip correctly
        if (mTotalNonCompressed > 0)
        {
            double passRate = static_cast<double>(mRoundtripOk) / mTotalNonCompressed;
            qDebug("Pass rate: %.1f%%", passRate * 100);
            QVERIFY2(passRate >= 0.8,
                qPrintable(QString("Round-trip pass rate too low: %1%").arg(passRate * 100, 0, 'f', 1)));
        }
    }
};

QTEST_MAIN(TestSubrecordRoundtrip)
#include "test_subrecord_roundtrip.moc"

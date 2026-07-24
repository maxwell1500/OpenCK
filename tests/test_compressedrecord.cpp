#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDataStream>
#include <QByteArray>
#include <QString>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/tes4.hpp"
#include "../../libs/files/esm/common.hpp"
#include "../../libs/files/esm/cellrecord.hpp"
#include "../../libs/files/log/logger.hpp"

#include <zlib.h>

// Helper: compress data into a Bethesda-style compressed record payload:
// 4 bytes LE uncompressed size + zlib stream (0x78 0x9C + deflate + 4-byte adler32)
static QByteArray makeBethesdaCompressed(const QByteArray& data)
{
    uLongf compressedBound = compressBound(static_cast<uLong>(data.size()));
    QByteArray zlibData(static_cast<int>(4 + compressedBound + 16), '\0');

    uLongf destLen = static_cast<uLongf>(zlibData.size() - 4);
    int res = compress2(reinterpret_cast<Bytef*>(zlibData.data() + 4),
                        &destLen,
                        reinterpret_cast<const Bytef*>(data.constData()),
                        static_cast<uLong>(data.size()),
                        Z_DEFAULT_COMPRESSION);
    if (res != Z_OK) {
        qWarning() << "zlib compress2 failed:" << res;
        return QByteArray();
    }

    QByteArray sizeHeader;
    QDataStream ds(&sizeHeader, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds << quint32(data.size());

    return sizeHeader + zlibData.mid(4, static_cast<int>(destLen));
}

class TestCompressedRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCompressedCellRecord();
    void testRoundTripCompressedCell();
    void testMasterNameTrim();
};

void TestCompressedRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init("C:/Users/max/Projects/OpenCK/test_compressed_log.txt");
}

void TestCompressedRecord::testCompressedCellRecord()
{
    QString filePath = "C:/Users/max/Projects/OpenCK/test_compressed_check.esm";

    QByteArray cellPayload;
    {
        QDataStream ds(&cellPayload, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds.setFloatingPointPrecision(QDataStream::SinglePrecision);
        ds.writeRawData("EDID", 4);
        ds << quint16(10);
        ds.writeRawData("TestCell\0\0", 10);
        ds.writeRawData("FULL", 4);
        ds << quint16(8);
        ds.writeRawData("My Cell\0", 8);
    }

    QByteArray compressedPayload = makeBethesdaCompressed(cellPayload);

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // TES4 record (28 bytes data: HEDR 18 + CNAM 10)
    out.writeRawData("TES4", 4);
    out << quint32(28);
    out << quint32(0x00000001);
    out << quint32(0);
    out << quint32(0);  // vc fields
    out << quint16(0);  // version
    out << quint16(0);  // unknown
    out.writeRawData("HEDR", 4);
    out << quint16(12);
    out << float(1.0f);
    out << qint32(0);
    out << quint32(0);
    out.writeRawData("CNAM", 4);
    out << quint16(4);
    out.writeRawData("Test", 4);

    // CELL record (compressed)
    out.writeRawData("CELL", 4);
    out << quint32(compressedPayload.size());
    out << quint32(0x00040000);  // FLAG_COMPRESSED
    out << quint32(1);
    out << quint32(0);
    out << quint16(0);
    out << quint16(0);
    out.writeRawData(compressedPayload.constData(), compressedPayload.size());

    file.close();

    QFile verify(filePath);
    QVERIFY(verify.open(QIODevice::ReadOnly));
    QByteArray data = verify.readAll();
    verify.close();

    qDebug() << "File size:" << data.size();
    qDebug() << "File hex:" << data.toHex(' ');
    QVERIFY(data.size() > 60);
    // Verify the CELL flags include 0x00040000
    int cellOffset = 24 + 28 + 4;  // skip TES4 + CELL name
    QByteArray flagsBytes = data.mid(cellOffset + 4, 4);
    quint32 cellFlags = static_cast<quint8>(flagsBytes[0])
        | (static_cast<quint8>(flagsBytes[1]) << 8)
        | (static_cast<quint8>(flagsBytes[2]) << 16)
        | (static_cast<quint8>(flagsBytes[3]) << 24);
    QVERIFY(cellFlags & 0x00040000);
}

void TestCompressedRecord::testRoundTripCompressedCell()
{
    QString filePath = "C:/Users/max/Projects/OpenCK/test_compressed_rt.esm";

    QByteArray cellPayload;
    {
        QDataStream ds(&cellPayload, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds.setFloatingPointPrecision(QDataStream::SinglePrecision);
        ds.writeRawData("EDID", 4);
        ds << quint16(10);
        ds.writeRawData("TestCell\0\0", 10);
        ds.writeRawData("FULL", 4);
        ds << quint16(8);
        ds.writeRawData("My Cell\0", 8);
    }

    QByteArray compressedPayload = makeBethesdaCompressed(cellPayload);

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // TES4 record (28 bytes data)
    out.writeRawData("TES4", 4);
    out << quint32(28);
    out << quint32(0x00000001);
    out << quint32(0);
    out << quint32(0);
    out << quint16(0);
    out << quint16(0);
    out.writeRawData("HEDR", 4);
    out << quint16(12);
    out << float(1.0f);
    out << qint32(0);
    out << quint32(0);
    out.writeRawData("CNAM", 4);
    out << quint16(4);
    out.writeRawData("Test", 4);

    // CELL record (compressed)
    out.writeRawData("CELL", 4);
    out << quint32(compressedPayload.size());
    out << quint32(0x00040000);
    out << quint32(1);
    out << quint32(0);
    out << quint16(0);
    out << quint16(0);
    out.writeRawData(compressedPayload.constData(), compressedPayload.size());

    file.close();

    ESMReader reader(filePath);
    reader.open();

    // After TES4 (consumed by open()), the next record is the CELL.
    NAME recName = reader.readName();
    QCOMPARE(recName, (NAME)'CELL');

    // readHeader should detect compression and decompress.
    RecHeader header;
    try {
        header = reader.readHeader();
    } catch (const std::exception& e) {
        qDebug() << "readHeader threw:" << e.what();
        QFAIL("readHeader should not throw on compressed CELL");
    }
    QVERIFY(header.flags.val & 0x00040000);
    QCOMPARE(header.id, (quint32)1);
    QCOMPARE(reader.recLeft(), (qint64)cellPayload.size());

    // Read the first subrecord (EDID)
    NAME sub = reader.readNSubHeader();
    QCOMPARE(sub, (NAME)'EDID');
    QCOMPARE(reader.subLeft(), (qint64)10);
    QString edid = reader.readZString();
    // readZString returns the full subrecord contents (including any embedded
    // nulls that pad the fixed-width string). Just check the meaningful prefix.
    QVERIFY(edid.startsWith("TestCell"));

    // Read the second subrecord (FULL)
    sub = reader.readNSubHeader();
    QCOMPARE(sub, (NAME)'FULL');
    QString full = reader.readZString();
    QVERIFY(full.startsWith("My Cell"));

    // The record should be fully consumed
    QCOMPARE(reader.recLeft(), (qint64)0);
}

// Simulate the exact pattern from SeydaNeen.esp's master list:
// "Starfield.esm" stored in a 36-byte MAST subrecord, with null padding.
void TestCompressedRecord::testMasterNameTrim()
{
    QString name = QStringLiteral("Starfield.esm");
    while (name.length() < 36) name.append(QChar(0));
    QCOMPARE(name.length(), 36);

    // Strip trailing control chars (the trim logic from Data::preload)
    while (!name.isEmpty() && (name.at(name.length()-1) < QChar(0x20))) {
        name.chop(1);
    }
    QCOMPARE(name, QStringLiteral("Starfield.esm"));
    QCOMPARE(name.length(), 13);
}

QTEST_MAIN(TestCompressedRecord)
#include "test_compressedrecord.moc"

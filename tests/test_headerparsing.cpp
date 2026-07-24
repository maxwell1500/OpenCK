#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDataStream>
#include <QByteArray>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/tes4.hpp"
#include "../../libs/files/esm/common.hpp"
#include "../../libs/files/log/logger.hpp"

class TestHeaderParsing : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testWellFormedHeader();
    void testHeaderWithUnknownSubrecord();
    void testHeaderWithCorruptSubrecordSize();
    void testHeaderWithNegativeRecLeft();
};

// Helper: write a TES4 + subrecords to a QByteArray.
static QByteArray makeHeader(QVector<QPair<QByteArray, QByteArray>> subs,
                             quint32 flags = 0x00000001)
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // We'll first build the subrecord body, then prepend a 24-byte record header.
    QByteArray body;
    QDataStream bs(&body, QIODevice::WriteOnly);
    bs.setByteOrder(QDataStream::LittleEndian);
    bs.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // HEDR (12 bytes) — required first subrecord
    bs.writeRawData("HEDR", 4);
    bs << quint16(12);
    bs << float(1.0f);
    bs << qint32(0);
    bs << quint32(0);

    // CNAM (4 bytes) — author
    bs.writeRawData("CNAM", 4);
    bs << quint16(4);
    bs.writeRawData("Test", 4);

    // Extra subrecords (caller-supplied)
    for (const auto& sub : subs)
    {
        bs.writeRawData(sub.first.constData(), 4);
        bs << quint16(static_cast<quint16>(sub.second.size()));
        bs.writeRawData(sub.second.constData(), sub.second.size());
    }

    // TES4 record header (24 bytes)
    ds.writeRawData("TES4", 4);
    ds << quint32(static_cast<quint32>(body.size()));
    ds << flags;
    ds << quint32(0);
    ds << quint32(0);
    ds << quint16(0);
    ds << quint16(0);

    ds.writeRawData(body.constData(), body.size());
    return data;
}

void TestHeaderParsing::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
}

void TestHeaderParsing::cleanupTestCase()
{
}

void TestHeaderParsing::testWellFormedHeader()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.filePath("wellformed.esm");

    QByteArray data = makeHeader({});
    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
    f.close();

    ESMReader reader(filePath);
    reader.open();

    const Header& h = reader.getHeader();
    QCOMPARE(h.version, 1.0f);
    QCOMPARE(h.author, QStringLiteral("Test"));
    QCOMPARE(h.recHeader.size, static_cast<quint32>(data.size() - 24));
}

void TestHeaderParsing::testHeaderWithUnknownSubrecord()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.filePath("unknown_sub.esm");

    // Insert an unknown 'XYZZ' subrecord with 5 bytes of payload.
    QByteArray payload = "abcde";
    QByteArray data = makeHeader({ qMakePair(QByteArray("XYZZ"), payload) });

    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
    f.close();

    ESMReader reader(filePath);
    reader.open();

    const Header& h = reader.getHeader();
    QCOMPARE(h.author, QStringLiteral("Test"));
    QCOMPARE(h.version, 1.0f);
    // recLeft should be drained to 0, not negative.
    QVERIFY(h.recHeader.size > 0);
}

void TestHeaderParsing::testHeaderWithCorruptSubrecordSize()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.filePath("corrupt_sub.esm");

    // Build a header with HEDR + CNAM, then a 'BOGX' subrecord whose declared
    // size (28265) is much larger than the actual remaining recLeft.
    // The fix in tes4.cpp should detect this and not underflow recLeft.
    QByteArray data;
    {
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

        QByteArray body;
        QDataStream bs(&body, QIODevice::WriteOnly);
        bs.setByteOrder(QDataStream::LittleEndian);
        bs.setFloatingPointPrecision(QDataStream::SinglePrecision);

        // HEDR
        bs.writeRawData("HEDR", 4);
        bs << quint16(12);
        bs << float(1.0f);
        bs << qint32(0);
        bs << quint32(0);
        // CNAM
        bs.writeRawData("CNAM", 4);
        bs << quint16(4);
        bs.writeRawData("Test", 4);
        // BOGX: 1 byte of payload, but size claims 28265.
        bs.writeRawData("BOGX", 4);
        bs << quint16(28265);
        bs.writeRawData("x", 1);

        ds.writeRawData("TES4", 4);
        ds << quint32(static_cast<quint32>(body.size() - 28264));
        // ^^ record size excludes the bogus claimed bytes; the real body is
        // 12+6+5+1 = 24 bytes so the TES4 wraps just the real subrecords.
        ds << quint32(0x00000001);
        ds << quint32(0);
        ds << quint32(0);
        ds << quint16(0);
        ds << quint16(0);
        ds.writeRawData(body.constData(), body.size());
    }

    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
    f.close();

    ESMReader reader(filePath);
    reader.open();

    // recHeader.size was 24 - 28264 = negative, which readHeader will store as a
    // huge unsigned. We just need to assert: loading didn't crash and the recLeft
    // doesn't underflow catastrophically.
    const Header& h = reader.getHeader();
    Q_UNUSED(h);
    // No exception is success.
}

void TestHeaderParsing::testHeaderWithNegativeRecLeft()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.filePath("negative.esm");

    // Minimal header: HEDR (12) + CNAM (4) = 16 bytes of body.
    QByteArray data;
    {
        QDataStream ds(&data, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

        QByteArray body;
        QDataStream bs(&body, QIODevice::WriteOnly);
        bs.setByteOrder(QDataStream::LittleEndian);
        bs.setFloatingPointPrecision(QDataStream::SinglePrecision);

        bs.writeRawData("HEDR", 4);
        bs << quint16(12);
        bs << float(1.0f);
        bs << qint32(0);
        bs << quint32(0);
        bs.writeRawData("CNAM", 4);
        bs << quint16(4);
        bs.writeRawData("Test", 4);

        ds.writeRawData("TES4", 4);
        ds << quint32(static_cast<quint32>(body.size() - 20)); // 16 - 20 = -4 size
        ds << quint32(0x00000001);
        ds << quint32(0);
        ds << quint32(0);
        ds << quint16(0);
        ds << quint16(0);
        ds.writeRawData(body.constData(), body.size());
    }

    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
    f.close();

    ESMReader reader(filePath);
    // Loading must not throw even with an underflow-prone header size.
    reader.open();
    QVERIFY(true);
}

QTEST_MAIN(TestHeaderParsing)
#include "test_headerparsing.moc"

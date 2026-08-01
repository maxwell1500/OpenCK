#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QImage>

#include "../../libs/files/nif/ddsencoder.hpp"
#include "../../libs/files/nif/ddsdecoder.hpp"
#include "../../libs/files/log/logger.hpp"

class TestDdsDecoder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDxt1RoundTrip();
    void testDxt5RoundTrip();
    void testUncompressed();
    void testInvalidFile();
    void testFourCC();
};

void TestDdsDecoder::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_ddsdecoder_log.txt"));
}

static QImage makeTestImage(int w, int h)
{
    QImage img(w, h, QImage::Format_ARGB32);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const QRgb c = qRgba((x * 255) / w, (y * 255) / h, ((x + y) % 256), 255);
            img.setPixel(x, y, c);
        }
    return img;
}

static void testRoundTrip(int dxtFormat)
{
    // Solid colors round-trip with small error; this validates the decoder
    // independently of the encoder's (lossy) gradient handling.
    QImage src(16, 16, QImage::Format_ARGB32);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
        {
            const QRgb c = (x / 8 == 0)
                ? qRgba(120, 60, 200, 255)   // left half
                : qRgba(30, 200, 90, 255);   // right half
            src.setPixel(x, y, c);
        }

    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    const QString path = tmpFile.fileName();
    tmpFile.close();
    QFile::remove(path);

    QVERIFY(DdsEncoder::encode(src, path, dxtFormat));

    QImage decoded = DdsDecoder::decodeFile(path);
    QVERIFY(!decoded.isNull());
    QCOMPARE(decoded.width(), src.width());
    QCOMPARE(decoded.height(), src.height());

    // Both halves must decode close to their source colors.
    const QRgb leftSrc = src.pixel(2, 2);
    const QRgb rightSrc = src.pixel(10, 10);
    const QRgb leftDec = decoded.pixel(2, 2);
    const QRgb rightDec = decoded.pixel(10, 10);

    QVERIFY2(qAbs(qRed(leftSrc) - qRed(leftDec)) <= 40
             && qAbs(qGreen(leftSrc) - qGreen(leftDec)) <= 40
             && qAbs(qBlue(leftSrc) - qBlue(leftDec)) <= 40,
             "left half decoded incorrectly");
    QVERIFY2(qAbs(qRed(rightSrc) - qRed(rightDec)) <= 40
             && qAbs(qGreen(rightSrc) - qGreen(rightDec)) <= 40
             && qAbs(qBlue(rightSrc) - qBlue(rightDec)) <= 40,
             "right half decoded incorrectly");
}

void TestDdsDecoder::testDxt1RoundTrip()
{
    testRoundTrip(1);
}

void TestDdsDecoder::testDxt5RoundTrip()
{
    testRoundTrip(5);
}

void TestDdsDecoder::testUncompressed()
{
    // Hand-craft a small uncompressed BGRA32 DDS.
    const int w = 4, h = 4;
    QByteArray dds;
    dds.append("DDS ", 4);
    quint32 header[31] = { 0 };
    // DDS_HEADER: dwSize, dwFlags, dwHeight, dwWidth, pitch, depth, mips
    header[0] = 124;
    header[1] = 0x1003;         // CAPS|HEIGHT|WIDTH|PITCH
    header[2] = h;
    header[3] = w;
    header[4] = w * 4;          // pitch
    header[7] = 0x1000;         // caps
    // DDS_PIXELFORMAT: dwSize, dwFlags, dwFourCC, dwRGBBitCount, masks
    // (header[18] = dwSize, header[19] = dwFlags, header[20] = fourCC,
    //  header[21] = bitcount, header[22..25] = R,G,B,A masks)
    header[18] = 32;            // ddpfPixelFormat.dwSize
    header[19] = 0x40 | 0x1;    // ALPHAPIXELS | RGB
    header[20] = 0;             // dwFourCC (none)
    header[21] = 32;            // dwRGBBitCount
    header[22] = 0x00FF0000;    // R
    header[23] = 0x0000FF00;    // G
    header[24] = 0x000000FF;    // B
    header[25] = 0xFF000000;    // A
    dds.append(reinterpret_cast<const char*>(header), 124);

    for (int i = 0; i < w * h; ++i)
    {
        const quint8 b = static_cast<quint8>(i * 16);
        const quint8 g = static_cast<quint8>(255 - i * 16);
        const quint8 r = static_cast<quint8>(i * 4);
        const quint8 a = 255;
        dds.append(reinterpret_cast<const char*>(&b), 1);
        dds.append(reinterpret_cast<const char*>(&g), 1);
        dds.append(reinterpret_cast<const char*>(&r), 1);
        dds.append(reinterpret_cast<const char*>(&a), 1);
    }

    const QImage decoded = DdsDecoder::decode(dds);
    QVERIFY(!decoded.isNull());
    QCOMPARE(decoded.pixel(0, 0), qRgba(0, 255, 0, 255));
    // i=15: b=15*16=240, g=255-240=15, r=15*4=60
    QCOMPARE(decoded.pixel(3, 3), qRgba(60, 15, 240, 255));
}

void TestDdsDecoder::testInvalidFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("not a dds");
    file.close();
    QVERIFY(DdsDecoder::decodeFile(file.fileName()).isNull());
    QVERIFY(DdsDecoder::decode("garbage").isNull());
}

void TestDdsDecoder::testFourCC()
{
    const QImage src = makeTestImage(8, 8);
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    const QString path = tmpFile.fileName();
    tmpFile.close();
    QFile::remove(path);
    QVERIFY(DdsEncoder::encode(src, path, 1));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray data = f.readAll();
    f.close();
    QCOMPARE(DdsDecoder::fourCC(data), QStringLiteral("DXT1"));
}

QTEST_MAIN(TestDdsDecoder)
#include "test_ddsdecoder.moc"

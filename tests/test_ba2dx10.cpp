#include <QTest>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QImage>

#include "ba2archive.hpp"
#include "ddsdecoder.hpp"
#include "logger.hpp"

// Reads real Bethesda DX10 (texture) BA2 archives: Starfield v2 (zlib) and
// v3 (LZ4). The test path comes from OPENCK_TEST_BA2_DIR; when the variable
// is empty the archive-read tests are skipped (the pure zlib/LZ4 unit tests
// still run).
class TestBa2Dx10 : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testParseSfV2Textures();
    void testParseSfV3Textures();
    void testExtractSfV2Texture();
    void testExtractSfV3Texture();
    void testCreateRoundTrip();
};

void TestBa2Dx10::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_ba2dx10_log.txt"));
}

namespace {
QString testDir()
{
    return qEnvironmentVariable("OPENCK_TEST_BA2_DIR");
}

bool isDds(const QByteArray& bytes)
{
    return bytes.size() >= 4 && bytes.mid(0, 4) == QByteArray("DDS ");
}

// Build a minimal DX10-extended-header DDS (BC3/DXT5, 4x4, 1 mip, 16 bytes payload).
QByteArray makeDds(quint32 width, quint32 height, quint32 dxgi, const QByteArray& payload)
{
    QByteArray h(148, '\0');
    h.replace(0, 4, QByteArray("DDS ", 4));
    auto put = [&](int off, quint32 v) {
        for (int i = 0; i < 4; ++i)
            h[off + i] = static_cast<char>((v >> (8 * i)) & 0xFF);
    };
    put(0x04, 124);
    put(0x08, 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000); // caps|height|width|pixelformat|linearsize
    put(0x0C, height);
    put(0x10, width);
    put(0x14, static_cast<quint32>(payload.size()));
    put(0x1C, 1);                                   // mipMapCount
    put(0x4C, 32);
    put(0x50, 0x4);                                 // DDPF_FOURCC
    h.replace(0x54, 4, QByteArray("DX10", 4));
    put(0x6C, 0x1000);                              // caps: texture
    put(0x80, dxgi);
    put(0x84, 3);                                   // TEXTURE2D
    put(0x8C, 1);                                   // arraySize
    h.append(payload);
    return h;
}

QString writeTempDds(const QString& name, const QByteArray& bytes)
{
    const QString dir = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2dx10/input");
    QDir().mkpath(dir);
    const QString path = dir + "/" + name;
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(bytes);
    f.close();
    return path;
}
}

void TestBa2Dx10::testCreateRoundTrip()
{
    const QByteArray tex0 = QByteArray("DXT5PAYLOAD0").repeated(40); // 480 bytes
    const QByteArray tex1 = QByteArray("DXT5PAYLOAD1").repeated(20); // 240 bytes
    const QString in0 = writeTempDds("texa.dds", makeDds(32, 32, 77, tex0));
    const QString in1 = writeTempDds("texb.dds", makeDds(16, 16, 77, tex1));

    const QString archive = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2dx10/roundtrip.ba2");
    QFile::remove(archive);

    Ba2Archive writer;
    QVERIFY(writer.create(QStringList() << in0 << in1, archive, /*compress=*/true, "DX10"));
    QVERIFY(QFileInfo::exists(archive));

    Ba2Archive reader;
    QVERIFY(reader.open(archive));
    QVERIFY(reader.isTexture());
    QCOMPARE(reader.fileCount(), static_cast<quint32>(2));
    const auto& entries = reader.textureEntries();
    QCOMPARE(entries.size(), 2);

    // Read back both textures and verify the payload survives zlib + DDS rebuild.
    const QString out0 = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2dx10/out_rt0.dds");
    const QString out1 = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2dx10/out_rt1.dds");
    QVERIFY(reader.extract(0, out0));
    QVERIFY(reader.extract(1, out1));

    QFile f0(out0);
    QVERIFY(f0.open(QIODevice::ReadOnly));
    QByteArray got0 = f0.readAll();
    f0.close();
    QVERIFY(isDds(got0));
    // payload sits after the 148-byte header
    QCOMPARE(got0.mid(148), tex0);

    QFile f1(out1);
    QVERIFY(f1.open(QIODevice::ReadOnly));
    QByteArray got1 = f1.readAll();
    f1.close();
    QVERIFY(isDds(got1));
    QCOMPARE(got1.mid(148), tex1);

    QFile::remove(archive);
}

void TestBa2Dx10::testParseSfV2Textures()
{
    const QString dir = testDir();
    if (dir.isEmpty())
        QSKIP("OPENCK_TEST_BA2_DIR not set");

    const QString path = dir + "/SFBGS006 - Textures.ba2";
    if (!QFileInfo::exists(path))
        QSKIP("v2 texture BA2 not found");

    Ba2Archive a;
    QVERIFY(a.open(path));
    QVERIFY(a.isTexture());
    const auto& entries = a.textureEntries();
    QVERIFY(entries.size() > 100);
    QCOMPARE(a.fileCount(), static_cast<quint32>(entries.size()));

    // All entries are .dds textures with sane metadata.
    int seenDds = 0;
    for (const auto& e : entries)
    {
        QVERIFY(e.width > 0 && e.height > 0);
        QVERIFY(e.numMips >= 1);
        QVERIFY(e.tileMode == 8);
        QVERIFY(!e.chunks.isEmpty());
        if (e.relativePath.endsWith(".dds", Qt::CaseInsensitive)) seenDds++;
    }
    QVERIFY(seenDds == entries.size());
}

void TestBa2Dx10::testParseSfV3Textures()
{
    const QString dir = testDir();
    if (dir.isEmpty())
        QSKIP("OPENCK_TEST_BA2_DIR not set");

    const QString path = dir + "/Starfield - LODTextures02.ba2";
    if (!QFileInfo::exists(path))
        QSKIP("v3 texture BA2 not found");

    Ba2Archive a;
    QVERIFY(a.open(path));
    QVERIFY(a.isTexture());
    const auto& entries = a.textureEntries();
    QVERIFY(entries.size() > 100);
    QCOMPARE(a.fileCount(), static_cast<quint32>(entries.size()));

    int seenDds = 0;
    for (const auto& e : entries)
    {
        QVERIFY(e.width > 0 && e.height > 0);
        QVERIFY(e.numMips >= 1);
        QVERIFY(e.tileMode == 8);
        QVERIFY(!e.chunks.isEmpty());
        if (e.relativePath.endsWith(".dds", Qt::CaseInsensitive)) seenDds++;
    }
    QVERIFY(seenDds == entries.size());
}

void TestBa2Dx10::testExtractSfV2Texture()
{
    const QString dir = testDir();
    if (dir.isEmpty())
        QSKIP("OPENCK_TEST_BA2_DIR not set");

    const QString path = dir + "/SFBGS006 - Textures.ba2";
    if (!QFileInfo::exists(path))
        QSKIP("v2 texture BA2 not found");

    Ba2Archive a;
    QVERIFY(a.open(path));
    const auto& entries = a.textureEntries();
    QVERIFY(!entries.isEmpty());

    const QString outDir = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2dx10/v2");
    QDir().mkpath(outDir);

    int ok = 0;
    for (quint32 i = 0; i < static_cast<quint32>(qMin(entries.size(), 20)); ++i)
    {
        const QString out = outDir + "/tex" + QString::number(i) + ".dds";
        if (!a.extract(i, out)) continue;
        QFile f(out);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QByteArray bytes = f.readAll();
        f.close();
        QVERIFY(isDds(bytes));
        // DDS size field = 124, dimensions present.
        QVERIFY(bytes.size() >= 128);
        QVERIFY(isDds(bytes.mid(0, 4)));
        // The texture must be decodable by the in-app decoder (texture preview).
        QImage img = DdsDecoder::decodeFile(out);
        QVERIFY(!img.isNull());
        QVERIFY(img.width() > 0 && img.height() > 0);
        ok++;
    }
    QVERIFY(ok > 0);
}

void TestBa2Dx10::testExtractSfV3Texture()
{
    const QString dir = testDir();
    if (dir.isEmpty())
        QSKIP("OPENCK_TEST_BA2_DIR not set");

    const QString path = dir + "/Starfield - LODTextures02.ba2";
    if (!QFileInfo::exists(path))
        QSKIP("v3 texture BA2 not found");

    Ba2Archive a;
    QVERIFY(a.open(path));
    const auto& entries = a.textureEntries();
    QVERIFY(!entries.isEmpty());

    const QString outDir = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2dx10/v3");
    QDir().mkpath(outDir);

    int ok = 0;
    for (quint32 i = 0; i < static_cast<quint32>(qMin(entries.size(), 20)); ++i)
    {
        const QString out = outDir + "/tex" + QString::number(i) + ".dds";
        if (!a.extract(i, out)) continue;
        QFile f(out);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QByteArray bytes = f.readAll();
        f.close();
        QVERIFY(isDds(bytes));
        QImage img = DdsDecoder::decodeFile(out);
        QVERIFY(!img.isNull());
        QVERIFY(img.width() > 0 && img.height() > 0);
        ok++;
    }
    QVERIFY(ok > 0);
}

QTEST_MAIN(TestBa2Dx10)
#include "test_ba2dx10.moc"
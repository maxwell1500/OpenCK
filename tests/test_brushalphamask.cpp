#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QImage>

#include "../../src/model/tools/brushalphamask.hpp"
#include "../../libs/files/nif/ddsencoder.hpp"
#include "../../libs/files/nif/ddsdecoder.hpp"
#include "../../libs/files/log/logger.hpp"

class TestBrushAlphaMask : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testIdentityWhenNoMask();
    void testOpaqueCenterOpaqueEdges();
    void testClear();
    void testLoadDdsFile();
    void testLoadInvalidFile();
};

void TestBrushAlphaMask::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_brushalphamask_log.txt"));
}

void TestBrushAlphaMask::testIdentityWhenNoMask()
{
    BrushAlphaMask mask;
    QVERIFY(!mask.isValid());
    // Without a loaded mask, painting is unaffected everywhere.
    QCOMPARE(mask.valueAt(0.0f, 0.0f), 1.0f);
    QCOMPARE(mask.valueAt(-0.8f, 0.6f), 1.0f);
    QCOMPARE(mask.width(), 0);
    QCOMPARE(mask.height(), 0);
}

void TestBrushAlphaMask::testOpaqueCenterOpaqueEdges()
{
    // A 4x4 white (opaque) mask must produce full strength everywhere.
    QImage white(4, 4, QImage::Format_ARGB32);
    white.fill(Qt::white);
    BrushAlphaMask mask;
    mask.setImage(white);
    QVERIFY(mask.isValid());
    QCOMPARE(mask.width(), 4);
    QCOMPARE(mask.height(), 4);
    QVERIFY(qAbs(mask.valueAt(0.0f, 0.0f) - 1.0f) < 0.001f);
    QVERIFY(qAbs(mask.valueAt(0.9f, -0.9f) - 1.0f) < 0.001f);
    QVERIFY(qAbs(mask.valueAt(-1.0f, 1.0f) - 1.0f) < 0.001f);
}

void TestBrushAlphaMask::testClear()
{
    QImage white(2, 2, QImage::Format_ARGB32);
    white.fill(Qt::white);
    BrushAlphaMask mask;
    mask.setImage(white);
    QVERIFY(mask.isValid());
    mask.clear();
    QVERIFY(!mask.isValid());
    QCOMPARE(mask.valueAt(0.0f, 0.0f), 1.0f);
}

void TestBrushAlphaMask::testLoadDdsFile()
{
    // Encode a real DDS (opaque white) and load it back through the decoder.
    QImage src(8, 8, QImage::Format_ARGB32);
    src.fill(Qt::white);

    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    const QString path = tmpFile.fileName() + QStringLiteral(".dds");
    tmpFile.close();
    QFile::remove(path);

    QVERIFY(DdsEncoder::encode(src, path, 5)); // DXT5

    BrushAlphaMask mask;
    QVERIFY(mask.load(path));
    QVERIFY(mask.isValid());
    QCOMPARE(mask.width(), 8);
    QCOMPARE(mask.height(), 8);
    QVERIFY(!mask.filePath().isEmpty());
    QVERIFY(qAbs(mask.valueAt(0.0f, 0.0f) - 1.0f) < 0.001f);
}

void TestBrushAlphaMask::testLoadInvalidFile()
{
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    tmpFile.write("not a dds file at all");
    const QString path = tmpFile.fileName();
    tmpFile.close();

    BrushAlphaMask mask;
    QVERIFY(!mask.load(path));
    QVERIFY(!mask.isValid());
}

QTEST_MAIN(TestBrushAlphaMask)
#include "test_brushalphamask.moc"

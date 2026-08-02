#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QImage>

#include "../../src/model/tools/terrainoverlaymask.hpp"
#include "../../libs/files/nif/ddsencoder.hpp"

class TestTerrainOverlayMask : public QObject
{
    Q_OBJECT

private slots:
    void testInvalidNoMask();
    void testOpaqueSamples();
    void testResample();
    void testLoadDds();
    void testClear();
};

void TestTerrainOverlayMask::testInvalidNoMask()
{
    TerrainOverlayMask mask;
    QVERIFY(!mask.isValid());
    QCOMPARE(mask.sample(0.5f, 0.5f), 0.0f);
    QVERIFY(mask.resample(4).isEmpty() == false);
    QVERIFY(mask.resample(4).size() == 16);
}

void TestTerrainOverlayMask::testOpaqueSamples()
{
    QImage white(4, 4, QImage::Format_ARGB32);
    white.fill(Qt::white);
    TerrainOverlayMask mask;
    mask.setImage(white);
    QVERIFY(mask.isValid());
    QCOMPARE(mask.width(), 4);
    QCOMPARE(mask.height(), 4);
    QVERIFY(qAbs(mask.sample(0.0f, 0.0f) - 1.0f) < 0.001f);
    QVERIFY(qAbs(mask.sample(1.0f, 1.0f) - 1.0f) < 0.001f);
    QVERIFY(qAbs(mask.sample(0.5f, 0.5f) - 1.0f) < 0.001f);
}

void TestTerrainOverlayMask::testResample()
{
    // A 2x2 mask: black (0) top-left, white (255) elsewhere. Resampling to
    // a 4x4 grid should show ~1 at (3,3) and ~0 at (0,0).
    QImage img(2, 2, QImage::Format_ARGB32);
    img.setPixel(0, 0, qRgba(0, 0, 0, 0));
    img.setPixel(1, 0, qRgba(255, 255, 255, 255));
    img.setPixel(0, 1, qRgba(255, 255, 255, 255));
    img.setPixel(1, 1, qRgba(255, 255, 255, 255));

    TerrainOverlayMask mask;
    mask.setImage(img);
    const QVector<float> grid = mask.resample(4);
    QCOMPARE(grid.size(), 16);

    // Corner sampling matches the nearest texel.
    QVERIFY(grid[0 * 4 + 0] < 0.1f);
    QVERIFY(grid[3 * 4 + 3] > 0.9f);
    QVERIFY(grid[2 * 4 + 2] > 0.8f);  // near the white corner, bilinearly blended
}

void TestTerrainOverlayMask::testLoadDds()
{
    QImage src(4, 4, QImage::Format_ARGB32);
    src.fill(Qt::white);

    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    const QString path = tmpFile.fileName() + QStringLiteral(".dds");
    tmpFile.close();
    QFile::remove(path);

    QVERIFY(DdsEncoder::encode(src, path, 3));

    TerrainOverlayMask mask;
    QVERIFY(mask.load(path));
    QVERIFY(mask.isValid());
    QVERIFY(!mask.filePath().isEmpty());
    QVERIFY(qAbs(mask.sample(0.5f, 0.5f) - 1.0f) < 0.001f);

    // Invalid file fails cleanly.
    TerrainOverlayMask bad;
    QVERIFY(!bad.load(QStringLiteral("Z:/missing.tif")));
}

void TestTerrainOverlayMask::testClear()
{
    QImage white(2, 2, QImage::Format_ARGB32);
    white.fill(Qt::white);
    TerrainOverlayMask mask;
    mask.setImage(white);
    QVERIFY(mask.isValid());
    mask.clear();
    QVERIFY(!mask.isValid());
    QCOMPARE(mask.sample(0.5f, 0.5f), 0.0f);
}

QTEST_MAIN(TestTerrainOverlayMask)
#include "test_terrainoverlaymask.moc"

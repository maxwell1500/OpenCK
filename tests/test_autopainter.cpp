#include <QTest>

#include "../../src/model/tools/autopainter.hpp"

class TestAutoPainter : public QObject
{
    Q_OBJECT

private slots:
    void testSlopeAtFlat();
    void testSlopeAtCliff();
    void testPaintFlatByHeight();
    void testPaintBySlope();
    void testPriority();
    void testDefaultLayers();

private:
    static QVector<float> flatMap(int size, float height)
    {
        QVector<float> map(size * size, height);
        return map;
    }
};

void TestAutoPainter::testSlopeAtFlat()
{
    QVector<float> map = flatMap(4, 10.0f);
    QVERIFY(AutoPainter::slopeAt(map, 4, 2, 2) < 0.001f);
}

void TestAutoPainter::testSlopeAtCliff()
{
    // One cell is much higher than its neighbors -> steep slope.
    QVector<float> map = flatMap(4, 0.0f);
    map[2 * 4 + 2] = 100.0f;
    const float slope = AutoPainter::slopeAt(map, 4, 2, 2);
    QVERIFY(slope > 45.0f);
    // Slope scale shrinks the angle.
    const float small = AutoPainter::slopeAt(map, 4, 2, 2, 0.1f);
    QVERIFY(small < slope);
}

void TestAutoPainter::testPaintFlatByHeight()
{
    AutoPainter::Options opts;
    opts.useSlope = false;
    opts.useHeight = true;
    opts.mapSize = 3;
    opts.heightScale = 1.0f;

    QVector<float> map = flatMap(3, 0.0f);

    QVector<AutoPaintLayer> layers;
    AutoPaintLayer water;
    water.texturePath = "W";
    water.minHeight = -100000.0f; water.maxHeight = -1.0f;
    layers.append(water);
    AutoPaintLayer ground;
    ground.texturePath = "G";
    ground.minHeight = -1.0f; ground.maxHeight = 100000.0f;
    layers.append(ground);

    const QVector<int> result = AutoPainter::paint(map, layers, opts);
    QCOMPARE(result.size(), 9);
    for (int i = 0; i < result.size(); ++i)
        QCOMPARE(result[i], 1); // ground

    // Raise the map into the water band.
    for (float& h : map) h = -10.0f;
    const QVector<int> wet = AutoPainter::paint(map, layers, opts);
    for (int i = 0; i < wet.size(); ++i)
        QCOMPARE(wet[i], 0); // water
}

void TestAutoPainter::testPaintBySlope()
{
    AutoPainter::Options opts;
    opts.useSlope = true;
    opts.useHeight = false;
    opts.mapSize = 4;
    opts.heightScale = 1.0f;

    QVector<float> map = flatMap(4, 0.0f);
    map[2 * 4 + 2] = 50.0f;

    QVector<AutoPaintLayer> layers;
    AutoPaintLayer flat;
    flat.texturePath = "F";
    flat.maxSlope = 30.0f;
    layers.append(flat);
    AutoPaintLayer steep;
    steep.texturePath = "S";
    steep.minSlope = 30.0f;
    layers.append(steep);

    const QVector<int> result = AutoPainter::paint(map, layers, opts);
    QCOMPARE(result[2 * 4 + 2], 1);  // steep cell
    QCOMPARE(result[0], 0);          // flat cell
}

void TestAutoPainter::testPriority()
{
    AutoPainter::Options opts;
    opts.useSlope = false;
    opts.useHeight = false;
    opts.mapSize = 2;

    QVector<float> map = flatMap(2, 0.0f);

    QVector<AutoPaintLayer> layers;
    AutoPaintLayer low;
    low.texturePath = "L"; low.priority = 0;
    layers.append(low);
    AutoPaintLayer high;
    high.texturePath = "H"; high.priority = 2;
    layers.append(high);
    AutoPaintLayer mid;
    mid.texturePath = "M"; mid.priority = 1;
    layers.append(mid);

    const QVector<int> result = AutoPainter::paint(map, layers, opts);
    for (int i = 0; i < result.size(); ++i)
        QCOMPARE(result[i], 1); // highest priority layer
}

void TestAutoPainter::testDefaultLayers()
{
    const QVector<AutoPaintLayer> layers = AutoPainter::defaultLayers();
    QCOMPARE(layers.size(), 4);

    AutoPainter::Options opts;
    opts.mapSize = 4;
    opts.heightScale = 1.0f;

    QVector<float> map = flatMap(4, 0.0f);
    const QVector<int> result = AutoPainter::paint(map, layers, opts);
    QCOMPARE(result.size(), 16);
    // All cells get the grass layer (priority 1) on flat terrain.
    QVERIFY(result.contains(1));
}

QTEST_MAIN(TestAutoPainter)
#include "test_autopainter.moc"

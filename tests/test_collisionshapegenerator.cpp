#include <QTest>

#include "../../src/model/tools/collisionshapegenerator.hpp"

class TestCollisionShapeGenerator : public QObject
{
    Q_OBJECT

private slots:
    void testComputeAABB();
    void testComputeAABBEmpty();
    void testConvexHullSquare();
    void testConvexHullCollinear();
    void testDecimateVertices();
    void testShapeTypeNames();
};

void TestCollisionShapeGenerator::testComputeAABB()
{
    QVector<double> vertices = {
        0, 0, 0,
        10, 0, 0,
        0, 5, 0,
        0, 0, 8,
    };
    CollisionShapeGenerator::BoxShape box;
    QVERIFY(CollisionShapeGenerator::computeAABB(vertices, box));
    QCOMPARE(box.minX, 0.0);
    QCOMPARE(box.maxX, 10.0);
    QCOMPARE(box.maxY, 5.0);
    QCOMPARE(box.maxZ, 8.0);
    QCOMPARE(box.width(), 10.0);
    QCOMPARE(box.height(), 5.0);
    QCOMPARE(box.depth(), 8.0);
}

void TestCollisionShapeGenerator::testComputeAABBEmpty()
{
    CollisionShapeGenerator::BoxShape box;
    QVERIFY(!CollisionShapeGenerator::computeAABB(QVector<double>(), box));
    QVERIFY(!CollisionShapeGenerator::computeAABB({ 1, 2 }, box));  // < 3 values
}

void TestCollisionShapeGenerator::testConvexHullSquare()
{
    QVector<QPointF> pts = {
        QPointF(0, 0), QPointF(1, 0), QPointF(1, 1), QPointF(0, 1),
        QPointF(0.5, 0.5),  // interior point must be dropped
    };
    CollisionShapeGenerator::ConvexHull2D hull;
    QVERIFY(CollisionShapeGenerator::convexHull2D(pts, hull));
    QCOMPARE(hull.points.size(), 4);
    QVERIFY(qFuzzyCompare(hull.area(), 1.0));
}

void TestCollisionShapeGenerator::testConvexHullCollinear()
{
    QVector<QPointF> pts = { QPointF(0, 0), QPointF(1, 1), QPointF(2, 2) };
    CollisionShapeGenerator::ConvexHull2D hull;
    // Collinear points don't form a 2D hull.
    QVERIFY(!CollisionShapeGenerator::convexHull2D(pts, hull));

    QVERIFY(!CollisionShapeGenerator::convexHull2D({ QPointF(0, 0) }, hull));
}

void TestCollisionShapeGenerator::testDecimateVertices()
{
    QVector<double> vertices;
    for (int i = 0; i < 12; ++i)  // 4 vertices
        vertices.append(static_cast<double>(i));

    const QVector<double> stride2 = CollisionShapeGenerator::decimateVertices(vertices, 2);
    QCOMPARE(stride2.size(), 6);  // keeps every 2nd vertex -> 2 vertices

    const QVector<double> stride1 = CollisionShapeGenerator::decimateVertices(vertices, 1);
    QCOMPARE(stride1.size(), vertices.size());
}

void TestCollisionShapeGenerator::testShapeTypeNames()
{
    QCOMPARE(CollisionShapeGenerator::shapeTypeName(
                 CollisionShapeGenerator::ShapeType::Box),
             QStringLiteral("Box"));
    QCOMPARE(CollisionShapeGenerator::shapeTypeName(
                 CollisionShapeGenerator::ShapeType::Convex),
             QStringLiteral("Convex"));
    QCOMPARE(CollisionShapeGenerator::shapeTypeName(
                 CollisionShapeGenerator::ShapeType::CompressedMesh),
             QStringLiteral("Compressed Mesh"));
}

QTEST_MAIN(TestCollisionShapeGenerator)
#include "test_collisionshapegenerator.moc"

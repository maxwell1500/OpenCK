#include <QTest>

#include "../../src/model/tools/primitivemeshgenerator.hpp"
#include "../../libs/files/log/logger.hpp"

class TestPrimitiveMeshGenerator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCube();
    void testPlane();
    void testCylinder();
    void testSphere();
    void testTriangleMultipleOf3();
};

void TestPrimitiveMeshGenerator::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_primitives_log.txt"));
}

void TestPrimitiveMeshGenerator::testCube()
{
    const auto mesh = PrimitiveMeshGenerator::generate(
        PrimitiveMeshGenerator::Type::Cube, 2.0f);
    QCOMPARE(mesh.triangleCount(), 12); // 6 faces x 2 tris
    QCOMPARE(mesh.vertices.size(), 36);

    // All vertices must lie within [-1,1]^3 for a size-2 cube.
    for (const QVector3D& v : mesh.vertices)
    {
        QVERIFY(v.x() >= -1.0f && v.x() <= 1.0f);
        QVERIFY(v.y() >= -1.0f && v.y() <= 1.0f);
        QVERIFY(v.z() >= -1.0f && v.z() <= 1.0f);
    }
}

void TestPrimitiveMeshGenerator::testPlane()
{
    const auto mesh = PrimitiveMeshGenerator::generate(
        PrimitiveMeshGenerator::Type::Plane, 4.0f);
    QCOMPARE(mesh.triangleCount(), 2);
    QCOMPARE(mesh.vertices.size(), 6);

    // Plane is in the XZ plane (y == 0).
    for (const QVector3D& v : mesh.vertices)
    {
        QCOMPARE(v.y(), 0.0f);
        QVERIFY(qFabs(v.x()) <= 2.0f && qFabs(v.z()) <= 2.0f);
    }
}

void TestPrimitiveMeshGenerator::testCylinder()
{
    const auto mesh = PrimitiveMeshGenerator::generate(
        PrimitiveMeshGenerator::Type::Cylinder, 1.0f, 16);
    // 16 side quads (96 verts) + 16 top + 16 bottom caps (96 verts)
    QCOMPARE(mesh.vertices.size(), 192);

    // Bounds: x,z within radius 1, y within [-0.5, 0.5].
    for (const QVector3D& v : mesh.vertices)
    {
        QVERIFY(v.x() >= -1.0f && v.x() <= 1.0f);
        QVERIFY(v.z() >= -1.0f && v.z() <= 1.0f);
        QVERIFY(v.y() >= -0.5f && v.y() <= 0.5f);
    }
}

void TestPrimitiveMeshGenerator::testSphere()
{
    const auto mesh = PrimitiveMeshGenerator::generate(
        PrimitiveMeshGenerator::Type::Sphere, 1.0f, 32);
    QVERIFY(mesh.triangleCount() > 100);

    // Every vertex must be within the sphere radius 1.
    for (const QVector3D& v : mesh.vertices)
    {
        QVERIFY(v.length() <= 1.001f);
    }
}

void TestPrimitiveMeshGenerator::testTriangleMultipleOf3()
{
    for (PrimitiveMeshGenerator::Type type :
        { PrimitiveMeshGenerator::Type::Cube,
          PrimitiveMeshGenerator::Type::Cylinder,
          PrimitiveMeshGenerator::Type::Plane,
          PrimitiveMeshGenerator::Type::Sphere })
    {
        const auto mesh = PrimitiveMeshGenerator::generate(type, 1.0f);
        QCOMPARE(mesh.vertices.size() % 3, 0);
    }
}

QTEST_MAIN(TestPrimitiveMeshGenerator)
#include "test_primitivemeshgenerator.moc"

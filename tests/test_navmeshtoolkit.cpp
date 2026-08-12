#include <QTest>
#include <QVector3D>

#include "../src/model/tools/navmeshtoolkit.hpp"
#include "../src/model/tools/navmeshgenerator.hpp"

using namespace NavMeshTools;

class TestNavMeshToolkit : public QObject
{
    Q_OBJECT

private slots:
    void testAdjacencySquare();
    void testAnalyzeCleanMesh();
    void testAnalyzeIssues();
    void testWeldVertices();
    void testFindConnections();
    void testRemoveTjunctions();
    void testGenerateGridFlat();
    void testGenerateGridSteepSlopeDropped();
    void testGeneratorTriangleFan();
    void testGeneratorKeepsWalkableOnly();
    void testVoxelFilterFlatFloor();
    void testVoxelFilterStaircase();
    void testVoxelFilterRamp();
    void testVoxelFilterBlockedHeadroom();
    void testVoxelFilterDegenerate();
    void testComputeCoverData();
};

void TestNavMeshToolkit::testAdjacencySquare()
{
    QVector<QVector3D> verts = {
        QVector3D(0, 0, 0),
        QVector3D(1, 0, 0),
        QVector3D(1, 1, 0),
        QVector3D(0, 1, 0)
    };
    QVector<MeshTriangle> tris = {
        { 0, 1, 2, 0 },
        { 0, 2, 3, 0 }
    };

    QVector<EdgeInfo> edges;
    QVector<QVector<int>> adj = rebuildAdjacency(verts, tris, &edges);

    QCOMPARE(adj.size(), 2);
    QVERIFY(adj[0].contains(1));
    QVERIFY(adj[1].contains(0));

    QCOMPARE(edges.size(), 5);
    int shared = 0;
    for (const auto& e : edges)
        if (e.triB != -1) ++shared;
    QCOMPARE(shared, 1);
}

void TestNavMeshToolkit::testAnalyzeCleanMesh()
{
    QVector<QVector3D> verts = {
        QVector3D(0, 0, 0),
        QVector3D(1, 0, 0),
        QVector3D(1, 1, 0),
        QVector3D(0, 1, 0)
    };
    QVector<MeshTriangle> tris = {
        { 0, 1, 2, 0 },
        { 0, 2, 3, 0 }
    };

    CheckResult result = analyze(verts, tris);
    QCOMPARE(result.issues.size(), 0);
    QCOMPARE(result.componentCount, 1);
    QCOMPARE(result.borderEdgeCount, 4);
}

void TestNavMeshToolkit::testAnalyzeIssues()
{
    QVector<QVector3D> verts = {
        QVector3D(0, 0, 0),
        QVector3D(1, 0, 0),
        QVector3D(0, 1, 0)
    };
    QVector<MeshTriangle> tris = {
        { 0, 1, 2, 0 },
        { 0, 1, 2, 0 },
        { 0, 1, 9, 0 },
        { 0, 0, 0, 0 }
    };

    CheckResult result = analyze(verts, tris);
    bool foundOutOfRange = false;
    bool foundDegenerate = false;
    bool foundIsolated = false;
    for (const auto& issue : result.issues) {
        if (issue.kind == IssueKind::OutOfRangeVertex) foundOutOfRange = true;
        if (issue.kind == IssueKind::DegenerateTriangle) foundDegenerate = true;
        if (issue.kind == IssueKind::IsolatedTriangle) foundIsolated = true;
    }
    QVERIFY(foundOutOfRange);
    QVERIFY(foundDegenerate);
    QVERIFY(foundIsolated);
    QCOMPARE(result.componentCount, 3);
}

void TestNavMeshToolkit::testWeldVertices()
{
    QVector<QVector3D> verts = {
        QVector3D(0, 0, 0),
        QVector3D(1, 0, 0),
        QVector3D(1.0001f, 0.0001f, 0),
        QVector3D(0, 1, 0)
    };
    QVector<MeshTriangle> tris = {
        { 0, 1, 3, 0 },
        { 1, 2, 3, 0 }
    };

    weldVertices(verts, tris, 0.01f);
    QCOMPARE(verts.size(), 3);
    QCOMPARE(tris[1].v1, tris[0].v1);
}

void TestNavMeshToolkit::testFindConnections()
{
    QVector<QVector3D> vertsA = {
        QVector3D(0, 0, 0),
        QVector3D(1, 0, 0),
        QVector3D(1, 1, 0),
        QVector3D(0, 1, 0)
    };
    QVector<MeshTriangle> trisA = {
        { 0, 1, 2, 0 },
        { 0, 2, 3, 0 }
    };

    QVector<QVector3D> vertsB = {
        QVector3D(1, 0, 0),
        QVector3D(2, 0, 0),
        QVector3D(2, 1, 0),
        QVector3D(1, 1, 0)
    };
    QVector<MeshTriangle> trisB = {
        { 0, 1, 2, 0 },
        { 0, 2, 3, 0 }
    };

    QVector<Connection> connections = findConnections(vertsA, trisA, vertsB, trisB, 0.5f);
    QCOMPARE(connections.size(), 1);
    QCOMPARE(connections[0].width, 1.0f);
}

void TestNavMeshToolkit::testRemoveTjunctions()
{
    QVector<QVector3D> verts = {
        QVector3D(0, 0, 0),
        QVector3D(2, 0, 0),
        QVector3D(1, 1, 0),
        QVector3D(0, 1, 0),
        QVector3D(2, 1, 0)
    };
    QVector<MeshTriangle> tris = {
        { 0, 1, 2, 0 },
        { 3, 2, 4, 0 },
        { 1, 2, 4, 0 },
        { 2, 3, 0, 0 }
    };

    removeTjunctions(verts, tris, 0.01f);
    bool midUsed = false;
    for (const auto& tri : tris) {
        for (int v : { tri.v0, tri.v1, tri.v2 }) {
            if (v >= 0 && v < verts.size() &&
                std::abs(verts[v].y()) < 1e-4f &&
                std::abs(verts[v].x() - 1.0f) < 1e-4f)
                midUsed = true;
        }
    }
    QCOMPARE(midUsed, false);
}

void TestNavMeshToolkit::testGenerateGridFlat()
{
    GridNavmeshOptions opts;
    opts.columns = 3;
    opts.rows = 3;
    opts.cellSize = 128.0f;
    opts.maxSlope = 45.0f;

    // Flat 3x3 grid -> all 8 cells split into 16 triangles, 9 vertices.
    QVector<float> heights(9, 0.0f);
    QVector<QVector3D> verts = generateGridVertices(heights, opts);
    QCOMPARE(verts.size(), 9);
    QVector<MeshTriangle> tris = generateGridTriangles(heights, verts, opts);
    QCOMPARE(tris.size(), 8);

    // No degenerate triangles, all indices in range.
    for (const auto& t : tris)
    {
        QVERIFY(t.v0 >= 0 && t.v0 < verts.size());
        QVERIFY(t.v1 >= 0 && t.v1 < verts.size());
        QVERIFY(t.v2 >= 0 && t.v2 < verts.size());
        QVERIFY(t.v0 != t.v1 && t.v1 != t.v2 && t.v0 != t.v2);
    }
}

void TestNavMeshToolkit::testGenerateGridSteepSlopeDropped()
{
    GridNavmeshOptions opts;
    opts.columns = 3;
    opts.rows = 3;
    opts.cellSize = 128.0f;
    opts.maxSlope = 45.0f; // vertical drop allowed: tan(45)*128 = 128

    // Left column flat; right column a sheer cliff (drop 256 > 128).
    QVector<float> heights = {
        0, 0, 256,
        0, 0, 256,
        0, 0, 256
    };
    QVector<QVector3D> verts = generateGridVertices(heights, opts);
    QCOMPARE(verts.size(), 9);

    QVector<MeshTriangle> tris = generateGridTriangles(heights, verts, opts);
    // Cells spanning the cliff drop are dropped; the flat 2x2 left block
    // keeps 2 cells -> 4 triangles.
    QVERIFY(tris.size() >= 2);
    QVERIFY(tris.size() <= 8);
    for (const auto& t : tris)
    {
        const float maxH = qMax(qMax(verts[t.v0].z(), verts[t.v1].z()), verts[t.v2].z());
        const float minH = qMin(qMin(verts[t.v0].z(), verts[t.v1].z()), verts[t.v2].z());
        QVERIFY(maxH - minH <= 128.0f + 1e-4f);
    }
}

void TestNavMeshToolkit::testGeneratorTriangleFan()
{
    // Flat ground fan: center plus 4 rim vertices, 4 triangles all lying on
    // the ground plane (y = 0). The voxel filter must keep every triangle.
    NavMeshGenerator generator;
    const QVector<QVector3D> verts = {
        QVector3D(0, 0, 0),     // center
        QVector3D(1, 0, 0),
        QVector3D(1, 0, 1),
        QVector3D(-1, 0, 1),
        QVector3D(-1, 0, -1)
    };
    const QVector<unsigned int> indices = {
        0, 2, 1,
        0, 3, 2,
        0, 4, 3,
        0, 1, 4
    };

    const NavMeshGenerator::NavMesh mesh = generator.generateFromVertices(verts, indices);

    QVERIFY(!mesh.triangles.isEmpty());
    QVERIFY(mesh.vertices.size() >= 4 && mesh.vertices.size() <= 8);
    QCOMPARE(mesh.triangles.size(), 4);
    QCOMPARE(mesh.vertices.size(), 5);
    // Adjacent fan triangles share two vertices -> 4 shared edges.
    QCOMPARE(mesh.edges.size(), 4);
    for (const auto& tri : mesh.triangles)
        QVERIFY(tri.normal.y() > 0.9f);
}

void TestNavMeshToolkit::testGeneratorKeepsWalkableOnly()
{
    // Fan of four flat triangles plus one vertical wall (90 degree slope).
    // The wall triangle must be rejected by the walkable filter.
    NavMeshGenerator generator;
    const QVector<QVector3D> verts = {
        QVector3D(0, 0, 0),     // center
        QVector3D(1, 0, 0),
        QVector3D(1, 0, 1),
        QVector3D(-1, 0, 1),
        QVector3D(-1, 0, -1),
        QVector3D(0, 2, -1),    // wall top
        QVector3D(0, 2, 0)      // wall top
    };
    const QVector<unsigned int> indices = {
        0, 2, 1,
        0, 3, 2,
        0, 4, 3,
        0, 1, 4,
        1, 5, 6   // vertical wall: normal is horizontal, slope 90 degrees
    };

    const NavMeshGenerator::NavMesh mesh = generator.generateFromVertices(verts, indices);

    QVERIFY(!mesh.triangles.isEmpty());
    QCOMPARE(mesh.triangles.size(), 4);
    for (const auto& tri : mesh.triangles)
    {
        // Only triangles with an upward normal survive the filter.
        QVERIFY(tri.normal.y() > 0.9f);
    }
}

void TestNavMeshToolkit::testVoxelFilterFlatFloor()
{
    // Flat 2x2-cell floor at y=0 on a shared 3x3 vertex grid. cellSize =
    // 2*agentRadius = 72, so the four cells (0,0),(1,0),(0,1),(1,1) must all
    // be walkable and all eight triangles kept. This is the acceptance signal
    // for the voxel filter.
    NavMeshGenerator generator;
    QVector<QVector3D> verts;
    QVector<unsigned int> indices;
    const float cs = 72.0f;
    for (int z = 0; z < 3; ++z)
        for (int x = 0; x < 3; ++x)
            verts.append(QVector3D(x * cs, 0, z * cs));
    auto idx = [](int x, int z) { return unsigned(z * 3 + x); };
    for (int z = 0; z < 2; ++z) {
        for (int x = 0; x < 2; ++x) {
            const unsigned int tl = idx(x, z);
            const unsigned int tr = idx(x + 1, z);
            const unsigned int bl = idx(x, z + 1);
            const unsigned int br = idx(x + 1, z + 1);
            indices << tl << br << tr
                    << tl << bl << br;
        }
    }

    const NavMeshGenerator::NavMesh mesh = generator.generateFromVertices(verts, indices);

    QCOMPARE(mesh.vertices.size(), 9);
    QCOMPARE(mesh.triangles.size(), 8);
    QCOMPARE(mesh.cells.size(), 4);
    QVERIFY(mesh.cells.contains(qMakePair(0, 0)));
    QVERIFY(mesh.cells.contains(qMakePair(1, 0)));
    QVERIFY(mesh.cells.contains(qMakePair(0, 1)));
    QVERIFY(mesh.cells.contains(qMakePair(1, 1)));
}

void TestNavMeshToolkit::testVoxelFilterStaircase()
{
    // Three 24-unit steps (well under stepHeight 48) plus vertical risers.
    // Every tread must stay walkable; the risers are non-walkable and must
    // not block the tread cells they border.
    NavMeshGenerator generator;
    QVector<QVector3D> verts;
    QVector<unsigned int> indices;
    const float cs = 72.0f;
    const float rise = 24.0f;
    for (int k = 0; k < 3; ++k) {
        const float z0 = k * cs;
        const float y = k * rise;
        const unsigned int base = verts.size();
        verts << QVector3D(0, y, z0)
              << QVector3D(cs, y, z0)
              << QVector3D(cs, y, z0 + cs)
              << QVector3D(0, y, z0 + cs);
        indices << base << base + 2 << base + 1
                << base << base + 3 << base + 2;
        if (k < 2) {
            const unsigned int rb = verts.size();
            verts << QVector3D(0, y, z0 + cs)
                  << QVector3D(cs, y, z0 + cs)
                  << QVector3D(cs, y + rise, z0 + cs)
                  << QVector3D(0, y + rise, z0 + cs);
            indices << rb << rb + 1 << rb + 2
                    << rb << rb + 2 << rb + 3;
        }
    }

    const NavMeshGenerator::NavMesh mesh = generator.generateFromVertices(verts, indices);

    QCOMPARE(mesh.triangles.size(), 6);
    QCOMPARE(mesh.cells.size(), 3);
    QVERIFY(mesh.cells.contains(qMakePair(0, 0)));
    QVERIFY(mesh.cells.contains(qMakePair(0, 1)));
    QVERIFY(mesh.cells.contains(qMakePair(0, 2)));
}

void TestNavMeshToolkit::testVoxelFilterRamp()
{
    // A 30-degree ramp (rise 41.57 over a 72 run) is walkable; a 60-degree
    // ramp (rise 124.7) exceeds maxSlope 45 and must be dropped entirely.
    NavMeshGenerator generator;

    QVector<QVector3D> verts;
    QVector<unsigned int> indices;
    const float cs = 72.0f;
    const unsigned int base = verts.size();
    verts << QVector3D(0, 0, 0)          // p0: low x
          << QVector3D(cs, 41.57f, 0)    // p1: high x
          << QVector3D(cs, 41.57f, cs)   // p2: high x, high z
          << QVector3D(0, 0, cs);        // p3: low x, high z
    indices << base + 1 << base + 0 << base + 3
            << base + 2 << base + 1 << base + 3;

    NavMeshGenerator::NavMesh mesh = generator.generateFromVertices(verts, indices);
    QCOMPARE(mesh.triangles.size(), 2);
    QCOMPARE(mesh.cells.size(), 1);
    QVERIFY(mesh.cells.contains(qMakePair(0, 0)));

    QVector<QVector3D> steepVerts;
    QVector<unsigned int> steepIndices;
    const unsigned int sb = steepVerts.size();
    steepVerts << QVector3D(0, 0, 0)
               << QVector3D(cs, 124.71f, 0)
               << QVector3D(cs, 124.71f, cs)
               << QVector3D(0, 0, cs);
    steepIndices << sb + 1 << sb + 0 << sb + 3
                 << sb + 2 << sb + 1 << sb + 3;

    mesh = generator.generateFromVertices(steepVerts, steepIndices);
    QCOMPARE(mesh.triangles.size(), 0);
    QCOMPARE(mesh.cells.size(), 0);
}

void TestNavMeshToolkit::testVoxelFilterBlockedHeadroom()
{
    // A ceiling 100 units over a floor (between stepHeight 48 and
    // agentHeight 176) blocks the cell; a ceiling 300 units up does not.
    NavMeshGenerator generator;
    const float cs = 72.0f;

    auto floorWithCeiling = [&](float ceilY) {
        QVector<QVector3D> verts;
        QVector<unsigned int> indices;
        const unsigned int fb = verts.size();
        verts << QVector3D(0, 0, 0)
              << QVector3D(cs, 0, 0)
              << QVector3D(cs, 0, cs)
              << QVector3D(0, 0, cs);
        indices << fb << fb + 2 << fb + 1
                << fb << fb + 3 << fb + 2;
        const unsigned int cb = verts.size();
        verts << QVector3D(0, ceilY, 0)
              << QVector3D(cs, ceilY, 0)
              << QVector3D(cs, ceilY, cs)
              << QVector3D(0, ceilY, cs);
        // Ceiling wound downward so its normal points down (non-walkable).
        indices << cb << cb + 1 << cb + 2
                << cb << cb + 2 << cb + 3;
        return generator.generateFromVertices(verts, indices);
    };

    // Low ceiling: headroom 100 < agentHeight 176 -> cell blocked.
    NavMeshGenerator::NavMesh mesh = floorWithCeiling(100.0f);
    QCOMPARE(mesh.triangles.size(), 0);
    QCOMPARE(mesh.cells.size(), 0);

    // High ceiling: headroom 300 >= agentHeight -> cell stays walkable.
    mesh = floorWithCeiling(300.0f);
    QCOMPARE(mesh.triangles.size(), 2);
    QCOMPARE(mesh.cells.size(), 1);
    QVERIFY(mesh.cells.contains(qMakePair(0, 0)));
}

void TestNavMeshToolkit::testVoxelFilterDegenerate()
{
    NavMeshGenerator generator;

    // Degenerate triangle (two identical vertices) plus a valid floor quad:
    // the degenerate face is skipped, the floor survives.
    QVector<QVector3D> verts;
    QVector<unsigned int> indices;
    const float cs = 72.0f;
    const unsigned int fb = verts.size();
    verts << QVector3D(0, 0, 0)
          << QVector3D(cs, 0, 0)
          << QVector3D(cs, 0, cs)
          << QVector3D(0, 0, cs);
    indices << fb << fb + 2 << fb + 1
            << fb << fb + 3 << fb + 2;
    const unsigned int db = verts.size();
    verts << QVector3D(10, 0, 10)
          << QVector3D(10, 0, 10)
          << QVector3D(20, 0, 10);
    indices << db << db + 1 << db + 2;

    NavMeshGenerator::NavMesh mesh = generator.generateFromVertices(verts, indices);
    QCOMPARE(mesh.triangles.size(), 2);
    QCOMPARE(mesh.cells.size(), 1);

    // Empty input must produce an empty mesh without crashing.
    mesh = generator.generateFromVertices({}, {});
    QVERIFY(mesh.triangles.isEmpty());
    QVERIFY(mesh.vertices.isEmpty());
}

void TestNavMeshToolkit::testComputeCoverData()
{
    // A mesh with a rising wall: vertex 2 towers 400 units above its
    // neighbors (a sheer cliff face).
    QVector<QVector3D> verts = {
        QVector3D(0, 0, 0),
        QVector3D(0, 1, 0),     // within radius
        QVector3D(0, 1, 400),   // wall top
        QVector3D(1, 1, 0)
    };
    QVector<MeshTriangle> tris = {
        { 0, 1, 3, 0 },
        { 1, 2, 3, 0 },
        { 0, 3, 2, 0 }
    };

    QVector<CoverData> covers = computeCoverData(verts, tris, 512.0f, 128.0f);
    QCOMPARE(covers.size(), 4);

    // Vertex 1 sees vertex 2 rise 400 units vertically above it: high cover.
    QVERIFY(covers[1].flags & Cover_High_N);
    // Vertex 0 sees the 400-unit wall across the mesh: low cover.
    QVERIFY(covers[0].flags & Cover_Low_N);
    // Vertex 3 (same base height) also has low cover from the wall.
    QVERIFY(covers[3].flags & Cover_Low_N);
}

QTEST_MAIN(TestNavMeshToolkit)
#include "test_navmeshtoolkit.moc"
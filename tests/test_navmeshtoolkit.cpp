#include <QTest>
#include <QVector3D>

#include "../src/model/tools/navmeshtoolkit.hpp"

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

QTEST_MAIN(TestNavMeshToolkit)
#include "test_navmeshtoolkit.moc"

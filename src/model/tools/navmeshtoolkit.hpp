#ifndef NAVMESHTOOLKIT_HPP
#define NAVMESHTOOLKIT_HPP

#include <QVector>
#include <QString>
#include <QtGui/QVector3D>

namespace NavMeshTools {

struct MeshTriangle
{
    int v0, v1, v2;
    quint8 flags = 0;
};

inline bool operator==(const MeshTriangle& a, const MeshTriangle& b)
{
    return a.v0 == b.v0 && a.v1 == b.v1 && a.v2 == b.v2 && a.flags == b.flags;
}

inline bool operator!=(const MeshTriangle& a, const MeshTriangle& b)
{
    return !(a == b);
}

enum class IssueKind
{
    OutOfRangeVertex,
    DegenerateTriangle,
    DuplicateVertex,
    IsolatedTriangle,
    NonManifoldEdge,
    Tjunction
};

struct Issue
{
    IssueKind kind;
    int index;
    QString detail;
};

struct EdgeInfo
{
    int a = -1;
    int b = -1;
    int triA = -1;
    int triB = -1;
};

struct Connection
{
    int triA = -1;
    int edgeA = -1;
    int triB = -1;
    int edgeB = -1;
    float width = 0.0f;
};

struct CheckResult
{
    QVector<Issue> issues;
    QVector<EdgeInfo> edges;
    int componentCount = 0;
    int borderEdgeCount = 0;
};

CheckResult analyze(const QVector<QVector3D>& vertices,
                    const QVector<MeshTriangle>& triangles,
                    float weldEpsilon = 0.01f);

QVector<QVector<int>> rebuildAdjacency(const QVector<QVector3D>& vertices,
                                       const QVector<MeshTriangle>& triangles,
                                       QVector<EdgeInfo>* edges = nullptr);

QVector<Connection> findConnections(const QVector<QVector3D>& verticesA,
                                    const QVector<MeshTriangle>& trianglesA,
                                    const QVector<QVector3D>& verticesB,
                                    const QVector<MeshTriangle>& trianglesB,
                                    float epsilon = 1.0f);

void weldVertices(QVector<QVector3D>& vertices, QVector<MeshTriangle>& triangles,
                  float epsilon = 0.01f);

void removeTjunctions(QVector<QVector3D>& vertices, QVector<MeshTriangle>& triangles,
                      float epsilon = 1.0f);

// ─── Auto-generation from a height grid ────────────────────────────────────
//
// Builds a navmesh from a rectangular height field, the shape the landscape
// editor already works with. Each grid cell is split into two triangles;
// triangles that would be degenerate (two corners equal) or whose centroid
// slopes more steeply than maxSlope are dropped. Vertices are world-space
// (x = column * cellSize, z = height).
struct GridNavmeshOptions
{
    int columns = 0;      // samples per row
    int rows = 0;         // sample rows
    float cellSize = 256.0f;
    float maxSlope = 45.0f; // degrees; triangles steeper are discarded
    float originX = 0.0f;
    float originY = 0.0f;
};

QVector<QVector3D> generateGridVertices(const QVector<float>& heightGrid,
                                        const GridNavmeshOptions& options);

QVector<MeshTriangle> generateGridTriangles(const QVector<float>& heightGrid,
                                            const QVector<QVector3D>& vertices,
                                            const GridNavmeshOptions& options);

// ─── Cover data ────────────────────────────────────────────────────────────
//
// Navmesh cover marks a vertex as providing low/high cover to one or more
// directions based on the surrounding terrain depression / barrier slope.
// This is the NVCV payload of a NAVM record.
enum CoverFlag : quint8
{
    Cover_None    = 0,
    Cover_Low_N   = 0x01,
    Cover_Low_S   = 0x02,
    Cover_Low_E   = 0x04,
    Cover_Low_W   = 0x08,
    Cover_High_N  = 0x10,
    Cover_High_S  = 0x20,
    Cover_High_E  = 0x40,
    Cover_High_W  = 0x80
};

struct CoverData
{
    quint8 flags = 0;
    QVector<QVector3D> coverPoints; // for preview rendering
};

// Compute per-vertex cover flags from a triangle mesh. A vertex qualifies for
// low cover along a direction when a neighbor vertex within radius is at
// least minCoverDepth lower; high cover when the drop is at least
// minCoverDepth * 2 and the direction is along a nearly vertical barrier
// (steep neighbor slope). Returns one CoverData per vertex.
QVector<CoverData> computeCoverData(const QVector<QVector3D>& vertices,
                                    const QVector<MeshTriangle>& triangles,
                                    float radius = 512.0f,
                                    float minCoverDepth = 128.0f);

} // namespace NavMeshTools

#endif // NAVMESHTOOLKIT_HPP

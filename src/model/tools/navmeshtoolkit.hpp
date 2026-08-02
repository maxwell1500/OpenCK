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

} // namespace NavMeshTools

#endif // NAVMESHTOOLKIT_HPP

#include "navmeshtoolkit.hpp"

#include <QMap>
#include <cmath>
#include <algorithm>

namespace NavMeshTools {

namespace {

struct EdgeKey
{
    int a;
    int b;

    friend bool operator==(const EdgeKey& l, const EdgeKey& r)
    {
        return l.a == r.a && l.b == r.b;
    }

    friend bool operator<(const EdgeKey& l, const EdgeKey& r)
    {
        return l.a < r.a || (l.a == r.a && l.b < r.b);
    }
};

QVector<QPair<int,int>> triangleEdges(const MeshTriangle& tri)
{
    return {
        { tri.v0, tri.v1 },
        { tri.v1, tri.v2 },
        { tri.v2, tri.v0 }
    };
}

float triangleArea(const QVector3D& a, const QVector3D& b, const QVector3D& c)
{
    return QVector3D::crossProduct(b - a, c - a).length() * 0.5f;
}

bool sameEdge(const QVector3D& ax, const QVector3D& ay,
              const QVector3D& bx, const QVector3D& by,
              float epsilon)
{
    return ((ax - bx).lengthSquared() < epsilon * epsilon &&
            (ay - by).lengthSquared() < epsilon * epsilon) ||
           ((ax - by).lengthSquared() < epsilon * epsilon &&
            (ay - bx).lengthSquared() < epsilon * epsilon);
}

struct EdgeRef
{
    int tri;
    int edge;
};

} // namespace

CheckResult analyze(const QVector<QVector3D>& vertices,
                    const QVector<MeshTriangle>& triangles,
                    float weldEpsilon)
{
    CheckResult result;

    QVector<EdgeInfo> adjacencyEdges;
    QVector<QVector<int>> adjacency =
        rebuildAdjacency(vertices, triangles, &adjacencyEdges);

    for (int t = 0; t < triangles.size(); ++t) {
        const auto& tri = triangles[t];

        if (tri.v0 < 0 || tri.v0 >= vertices.size() ||
            tri.v1 < 0 || tri.v1 >= vertices.size() ||
            tri.v2 < 0 || tri.v2 >= vertices.size()) {
            result.issues.append({ IssueKind::OutOfRangeVertex, t,
                QStringLiteral("Triangle %1 references a vertex outside the vertex list").arg(t) });
            continue;
        }

        float area = triangleArea(vertices[tri.v0], vertices[tri.v1], vertices[tri.v2]);
        if (area < 1e-4f) {
            result.issues.append({ IssueKind::DegenerateTriangle, t,
                QStringLiteral("Triangle %1 has near-zero area (%2)").arg(t).arg(area) });
        }
    }

    for (int i = 0; i < vertices.size(); ++i) {
        for (int j = i + 1; j < vertices.size(); ++j) {
            if ((vertices[i] - vertices[j]).lengthSquared() < weldEpsilon * weldEpsilon) {
                result.issues.append({ IssueKind::DuplicateVertex, i,
                    QStringLiteral("Vertices %1 and %2 coincide within %3 units")
                        .arg(i).arg(j).arg(weldEpsilon) });
                break;
            }
        }
    }

    for (int t = 0; t < adjacency.size(); ++t) {
        if (adjacency[t].isEmpty()) {
            result.issues.append({ IssueKind::IsolatedTriangle, t,
                QStringLiteral("Triangle %1 has no adjacent triangles").arg(t) });
        }
    }

    QMap<EdgeKey, int> edgeOccurrences;
    for (const auto& tri : triangles) {
        if (tri.v0 < 0 || tri.v0 >= vertices.size() ||
            tri.v1 < 0 || tri.v1 >= vertices.size() ||
            tri.v2 < 0 || tri.v2 >= vertices.size())
            continue;
        const auto edges = triangleEdges(tri);
        for (const auto& e : edges) {
            EdgeKey key{ std::min(e.first, e.second), std::max(e.first, e.second) };
            ++edgeOccurrences[key];
        }
    }
    for (int t = 0; t < triangles.size(); ++t) {
        const auto& tri = triangles[t];
        if (tri.v0 < 0 || tri.v0 >= vertices.size() ||
            tri.v1 < 0 || tri.v1 >= vertices.size() ||
            tri.v2 < 0 || tri.v2 >= vertices.size())
            continue;
        const auto edges = triangleEdges(tri);
        for (const auto& e : edges) {
            EdgeKey key{ std::min(e.first, e.second), std::max(e.first, e.second) };
            if (edgeOccurrences[key] > 2) {
                result.issues.append({ IssueKind::NonManifoldEdge, t,
                    QStringLiteral("Edge (%1,%2) is shared by more than two triangles")
                        .arg(key.a).arg(key.b) });
            }
        }
    }

    for (const auto& edge : adjacencyEdges) {
        if (edge.triB == -1) ++result.borderEdgeCount;
    }

    QVector<int> compId(triangles.size(), -1);
    int nextComp = 0;
    for (int t = 0; t < triangles.size(); ++t) {
        if (compId[t] != -1) continue;
        QVector<int> stack;
        stack.append(t);
        compId[t] = nextComp;
        while (!stack.isEmpty()) {
            int cur = stack.takeLast();
            for (int n : adjacency[cur]) {
                if (compId[n] == -1) {
                    compId[n] = nextComp;
                    stack.append(n);
                }
            }
        }
        ++nextComp;
    }
    result.componentCount = nextComp;

    return result;
}

QVector<QVector<int>> rebuildAdjacency(const QVector<QVector3D>& vertices,
                                       const QVector<MeshTriangle>& triangles,
                                       QVector<EdgeInfo>* edges)
{
    QVector<QVector<int>> adjacency(triangles.size());
    if (edges) edges->clear();

    QMap<EdgeKey, EdgeRef> edgeOwner;
    for (int t = 0; t < triangles.size(); ++t) {
        const auto& tri = triangles[t];
        if (tri.v0 < 0 || tri.v0 >= vertices.size() ||
            tri.v1 < 0 || tri.v1 >= vertices.size() ||
            tri.v2 < 0 || tri.v2 >= vertices.size())
            continue;
        const auto triEdges = triangleEdges(tri);
        for (int e = 0; e < 3; ++e) {
            EdgeKey key{ std::min(triEdges[e].first, triEdges[e].second),
                         std::max(triEdges[e].first, triEdges[e].second) };
            auto it = edgeOwner.find(key);
            if (it == edgeOwner.end()) {
                edgeOwner.insert(key, { t, e });
            } else {
                EdgeRef& first = it.value();
                if (first.tri != t) {
                    if (!adjacency[first.tri].contains(t)) {
                        adjacency[first.tri].append(t);
                        adjacency[t].append(first.tri);
                    }
                }
            }
        }
    }

    if (edges) {
        for (auto it = edgeOwner.constBegin(); it != edgeOwner.constEnd(); ++it) {
            EdgeInfo info;
            info.a = it.key().a;
            info.b = it.key().b;
            info.triA = it.value().tri;
            info.triB = -1;
            for (int t = 0; t < triangles.size(); ++t) {
                const auto& tri = triangles[t];
                if (t == info.triA)
                    continue;
                if (tri.v0 < 0 || tri.v0 >= vertices.size() ||
                    tri.v1 < 0 || tri.v1 >= vertices.size() ||
                    tri.v2 < 0 || tri.v2 >= vertices.size())
                    continue;
                const auto triEdges = triangleEdges(tri);
                for (int e = 0; e < 3; ++e) {
                    if (std::min(triEdges[e].first, triEdges[e].second) == it.key().a &&
                        std::max(triEdges[e].first, triEdges[e].second) == it.key().b) {
                        info.triB = t;
                        break;
                    }
                }
                if (info.triB != -1) break;
            }
            edges->append(info);
        }
    }

    return adjacency;
}

QVector<Connection> findConnections(const QVector<QVector3D>& verticesA,
                                    const QVector<MeshTriangle>& trianglesA,
                                    const QVector<QVector3D>& verticesB,
                                    const QVector<MeshTriangle>& trianglesB,
                                    float epsilon)
{
    QVector<EdgeInfo> edgesA;
    QVector<EdgeInfo> edgesB;
    rebuildAdjacency(verticesA, trianglesA, &edgesA);
    rebuildAdjacency(verticesB, trianglesB, &edgesB);

    struct BorderEdge
    {
        int a;
        int b;
        int tri;
    };

    QVector<BorderEdge> bordersA;
    QVector<BorderEdge> bordersB;

    for (const auto& e : edgesA) {
        if (e.triB != -1) continue;
        if (e.a < 0 || e.a >= verticesA.size() ||
            e.b < 0 || e.b >= verticesA.size())
            continue;
        bordersA.append({ e.a, e.b, e.triA });
    }
    for (const auto& e : edgesB) {
        if (e.triB != -1) continue;
        if (e.a < 0 || e.a >= verticesB.size() ||
            e.b < 0 || e.b >= verticesB.size())
            continue;
        bordersB.append({ e.a, e.b, e.triA });
    }

    QVector<Connection> connections;
    for (const auto& ea : bordersA) {
        for (const auto& eb : bordersB) {
            if (sameEdge(verticesA[ea.a], verticesA[ea.b],
                         verticesB[eb.a], verticesB[eb.b], epsilon)) {
                Connection conn;
                conn.triA = ea.tri;
                conn.triB = eb.tri;
                conn.width = (verticesA[ea.a] - verticesA[ea.b]).length();
                const auto triAEdges = triangleEdges(trianglesA[ea.tri]);
                const auto triBEdges = triangleEdges(trianglesB[eb.tri]);
                for (int i = 0; i < 3; ++i) {
                    if ((triAEdges[i].first == ea.a && triAEdges[i].second == ea.b) ||
                        (triAEdges[i].first == ea.b && triAEdges[i].second == ea.a))
                        conn.edgeA = i;
                    if ((triBEdges[i].first == eb.a && triBEdges[i].second == eb.b) ||
                        (triBEdges[i].first == eb.b && triBEdges[i].second == eb.a))
                        conn.edgeB = i;
                }
                connections.append(conn);
                break;
            }
        }
    }

    return connections;
}

void weldVertices(QVector<QVector3D>& vertices, QVector<MeshTriangle>& triangles,
                  float epsilon)
{
    QVector<int> remap(vertices.size());
    QVector<QVector3D> unique;
    for (int i = 0; i < vertices.size(); ++i) {
        int best = -1;
        for (int j = 0; j < unique.size(); ++j) {
            if ((unique[j] - vertices[i]).lengthSquared() < epsilon * epsilon) {
                best = j;
                break;
            }
        }
        if (best == -1) {
            best = unique.size();
            unique.append(vertices[i]);
        }
        remap[i] = best;
    }
    vertices = unique;
    for (auto& tri : triangles) {
        if (tri.v0 >= 0 && tri.v0 < remap.size()) tri.v0 = remap[tri.v0];
        if (tri.v1 >= 0 && tri.v1 < remap.size()) tri.v1 = remap[tri.v1];
        if (tri.v2 >= 0 && tri.v2 < remap.size()) tri.v2 = remap[tri.v2];
    }
}

void removeTjunctions(QVector<QVector3D>& vertices, QVector<MeshTriangle>& triangles,
                      float epsilon)
{
    bool changed = true;
    while (changed) {
        changed = false;
        for (int t = 0; t < triangles.size(); ++t) {
            auto& tri = triangles[t];
            if (tri.v0 < 0 || tri.v0 >= vertices.size() ||
                tri.v1 < 0 || tri.v1 >= vertices.size() ||
                tri.v2 < 0 || tri.v2 >= vertices.size())
                continue;

            const auto edges = triangleEdges(tri);
            for (int e = 0; e < 3; ++e) {
                const int ea = edges[e].first;
                const int eb = edges[e].second;
                const QVector3D& eaPos = vertices[ea];
                const QVector3D& ebPos = vertices[eb];
                float lenSq = (ebPos - eaPos).lengthSquared();
                if (lenSq < 1e-8f) continue;

                for (int j = 0; j < triangles.size(); ++j) {
                    if (j == t) continue;
                    auto& other = triangles[j];
                    if (other.v0 < 0 || other.v0 >= vertices.size() ||
                        other.v1 < 0 || other.v1 >= vertices.size() ||
                        other.v2 < 0 || other.v2 >= vertices.size())
                        continue;
                    if (other.v0 == ea || other.v0 == eb ||
                        other.v1 == ea || other.v1 == eb ||
                        other.v2 == ea || other.v2 == eb)
                        continue;

                    for (int ov : { other.v0, other.v1, other.v2 }) {
                        const QVector3D& p = vertices[ov];
                        float tProj = QVector3D::dotProduct(p - eaPos, ebPos - eaPos) / lenSq;
                        if (tProj <= 1e-4f || tProj >= 1.0f - 1e-4f)
                            continue;
                        QVector3D closest = eaPos + (ebPos - eaPos) * tProj;
                        if ((p - closest).lengthSquared() > epsilon * epsilon)
                            continue;

                        int& slot = (ea == tri.v0) ? tri.v0 : (ea == tri.v1) ? tri.v1 : tri.v2;
                        if (ea != tri.v0 && ea != tri.v1 && ea != tri.v2)
                            continue;
                        slot = ov;
                        changed = true;
                        break;
                    }
                    if (changed) break;
                }
                if (changed) break;
            }
            if (changed) break;
        }
    }

    weldVertices(vertices, triangles, epsilon);
}

} // namespace NavMeshTools

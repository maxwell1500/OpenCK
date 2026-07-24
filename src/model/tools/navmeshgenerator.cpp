#include "navmeshgenerator.hpp"
#include "nifparser.hpp"

#include <cmath>
#include <limits>
#include <algorithm>

static constexpr float PI = 3.14159265358979323846f;

static void collectShapes(const Nif::Node* node, QVector<const Nif::TriShape*>& out)
{
    if (!node) return;
    for (const auto& shape : node->shapes) {
        out.append(&shape);
    }
    for (const auto* child : node->children) {
        collectShapes(child, out);
    }
}

static bool verticesEqual(const QVector3D& a, const QVector3D& b, float epsilon = 0.01f)
{
    return (a - b).lengthSquared() < epsilon * epsilon;
}

NavMeshGenerator::NavMesh NavMeshGenerator::generate(const Nif::NifParser& parser)
{
    auto verts = extractVertices(parser);
    auto indices = extractIndices(parser);
    return generateFromVertices(verts, indices);
}

QVector<QVector3D> NavMeshGenerator::extractVertices(const Nif::NifParser& parser)
{
    QVector<QVector3D> vertices;
    const Nif::Node* root = parser.getRoot();
    if (!root) return vertices;

    QVector<const Nif::TriShape*> shapes;
    collectShapes(root, shapes);

    for (const auto* shape : shapes) {
        for (const auto& v : shape->vertices) {
            vertices.append(QVector3D(v.x, v.y, v.z));
        }
    }

    return vertices;
}

QVector<unsigned int> NavMeshGenerator::extractIndices(const Nif::NifParser& parser)
{
    QVector<unsigned int> indices;
    const Nif::Node* root = parser.getRoot();
    if (!root) return indices;

    QVector<const Nif::TriShape*> shapes;
    collectShapes(root, shapes);

    unsigned int vertexOffset = 0;
    for (const auto* shape : shapes) {
        for (auto idx : shape->indices) {
            indices.append(idx + vertexOffset);
        }
        vertexOffset += static_cast<unsigned int>(shape->vertices.size());
    }

    return indices;
}

NavMeshGenerator::NavMesh NavMeshGenerator::generateFromVertices(
    const QVector<QVector3D>& verts,
    const QVector<unsigned int>& indices)
{
    NavMesh mesh;

    if (verts.isEmpty() || indices.size() < 3) return mesh;

    mesh.vertices = verts;

    QVector<NavTriangle> triangles;
    for (int i = 0; i + 2 < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        if (i0 >= static_cast<unsigned int>(verts.size()) ||
            i1 >= static_cast<unsigned int>(verts.size()) ||
            i2 >= static_cast<unsigned int>(verts.size())) {
            continue;
        }

        NavTriangle tri;
        tri.v0 = verts[i0];
        tri.v1 = verts[i1];
        tri.v2 = verts[i2];

        QVector3D edge1 = tri.v1 - tri.v0;
        QVector3D edge2 = tri.v2 - tri.v0;
        tri.normal = QVector3D::crossProduct(edge1, edge2).normalized();
        tri.center = (tri.v0 + tri.v1 + tri.v2) / 3.0f;

        triangles.append(tri);
    }

    triangles = voxelFilter(triangles);

    for (int i = 0; i < triangles.size(); ++i) {
        for (int j = i + 1; j < triangles.size(); ++j) {
            int sharedCount = 0;
            QVector3D vi[3] = { triangles[i].v0, triangles[i].v1, triangles[i].v2 };
            QVector3D vj[3] = { triangles[j].v0, triangles[j].v1, triangles[j].v2 };

            for (int a = 0; a < 3 && sharedCount < 2; ++a) {
                for (int b = 0; b < 3 && sharedCount < 2; ++b) {
                    if (verticesEqual(vi[a], vj[b])) {
                        sharedCount++;
                    }
                }
            }

            if (sharedCount == 2) {
                mesh.edges.append(qMakePair(i, j));
            }
        }
    }

    mesh.triangles = triangles;
    return mesh;
}

QVector<NavMeshGenerator::NavTriangle> NavMeshGenerator::voxelFilter(
    const QVector<NavTriangle>& triangles)
{
    if (triangles.isEmpty()) return {};

    float cellSize = agentRadius * 2.0f;

    float minX = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& tri : triangles) {
        for (const auto* v : {&tri.v0, &tri.v1, &tri.v2}) {
            if (v->x() < minX) minX = v->x();
            if (v->z() < minZ) minZ = v->z();
            if (v->x() > maxX) maxX = v->x();
            if (v->z() > maxZ) maxZ = v->z();
        }
    }

    int gridMinX = static_cast<int>(std::floor(minX / cellSize));
    int gridMinZ = static_cast<int>(std::floor(minZ / cellSize));
    int gridMaxX = static_cast<int>(std::floor(maxX / cellSize));
    int gridMaxZ = static_cast<int>(std::floor(maxZ / cellSize));

    int gridW = gridMaxX - gridMinX + 1;
    int gridH = gridMaxZ - gridMinZ + 1;

    if (gridW <= 0 || gridH <= 0) return triangles;

    QVector<bool> cellWalkable(gridW * gridH, false);

    for (const auto& tri : triangles) {
        if (!isWalkable(tri)) continue;

        int triMinX = static_cast<int>(std::floor(
            std::min({tri.v0.x(), tri.v1.x(), tri.v2.x()}) / cellSize)) - gridMinX;
        int triMinZ = static_cast<int>(std::floor(
            std::min({tri.v0.z(), tri.v1.z(), tri.v2.z()}) / cellSize)) - gridMinZ;
        int triMaxX = static_cast<int>(std::floor(
            std::max({tri.v0.x(), tri.v1.x(), tri.v2.x()}) / cellSize)) - gridMinX;
        int triMaxZ = static_cast<int>(std::floor(
            std::max({tri.v0.z(), tri.v1.z(), tri.v2.z()}) / cellSize)) - gridMinZ;

        triMinX = std::max(0, triMinX);
        triMinZ = std::max(0, triMinZ);
        triMaxX = std::min(gridW - 1, triMaxX);
        triMaxZ = std::min(gridH - 1, triMaxZ);

        for (int z = triMinZ; z <= triMaxZ; ++z) {
            for (int x = triMinX; x <= triMaxX; ++x) {
                cellWalkable[z * gridW + x] = true;
            }
        }
    }

    QVector<NavTriangle> result;
    for (const auto& tri : triangles) {
        if (!isWalkable(tri)) continue;

        int triMinX = static_cast<int>(std::floor(
            std::min({tri.v0.x(), tri.v1.x(), tri.v2.x()}) / cellSize)) - gridMinX;
        int triMinZ = static_cast<int>(std::floor(
            std::min({tri.v0.z(), tri.v1.z(), tri.v2.z()}) / cellSize)) - gridMinZ;
        int triMaxX = static_cast<int>(std::floor(
            std::max({tri.v0.x(), tri.v1.x(), tri.v2.x()}) / cellSize)) - gridMinX;
        int triMaxZ = static_cast<int>(std::floor(
            std::max({tri.v0.z(), tri.v1.z(), tri.v2.z()}) / cellSize)) - gridMinZ;

        triMinX = std::max(0, triMinX);
        triMinZ = std::max(0, triMinZ);
        triMaxX = std::min(gridW - 1, triMaxX);
        triMaxZ = std::min(gridH - 1, triMaxZ);

        bool inWalkableCell = false;
        for (int z = triMinZ; z <= triMaxZ && !inWalkableCell; ++z) {
            for (int x = triMinX; x <= triMaxX && !inWalkableCell; ++x) {
                if (cellWalkable[z * gridW + x]) {
                    inWalkableCell = true;
                }
            }
        }

        if (inWalkableCell) {
            result.append(tri);
        }
    }

    return result;
}

bool NavMeshGenerator::isWalkable(const NavTriangle& tri) const
{
    float slope = computeSlope(tri.normal);
    if (slope > maxSlope) return false;

    float avgY = (tri.v0.y() + tri.v1.y() + tri.v2.y()) / 3.0f;
    if (avgY < -1000.0f) return false;

    return true;
}

float NavMeshGenerator::computeSlope(const QVector3D& normal) const
{
    QVector3D up(0.0f, 1.0f, 0.0f);
    float dot = QVector3D::dotProduct(normal.normalized(), up);
    dot = qBound(-1.0f, dot, 1.0f);
    return std::acos(dot) * 180.0f / PI;
}

#include "collisionshapegenerator.hpp"

#include <algorithm>
#include <cmath>

double CollisionShapeGenerator::ConvexHull2D::area() const
{
    // Shoelace formula over counter-clockwise points.
    double a = 0.0;
    for (int i = 0; i < points.size(); ++i)
    {
        const QPointF& p = points[i];
        const QPointF& q = points[(i + 1) % points.size()];
        a += p.x() * q.y() - q.x() * p.y();
    }
    return std::fabs(a) * 0.5;
}

bool CollisionShapeGenerator::computeAABB(const QVector<double>& vertices,
                                          BoxShape& out)
{
    if (vertices.size() < 3)
        return false;

    out.minX = out.maxX = vertices[0];
    out.minY = out.maxY = vertices[1];
    out.minZ = out.maxZ = vertices[2];
    for (int i = 3; i + 2 < vertices.size(); i += 3)
    {
        out.minX = qMin(out.minX, vertices[i]);
        out.maxX = qMax(out.maxX, vertices[i]);
        out.minY = qMin(out.minY, vertices[i + 1]);
        out.maxY = qMax(out.maxY, vertices[i + 1]);
        out.minZ = qMin(out.minZ, vertices[i + 2]);
        out.maxZ = qMax(out.maxZ, vertices[i + 2]);
    }
    return true;
}

namespace {

int cross(const QPointF& o, const QPointF& a, const QPointF& b)
{
    const double v = (a.x() - o.x()) * (b.y() - o.y())
        - (a.y() - o.y()) * (b.x() - o.x());
    if (v > 1e-9) return 1;
    if (v < -1e-9) return -1;
    return 0;
}

} // namespace

bool CollisionShapeGenerator::convexHull2D(const QVector<QPointF>& points,
                                           ConvexHull2D& out)
{
    if (points.size() < 3)
        return false;

    QVector<QPointF> sorted = points;
    std::sort(sorted.begin(), sorted.end(), [](const QPointF& a, const QPointF& b) {
        return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
    });

    QVector<QPointF> hull;
    hull.reserve(sorted.size() * 2);

    // Lower hull.
    for (const QPointF& p : sorted)
    {
        while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull.back(), p) <= 0)
            hull.pop_back();
        hull.append(p);
    }
    // Upper hull.
    const int lowerSize = hull.size();
    for (int i = sorted.size() - 2; i >= 0; --i)
    {
        const QPointF& p = sorted[i];
        while (hull.size() > lowerSize
               && cross(hull[hull.size() - 2], hull.back(), p) <= 0)
            hull.pop_back();
        hull.append(p);
    }
    hull.pop_back();  // remove the duplicated first point

    if (hull.size() < 3)
        return false;

    out.points = hull;
    return true;
}

QVector<double> CollisionShapeGenerator::decimateVertices(
    const QVector<double>& vertices, int stride)
{
    QVector<double> out;
    if (stride <= 1)
        return vertices;
    for (int i = 0; i + 2 < vertices.size(); i += 3)
    {
        if ((i / 3) % stride != 0)
            continue;
        out.append(vertices[i]);
        out.append(vertices[i + 1]);
        out.append(vertices[i + 2]);
    }
    return out;
}

QString CollisionShapeGenerator::shapeTypeName(ShapeType type)
{
    switch (type)
    {
    case ShapeType::Box: return QStringLiteral("Box");
    case ShapeType::Convex: return QStringLiteral("Convex");
    case ShapeType::CompressedMesh: return QStringLiteral("Compressed Mesh");
    }
    return QStringLiteral("Unknown");
}

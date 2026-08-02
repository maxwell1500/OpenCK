#ifndef COLLISIONSHAPEGENERATOR_H
#define COLLISIONSHAPEGENERATOR_H

#include <QVector>
#include <QString>
#include <QPointF>

// CollisionShapeGenerator defines the physics-collision shapes OpenCK can
// generate for a mesh (mirroring what the real Creation Kit produces via its
// Havok hknp pipeline and the Morrowind project's custom hknp encoders):
//
//   - A box shape (AABB, axis-aligned bounding box)
//   - A convex hull (convex vertex set)
//   - A compressed-mesh shape (triangle soup with optional simplification)
//
// The generator computes the shapes from a triangle mesh; the actual hknp
// binary encoding is a separate concern. The convex-hull computation here is
// a 2D projection-based hull (used for the footprint) plus a 3D AABB.
struct CollisionShapeGenerator
{
    struct Triangle
    {
        QVector<QPointF> vertices;  // 3D projected; x/y are plane coords
    };

    struct BoxShape
    {
        double minX = 0, minY = 0, minZ = 0;
        double maxX = 0, maxY = 0, maxZ = 0;
        double width() const { return maxX - minX; }
        double height() const { return maxY - minY; }
        double depth() const { return maxZ - minZ; }
    };

    struct ConvexHull2D
    {
        QVector<QPointF> points;  // counter-clockwise, no collinear dupes
        double area() const;
    };

    // Computes the axis-aligned bounding box of the given vertices (x,y,z
    // interleaved as triples). Returns false on empty input.
    static bool computeAABB(const QVector<double>& vertices, BoxShape& out);

    // Computes the 2D convex hull of a point set (Andrew's monotone chain),
    // counter-clockwise, no collinear interior points. Returns false when
    // there are fewer than 3 distinct points.
    static bool convexHull2D(const QVector<QPointF>& points, ConvexHull2D& out);

    // Simple vertex decimation for a compressed-mesh shape: keeps every
    // 'stride'th vertex. Used to reduce collision mesh complexity.
    static QVector<double> decimateVertices(const QVector<double>& vertices,
                                            int stride);

    // The hknp shape types this generator can emit.
    enum class ShapeType { Box, Convex, CompressedMesh };
    static QString shapeTypeName(ShapeType type);
};

#endif // COLLISIONSHAPEGENERATOR_H

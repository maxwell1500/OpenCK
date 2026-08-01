#ifndef PRIMITIVEMESHGENERATOR_HPP
#define PRIMITIVEMESHGENERATOR_HPP

#include <QVector>
#include <QVector3D>

// Generates triangle mesh data for the standard preview primitives the
// Creation Kit uses as placeholder geometry. Output is a flat interleaved
// triangle list (positions only, one QVector3D per corner) so the caller
// can upload it directly to a VBO with GL_TRIANGLES.
struct PrimitiveMeshGenerator
{
    enum class Type { Cube, Cylinder, Plane, Sphere };

    struct Mesh
    {
        QVector<QVector3D> vertices;   // 3 per triangle
        int triangleCount() const { return vertices.size() / 3; }
    };

    // Generates the given primitive. size controls the overall extent:
    //   Cube:     edge length = size
    //   Cylinder: radius = size, height = size (sides = segments)
    //   Plane:    square of side = size, in the XZ plane
    //   Sphere:   radius = size (latitude/longitude segments)
    static Mesh generate(Type type, float size, int segments = 32);
};

#endif // PRIMITIVEMESHGENERATOR_HPP

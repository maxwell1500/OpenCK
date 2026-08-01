#include "primitivemeshgenerator.hpp"

#include <QtGlobal>
#include <cmath>

namespace {

void addQuad(QVector<QVector3D>& verts, const QVector3D& a, const QVector3D& b,
             const QVector3D& c, const QVector3D& d)
{
    verts.append(a);
    verts.append(b);
    verts.append(c);
    verts.append(a);
    verts.append(c);
    verts.append(d);
}

} // namespace

PrimitiveMeshGenerator::Mesh PrimitiveMeshGenerator::generate(Type type, float size, int segments)
{
    Mesh mesh;
    const int seg = qMax(3, segments);

    switch (type)
    {
    case Type::Cube:
    {
        const float h = size * 0.5f;
        const QVector3D c[8] = {
            QVector3D(-h, -h, -h), QVector3D(h, -h, -h),
            QVector3D(h, h, -h),   QVector3D(-h, h, -h),
            QVector3D(-h, -h, h),  QVector3D(h, -h, h),
            QVector3D(h, h, h),    QVector3D(-h, h, h)
        };
        // -Z, +Z, -X, +X, -Y, +Y
        addQuad(mesh.vertices, c[0], c[3], c[2], c[1]);
        addQuad(mesh.vertices, c[4], c[5], c[6], c[7]);
        addQuad(mesh.vertices, c[0], c[1], c[5], c[4]);
        addQuad(mesh.vertices, c[3], c[7], c[6], c[2]);
        addQuad(mesh.vertices, c[0], c[4], c[7], c[3]);
        addQuad(mesh.vertices, c[1], c[2], c[6], c[5]);
        break;
    }
    case Type::Cylinder:
    {
        const float r = size;
        const float halfH = size * 0.5f;
        const float twoPi = static_cast<float>(2.0 * M_PI);

        // Side wall
        for (int i = 0; i < seg; ++i)
        {
            const float a0 = twoPi * i / seg;
            const float a1 = twoPi * (i + 1) / seg;
            const QVector3D p0(cosf(a0) * r, -halfH, sinf(a0) * r);
            const QVector3D p1(cosf(a1) * r, -halfH, sinf(a1) * r);
            const QVector3D p2(cosf(a1) * r, halfH, sinf(a1) * r);
            const QVector3D p3(cosf(a0) * r, halfH, sinf(a0) * r);
            addQuad(mesh.vertices, p0, p1, p2, p3);
        }
        // Caps (fan from center)
        const QVector3D topCenter(0, halfH, 0);
        const QVector3D botCenter(0, -halfH, 0);
        for (int i = 0; i < seg; ++i)
        {
            const float a0 = twoPi * i / seg;
            const float a1 = twoPi * (i + 1) / seg;
            const QVector3D t0(cosf(a0) * r, halfH, sinf(a0) * r);
            const QVector3D t1(cosf(a1) * r, halfH, sinf(a1) * r);
            const QVector3D b0(cosf(a0) * r, -halfH, sinf(a0) * r);
            const QVector3D b1(cosf(a1) * r, -halfH, sinf(a1) * r);
            mesh.vertices.append(topCenter); mesh.vertices.append(t1); mesh.vertices.append(t0);
            mesh.vertices.append(botCenter); mesh.vertices.append(b0); mesh.vertices.append(b1);
        }
        break;
    }
    case Type::Plane:
    {
        const float h = size * 0.5f;
        addQuad(mesh.vertices,
            QVector3D(-h, 0, -h), QVector3D(h, 0, -h),
            QVector3D(h, 0, h), QVector3D(-h, 0, h));
        break;
    }
    case Type::Sphere:
    {
        const float r = size;
        const int lat = qMax(4, seg / 2);
        const int lon = seg;
        for (int i = 0; i < lat; ++i)
        {
            const float phi0 = static_cast<float>(M_PI) * i / lat;
            const float phi1 = static_cast<float>(M_PI) * (i + 1) / lat;
            for (int j = 0; j < lon; ++j)
            {
                const float theta0 = static_cast<float>(2.0 * M_PI) * j / lon;
                const float theta1 = static_cast<float>(2.0 * M_PI) * (j + 1) / lon;
                const QVector3D p00(sinf(phi0) * cosf(theta0) * r, cosf(phi0) * r, sinf(phi0) * sinf(theta0) * r);
                const QVector3D p10(sinf(phi1) * cosf(theta0) * r, cosf(phi1) * r, sinf(phi1) * sinf(theta0) * r);
                const QVector3D p11(sinf(phi1) * cosf(theta1) * r, cosf(phi1) * r, sinf(phi1) * sinf(theta1) * r);
                const QVector3D p01(sinf(phi0) * cosf(theta1) * r, cosf(phi0) * r, sinf(phi0) * sinf(theta1) * r);
                mesh.vertices.append(p00); mesh.vertices.append(p10); mesh.vertices.append(p11);
                mesh.vertices.append(p00); mesh.vertices.append(p11); mesh.vertices.append(p01);
            }
        }
        break;
    }
    }
    return mesh;
}

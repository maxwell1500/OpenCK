#ifndef NAVMESHGENERATOR_HPP
#define NAVMESHGENERATOR_HPP

#include <QVector>
#include <QPair>
#include <QVector3D>

namespace Nif { class NifParser; }

class NavMeshGenerator
{
public:
    struct NavTriangle
    {
        QVector3D v0, v1, v2;
        QVector3D center;
        QVector3D normal;
    };

    struct NavMesh
    {
        QVector<NavTriangle> triangles;
        QVector<QVector3D> vertices;
        QVector<QPair<int,int>> edges;
    };

    NavMesh generate(const Nif::NifParser& parser);
    NavMesh generateFromVertices(const QVector<QVector3D>& verts,
                                  const QVector<unsigned int>& indices);

private:
    float agentHeight = 176.0f;
    float agentRadius = 36.0f;
    float stepHeight = 48.0f;
    float maxSlope = 45.0f;

    QVector<NavTriangle> voxelFilter(const QVector<NavTriangle>& triangles);
    bool isWalkable(const NavTriangle& tri) const;
    float computeSlope(const QVector3D& normal) const;
    QVector<QVector3D> extractVertices(const Nif::NifParser& parser);
    QVector<unsigned int> extractIndices(const Nif::NifParser& parser);
};

#endif // NAVMESHGENERATOR_HPP

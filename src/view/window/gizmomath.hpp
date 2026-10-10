#ifndef GIZMOMATH_HPP
#define GIZMOMATH_HPP

#include <QMatrix4x4>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QVector3D>

namespace gizmo {

struct ViewTransform
{
    QMatrix4x4 view;
    QMatrix4x4 model;
    QMatrix4x4 proj;
    QSize viewport;

    QMatrix4x4 modelView() const
    {
        return view * model;
    }
};

// Convert a widget screen point (Y-down, top-left origin) into world-space
// coordinates on the plane parallel to the view plane that passes through
// refPoint. refPoint is in world space.
QVector3D screenToWorld(const ViewTransform& t, const QPointF& screenPos,
                        const QVector3D& refPoint);

// Convert a world-space point to widget screen coords (Y-down). z() of result
// is the NDC-ish depth (in [0,1] when visible).
QVector3D worldToScreen(const ViewTransform& t, const QVector3D& worldPos);

// Screen-space distance in pixels from screenPos to the projected segment
// origin..(origin + axisDir*length). Returns -1 if the segment's projected
// length is degenerate (< 1e-6 px).
float axisPickDistance(const ViewTransform& t, const QPointF& screenPos,
                       const QVector3D& origin, const QVector3D& axisDir, float length);

// Signed world-space delta along axisDir for a mouse drag of screenDelta
// pixels. Uses screenToWorld at the origin's depth; worldDelta is projected
// onto axisDir via dot product.
float dragDeltaAlongAxis(const ViewTransform& t, const QVector3D& origin,
                         const QVector3D& axisDir, const QPointF& screenDelta);

// Signed rotation angle in degrees from an arc-ball drag around the screen
// projection of origin. startScreen/currentScreen are widget coords (Y-down).
// Positive angle = counter-clockwise on screen (right-hand around view axis).
float arcballRotation(const ViewTransform& t, const QVector3D& origin,
                      const QPointF& startScreen, const QPointF& currentScreen);

// World-space length that corresponds to screenPixels on screen at the
// current zoom (so gizmos keep a constant screen size).
float worldSizeForPixels(const ViewTransform& t, float screenPixels);

// Snap helpers.
float snapToStep(float value, double step);
float snapDegrees(float degrees, int increment);
// Screen-space ray through a widget point, returned as world-space origin and
// direction. Direction is the view-plane-normal ray, long enough to reach any
// visible placed reference.
struct PickRay
{
    QVector3D origin;
    QVector3D direction;
};
PickRay pickRay(const ViewTransform& t, const QPointF& screenPos, float length = 1.0e6f);

// Slab-test ray against an axis-aligned box. Returns the entry distance along
// the ray, or -1 when there is no intersection.
float rayAabbDistance(const QVector3D& rayOrigin, const QVector3D& rayDir,
                      const QVector3D& boxMin, const QVector3D& boxMax);

// Ray against an oriented box given by center, half extents, and a rotation
// matrix mapping box-local axes to world axes. Returns entry distance or -1.
float rayObbDistance(const QVector3D& rayOrigin, const QVector3D& rayDir,
                     const QVector3D& boxCenter, const QVector3D& halfExtents,
                     const QMatrix4x4& rotation);

// Möller-Trumbore ray/triangle intersection. Returns distance t along rayDir (>= 0),
// or -1.0f on miss or parallel ray.
float rayTriangleDistance(const QVector3D& rayOrigin, const QVector3D& rayDir,
                          const QVector3D& v0, const QVector3D& v1, const QVector3D& v2);

// Raycast against a 33x33 heightmap grid spanning a 4096x4096 exterior cell at (cellOriginX, cellOriginY)
// with baseHeight + heightData[row][col] * 8.0f elevation.
// Returns distance along rayDir or -1.0f on miss. If outHit is non-null, populates hit world coordinates.
float rayTerrainDistance(const QVector3D& rayOrigin, const QVector3D& rayDir,
                         float cellOriginX, float cellOriginY,
                         float baseHeight, const qint8 heightData[33][33],
                         QVector3D* outHit = nullptr);
} // namespace gizmo

#endif // GIZMOMATH_HPP

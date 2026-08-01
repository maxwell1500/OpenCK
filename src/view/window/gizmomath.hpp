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

} // namespace gizmo

#endif // GIZMOMATH_HPP

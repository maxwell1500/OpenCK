#include "gizmomath.hpp"

#include <QtGlobal>
#include <cmath>

namespace gizmo {

namespace {

constexpr float kPi = 3.14159265358979323846f;

QRect viewportRect(const ViewTransform& t)
{
    return QRect(QPoint(0, 0), t.viewport);
}

QVector3D screenXy(const QVector3D& v)
{
    return QVector3D(v.x(), v.y(), 0.0f);
}

} // namespace

QVector3D screenToWorld(const ViewTransform& t, const QPointF& screenPos,
                        const QVector3D& refPoint)
{
    QMatrix4x4 modelView = t.modelView();
    float depth = (modelView * refPoint).z();
    float ndcX = 2.0f * static_cast<float>(screenPos.x()) / static_cast<float>(t.viewport.width()) - 1.0f;
    float ndcY = 1.0f - 2.0f * static_cast<float>(screenPos.y()) / static_cast<float>(t.viewport.height());

    bool invertible = false;
    QMatrix4x4 inverse = modelView.inverted(&invertible);
    return inverse.map(QVector3D(ndcX, ndcY, depth));
}

QVector3D worldToScreen(const ViewTransform& t, const QVector3D& worldPos)
{
    QVector3D projected = worldPos.project(t.modelView(), t.proj, viewportRect(t));
    return QVector3D(projected.x(), static_cast<float>(t.viewport.height()) - projected.y(), projected.z());
}

float axisPickDistance(const ViewTransform& t, const QPointF& screenPos,
                       const QVector3D& origin, const QVector3D& axisDir, float length)
{
    QVector3D a = screenXy(worldToScreen(t, origin));
    QVector3D b = screenXy(worldToScreen(t, origin + axisDir * length));
    QVector3D ab = b - a;
    float abLen = ab.length();
    if (abLen < 1e-6f)
        return -1.0f;

    QVector3D p(static_cast<float>(screenPos.x()), static_cast<float>(screenPos.y()), 0.0f);
    QVector3D ap = p - a;
    float along = QVector3D::dotProduct(ap, ab) / abLen;
    along = qBound(0.0f, along, abLen);

    QVector3D closest = a + ab * (along / abLen);
    return (p - closest).length();
}

float dragDeltaAlongAxis(const ViewTransform& t, const QVector3D& origin,
                         const QVector3D& axisDir, const QPointF& screenDelta)
{
    QPointF start = worldToScreen(t, origin).toPointF();
    QPointF current = start + screenDelta;

    QVector3D worldStart = screenToWorld(t, start, origin);
    QVector3D worldCurrent = screenToWorld(t, current, origin);
    QVector3D worldDelta = worldCurrent - worldStart;
    return QVector3D::dotProduct(worldDelta, axisDir.normalized());
}

float arcballRotation(const ViewTransform& t, const QVector3D& origin,
                      const QPointF& startScreen, const QPointF& currentScreen)
{
    QVector3D center = worldToScreen(t, origin);
    float v1x = static_cast<float>(startScreen.x()) - center.x();
    float v1y = static_cast<float>(startScreen.y()) - center.y();
    float v2x = static_cast<float>(currentScreen.x()) - center.x();
    float v2y = static_cast<float>(currentScreen.y()) - center.y();

    if (std::sqrt(v1x * v1x + v1y * v1y) < 1e-4f || std::sqrt(v2x * v2x + v2y * v2y) < 1e-4f)
        return 0.0f;

    float cross = v1x * v2y - v1y * v2x;
    float dot = v1x * v2x + v1y * v2y;
    return std::atan2(cross, dot) * 180.0f / kPi;
}

float worldSizeForPixels(const ViewTransform& t, float screenPixels)
{
    float zoom = t.view.column(0).toVector3D().length();
    if (zoom < 1e-4f)
        zoom = 1e-4f;
    float modelScale = t.model(0, 0);
    if (modelScale == 0.0f)
        modelScale = 1.0f;

    float worldPerPx = 2.0f / (zoom * modelScale * static_cast<float>(t.viewport.width()));
    return screenPixels * worldPerPx;
}

float snapToStep(float value, double step)
{
    if (step <= 0.0)
        return value;
    return static_cast<float>(std::round(value / step) * step);
}

float snapDegrees(float degrees, int increment)
{
    return static_cast<float>(std::round(static_cast<double>(degrees) / increment) * increment);
}

} // namespace gizmo

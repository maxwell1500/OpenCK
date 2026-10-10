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


PickRay pickRay(const ViewTransform& t, const QPointF& screenPos, float length)
{
    QMatrix4x4 modelView = t.modelView();
    bool projOk = false;
    QMatrix4x4 invProj = t.proj.inverted(&projOk);
    if (!projOk || length <= 0.0f)
        return { QVector3D(), QVector3D(0.0f, 0.0f, 1.0f) };

    float ndcX = 2.0f * static_cast<float>(screenPos.x()) / static_cast<float>(t.viewport.width()) - 1.0f;
    float ndcY = 1.0f - 2.0f * static_cast<float>(screenPos.y()) / static_cast<float>(t.viewport.height());
    QVector4D nearClip = invProj * QVector4D(ndcX, ndcY, -1.0f, 1.0f);
    QVector4D farClip = invProj * QVector4D(ndcX, ndcY, 1.0f, 1.0f);
    if (qFuzzyIsNull(nearClip.w()) || qFuzzyIsNull(farClip.w()))
        return { QVector3D(), QVector3D(0.0f, 0.0f, 1.0f) };
    nearClip /= nearClip.w();
    farClip /= farClip.w();

    bool viewOk = false;
    QMatrix4x4 invView = modelView.inverted(&viewOk);
    if (!viewOk)
        return { QVector3D(), QVector3D(0.0f, 0.0f, 1.0f) };

    QVector4D worldOrigin = invView * nearClip;
    QVector4D worldTarget = invView * farClip;
    QVector3D direction = (worldTarget.toVector3D() - worldOrigin.toVector3D()).normalized();
    return { worldOrigin.toVector3D(), direction };
}

float rayAabbDistance(const QVector3D& rayOrigin, const QVector3D& rayDir,
                      const QVector3D& boxMin, const QVector3D& boxMax)
{
    float tMin = 0.0f;
    float tMax = 1.0e12f;
    for (int axis = 0; axis < 3; ++axis)
    {
        const float origin = rayOrigin[axis];
        const float dir = rayDir[axis];
        const float lo = boxMin[axis];
        const float hi = boxMax[axis];
        if (qFuzzyIsNull(dir))
        {
            if (origin < lo || origin > hi)
                return -1.0f;
            continue;
        }
        float inv = 1.0f / dir;
        float t0 = (lo - origin) * inv;
        float t1 = (hi - origin) * inv;
        if (t0 > t1)
            qSwap(t0, t1);
        tMin = qMax(tMin, t0);
        tMax = qMin(tMax, t1);
        if (tMax < tMin)
            return -1.0f;
    }
    return tMin;
}

float rayObbDistance(const QVector3D& rayOrigin, const QVector3D& rayDir,
                     const QVector3D& boxCenter, const QVector3D& halfExtents,
                     const QMatrix4x4& rotation)
{
    QVector3D localOrigin = rotation * (rayOrigin - boxCenter);
    QVector3D localDir = rotation * rayDir;
    return rayAabbDistance(localOrigin, localDir, -halfExtents, halfExtents);
}

float rayTriangleDistance(const QVector3D& rayOrigin, const QVector3D& rayDir,
                          const QVector3D& v0, const QVector3D& v1, const QVector3D& v2)
{
    const float EPSILON = 1e-7f;
    QVector3D edge1 = v1 - v0;
    QVector3D edge2 = v2 - v0;
    QVector3D h = QVector3D::crossProduct(rayDir, edge2);
    float a = QVector3D::dotProduct(edge1, h);
    if (qAbs(a) < EPSILON)
        return -1.0f; // ray is parallel to triangle

    float f = 1.0f / a;
    QVector3D s = rayOrigin - v0;
    float u = f * QVector3D::dotProduct(s, h);
    if (u < 0.0f || u > 1.0f)
        return -1.0f;

    QVector3D q = QVector3D::crossProduct(s, edge1);
    float v = f * QVector3D::dotProduct(rayDir, q);
    if (v < 0.0f || u + v > 1.0f)
        return -1.0f;

    float t = f * QVector3D::dotProduct(edge2, q);
    if (t < 0.0f)
        return -1.0f;

    return t;
}

float rayTerrainDistance(const QVector3D& rayOrigin, const QVector3D& rayDir,
                         float cellOriginX, float cellOriginY,
                         float baseHeight, const qint8 heightData[33][33],
                         QVector3D* outHit)
{
    const float cellSize = 4096.0f;
    const float cellHalf = 2048.0f;
    const float minX = cellOriginX - cellHalf;
    const float minY = cellOriginY - cellHalf;
    const int N = 33;
    const float step = cellSize / static_cast<float>(N - 1);

    // Pre-calculate min and max elevation to form AABB for early exit
    float minZ = baseHeight - 128.0f * 8.0f;
    float maxZ = baseHeight + 127.0f * 8.0f;
    if (heightData)
    {
        minZ = 1e9f;
        maxZ = -1e9f;
        for (int r = 0; r < N; ++r)
        {
            for (int c = 0; c < N; ++c)
            {
                float h = baseHeight + static_cast<float>(heightData[r][c]) * 8.0f;
                if (h < minZ) minZ = h;
                if (h > maxZ) maxZ = h;
            }
        }
        minZ -= 1.0f;
        maxZ += 1.0f;
    }

    float aabbDist = rayAabbDistance(rayOrigin, rayDir,
                                     QVector3D(minX, minY, minZ),
                                     QVector3D(minX + cellSize, minY + cellSize, maxZ));
    if (aabbDist < 0.0f)
        return -1.0f;

    float closestT = -1.0f;

    // Helper lambda to fetch vertex in world space
    auto getVertex = [&](int r, int c) -> QVector3D {
        float x = minX + static_cast<float>(c) * step;
        float y = minY + static_cast<float>(r) * step;
        float z = baseHeight;
        if (heightData)
            z += static_cast<float>(heightData[r][c]) * 8.0f;
        return QVector3D(x, y, z);
    };

    for (int r = 0; r < N - 1; ++r)
    {
        for (int c = 0; c < N - 1; ++c)
        {
            QVector3D v00 = getVertex(r, c);
            QVector3D v01 = getVertex(r, c + 1);
            QVector3D v10 = getVertex(r + 1, c);
            QVector3D v11 = getVertex(r + 1, c + 1);

            // Quad split into two triangles: (v00, v10, v01) and (v01, v10, v11)
            float t1 = rayTriangleDistance(rayOrigin, rayDir, v00, v10, v01);
            if (t1 >= 0.0f && (closestT < 0.0f || t1 < closestT))
                closestT = t1;

            float t2 = rayTriangleDistance(rayOrigin, rayDir, v01, v10, v11);
            if (t2 >= 0.0f && (closestT < 0.0f || t2 < closestT))
                closestT = t2;
        }
    }

    if (closestT >= 0.0f && outHit)
    {
        *outHit = rayOrigin + rayDir * closestT;
    }

    return closestT;
}

} // namespace gizmo

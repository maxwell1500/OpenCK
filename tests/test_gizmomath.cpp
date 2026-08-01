// Unit tests for the gizmo picking/dragging math library
// (src/view/window/gizmomath). Pure math: screen <-> world mapping,
// axis picking distance, drag deltas, arc-ball rotation, world-space
// sizing and snapping. No QTest, no widgets -- a plain executable
// that exits 0 on success and 1 on any failed check.

#include <cmath>
#include <cstdio>

#include "../src/view/window/gizmomath.hpp"

static int gFailures = 0;

static void checkFailed(bool ok, const char* expr, int line)
{
    if (!ok) {
        ++gFailures;
        std::printf("FAIL line %d: %s\n", line, expr);
    }
}

#define CHECK(cond) checkFailed((cond), #cond, __LINE__)

static bool close(float a, float b, float tol)
{
    return std::fabs(a - b) <= tol;
}

static bool close(const QVector3D& a, const QVector3D& b, float tol)
{
    return close(a.x(), b.x(), tol) && close(a.y(), b.y(), tol) && close(a.z(), b.z(), tol);
}

static gizmo::ViewTransform translatedZoomTransform()
{
    QMatrix4x4 translation;
    translation.translate(-5.0f, 0.0f, 0.0f);
    QMatrix4x4 scale;
    scale.scale(2.0f);

    gizmo::ViewTransform t;
    t.view = translation * scale;
    t.model.scale(0.01f);
    t.proj.setToIdentity();
    t.viewport = QSize(800, 600);
    return t;
}

static gizmo::ViewTransform centeredTransform()
{
    gizmo::ViewTransform t;
    t.view.scale(2.0f);
    t.model.scale(0.01f);
    t.proj.setToIdentity();
    t.viewport = QSize(800, 600);
    return t;
}

int main()
{
    // Test 1: identity-ish transform with a translation and zoom.
    // modelView = translate(-5,0,0) * scale(2) * scale(0.01); NDC(0,0,0) =
    // (-5,0,0), so the world point at the screen center is (250, 0, 0).
    {
        gizmo::ViewTransform t = translatedZoomTransform();

        QVector3D centerScreen = gizmo::worldToScreen(t, QVector3D(250.0f, 0.0f, 0.0f));
        CHECK(close(centerScreen, QVector3D(400.0f, 300.0f, 0.5f), 1e-3f));

        QVector3D originScreen = gizmo::worldToScreen(t, QVector3D(0.0f, 0.0f, 0.0f));
        CHECK(close(originScreen, QVector3D(-1600.0f, 300.0f, 0.5f), 1e-3f));

        QVector3D p(100.0f, 50.0f, -200.0f);
        QVector3D pScreen = gizmo::worldToScreen(t, p);
        QVector3D pRound = gizmo::screenToWorld(t, QPointF(pScreen.x(), pScreen.y()), p);
        CHECK(close(pRound, p, 1e-3f));

        QVector3D originRound = gizmo::screenToWorld(t, QPointF(originScreen.x(), originScreen.y()),
                                                     QVector3D(0.0f, 0.0f, 0.0f));
        CHECK(close(originRound, QVector3D(0.0f, 0.0f, 0.0f), 1e-3f));

        float size = gizmo::worldSizeForPixels(t, 80.0f);
        CHECK(size > 0.0f);
        CHECK(close(size, 10.0f, 1e-3f));

        gizmo::ViewTransform zoomed = t;
        zoomed.view.scale(2.0f);
        float zoomedSize = gizmo::worldSizeForPixels(zoomed, 80.0f);
        CHECK(zoomedSize < size);
        CHECK(close(zoomedSize, 5.0f, 1e-3f));

        gizmo::ViewTransform centered;
        centered.model.scale(0.01f);
        centered.proj.setToIdentity();
        centered.viewport = QSize(800, 600);
        QVector3D origin = gizmo::worldToScreen(centered, QVector3D(0.0f, 0.0f, 0.0f));
        CHECK(close(origin, QVector3D(400.0f, 300.0f, 0.5f), 1e-3f));
    }

    // Test 2: rotation transform (rotX=30, rotY=45) with zoom and camera
    // translation; known point + round trip.
    {
        QMatrix4x4 rotX;
        rotX.rotate(30.0f, 1.0f, 0.0f, 0.0f);
        QMatrix4x4 rotY;
        rotY.rotate(45.0f, 0.0f, 1.0f, 0.0f);

        gizmo::ViewTransform t;
        t.view = rotY * rotX;
        t.view.scale(2.0f);
        t.model.scale(0.01f);
        t.proj.setToIdentity();
        t.viewport = QSize(800, 600);

        QVector3D center = gizmo::worldToScreen(t, QVector3D(0.0f, 0.0f, 0.0f));
        CHECK(close(center, QVector3D(400.0f, 300.0f, 0.5f), 1e-3f));

        float cos45 = 0.70710678118f;
        float ndcX = 2.0f * 0.01f * 2.0f * cos45;
        float ndcZ = -2.0f * 0.01f * 2.0f * cos45;
        float expectedX = (ndcX + 1.0f) * 0.5f * 800.0f;

        QVector3D s = gizmo::worldToScreen(t, QVector3D(2.0f, 0.0f, 0.0f));
        CHECK(close(s.x(), expectedX, 0.5f));
        CHECK(close(s.y(), 300.0f, 0.5f));
        CHECK(close(s.z(), (ndcZ + 1.0f) * 0.5f, 1e-3f));

        gizmo::ViewTransform unrotated;
        unrotated.view.scale(2.0f);
        unrotated.model.scale(0.01f);
        unrotated.proj.setToIdentity();
        unrotated.viewport = QSize(800, 600);
        QVector3D sUnrotated = gizmo::worldToScreen(unrotated, QVector3D(2.0f, 0.0f, 0.0f));
        CHECK(sUnrotated.x() > 400.0f);
        CHECK(s.x() > 400.0f);
        CHECK(s.x() < sUnrotated.x());

        QVector3D q(3.0f, -7.0f, 11.0f);
        QVector3D qScreen = gizmo::worldToScreen(t, q);
        QVector3D qRound = gizmo::screenToWorld(t, QPointF(qScreen.x(), qScreen.y()), q);
        CHECK(close(qRound, q, 1e-3f));
    }

    // Test 3: axis pick distance on the world X axis through the origin.
    {
        gizmo::ViewTransform t = centeredTransform();

        QVector3D onAxis = gizmo::worldToScreen(t, QVector3D(50.0f, 0.0f, 0.0f));
        float dOnAxis = gizmo::axisPickDistance(t, QPointF(onAxis.x(), onAxis.y()),
                                                QVector3D(0.0f, 0.0f, 0.0f),
                                                QVector3D(1.0f, 0.0f, 0.0f), 100.0f);
        CHECK(dOnAxis >= 0.0f);
        CHECK(dOnAxis < 1.0f);

        float dCenter = gizmo::axisPickDistance(t, QPointF(400.0, 300.0),
                                                QVector3D(0.0f, 0.0f, 0.0f),
                                                QVector3D(1.0f, 0.0f, 0.0f), 100.0f);
        CHECK(dCenter < 1.0f);

        float dFar = gizmo::axisPickDistance(t, QPointF(400.0, 100.0),
                                             QVector3D(0.0f, 0.0f, 0.0f),
                                             QVector3D(1.0f, 0.0f, 0.0f), 100.0f);
        CHECK(dFar > 10.0f);

        float dDegenerate = gizmo::axisPickDistance(t, QPointF(400.0, 300.0),
                                                    QVector3D(0.0f, 0.0f, 0.0f),
                                                    QVector3D(1.0f, 0.0f, 0.0f), 0.0f);
        CHECK(dDegenerate < 0.0f);
    }

    // Test 4: drag delta along the world X axis for pure screen deltas.
    {
        gizmo::ViewTransform t = centeredTransform();

        float dX = gizmo::dragDeltaAlongAxis(t, QVector3D(0.0f, 0.0f, 0.0f),
                                             QVector3D(1.0f, 0.0f, 0.0f), QPointF(40.0, 0.0));
        CHECK(close(dX, gizmo::worldSizeForPixels(t, 40.0f), 1e-3f));
        CHECK(close(dX, 5.0f, 1e-3f));

        float dY = gizmo::dragDeltaAlongAxis(t, QVector3D(0.0f, 0.0f, 0.0f),
                                             QVector3D(1.0f, 0.0f, 0.0f), QPointF(0.0, 40.0));
        CHECK(close(dY, 0.0f, 1e-3f));
    }

    // Test 5: arc-ball rotation around the screen projection of the origin.
    {
        gizmo::ViewTransform t = centeredTransform();

        float ccw = gizmo::arcballRotation(t, QVector3D(0.0f, 0.0f, 0.0f),
                                           QPointF(450.0, 300.0), QPointF(400.0, 350.0));
        CHECK(close(ccw, 90.0f, 0.5f));

        float half = gizmo::arcballRotation(t, QVector3D(0.0f, 0.0f, 0.0f),
                                            QPointF(450.0, 300.0), QPointF(350.0, 300.0));
        CHECK(close(half, 180.0f, 0.5f));

        float cw = gizmo::arcballRotation(t, QVector3D(0.0f, 0.0f, 0.0f),
                                          QPointF(450.0, 300.0), QPointF(400.0, 250.0));
        CHECK(close(cw, -90.0f, 0.5f));

        float degenerate = gizmo::arcballRotation(t, QVector3D(0.0f, 0.0f, 0.0f),
                                                  QPointF(400.0, 300.0), QPointF(450.0, 300.0));
        CHECK(close(degenerate, 0.0f, 1e-3f));
    }

    // Test 6: snap helpers.
    {
        CHECK(close(gizmo::snapToStep(1.37f, 0.5), 1.5f, 1e-6f));
        CHECK(close(gizmo::snapToStep(-1.37f, 0.5), -1.5f, 1e-6f));
        CHECK(close(gizmo::snapToStep(1.37f, 0.0), 1.37f, 1e-6f));
        CHECK(close(gizmo::snapDegrees(37.0f, 15), 30.0f, 1e-6f));
        CHECK(close(gizmo::snapDegrees(352.0f, 15), 345.0f, 1e-6f));
        CHECK(close(gizmo::snapDegrees(-37.0f, 15), -30.0f, 1e-6f));
    }

    if (gFailures == 0) {
        std::printf("test_gizmomath: all checks passed\n");
        return 0;
    }
    std::printf("test_gizmomath: %d check(s) failed\n", gFailures);
    return 1;
}

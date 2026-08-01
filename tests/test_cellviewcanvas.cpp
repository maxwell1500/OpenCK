// Unit tests for CellMapView (src/view/window/cellmapview.{hpp,cpp}).
// Covers the world<->screen mapping, fitCell, pan, zoom-at-anchor,
// hitTest, and pxPerUnit clamping. Plain executable, no QTest.

#include <cmath>
#include <cstdio>

#include "../../src/view/window/cellmapview.hpp"

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::printf("FAIL: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
            ++failures; \
        } \
    } while (0)

#define CHECK_NEAR(a, b, eps) \
    do { \
        const double va_ = (a); \
        const double vb_ = (b); \
        if (std::fabs(va_ - vb_) > (eps)) { \
            std::printf("FAIL: |%s - %s| = |%.12g - %.12g| > %g at %s:%d\n", \
                #a, #b, va_, vb_, (eps), __FILE__, __LINE__); \
            ++failures; \
        } \
    } while (0)

static bool nearPoint(const QPointF& a, const QPointF& b, double eps)
{
    return std::fabs(a.x() - b.x()) <= eps && std::fabs(a.y() - b.y()) <= eps;
}

int main()
{
    int failures = 0;

    // Identity mapping on an 800x600 widget.
    {
        CellMapView view;
        view.setWidgetSize(QSize(800, 600));
        view.setCenterWorld(QPointF(0, 0));
        view.setPxPerUnit(1.0);

        CHECK(view.worldToScreen(QPointF(0, 0)) == QPointF(400, 300));
        CHECK(view.worldToScreen(QPointF(100, 50)) == QPointF(500, 350));
        CHECK(view.screenToWorld(QPointF(400, 300)) == QPointF(0, 0));

        const QPointF points[] = {
            QPointF(-100, 50),
            QPointF(1234.5, -999.25),
            QPointF(0, 0),
            QPointF(4096.0 * 3.7, -4096.0 * 1.2),
            QPointF(-8192.0, 0.001),
        };
        for (const QPointF& p : points)
            CHECK(nearPoint(view.screenToWorld(view.worldToScreen(p)), p, 1e-6));
    }

    // fitCell(0, 0, 800x600): center on the cell, cell fits with margin.
    {
        CellMapView view;
        view.fitCell(0, 0, QSize(800, 600));

        CHECK(nearPoint(view.centerWorld(), QPointF(2048, 2048), 1e-6));
        CHECK_NEAR(view.pxPerUnit(), 600.0 * 0.9 / 4096.0, 1e-6);

        const QPointF topLeft = view.worldToScreen(QPointF(0, 0));
        const QPointF bottomRight = view.worldToScreen(QPointF(4096, 4096));
        CHECK(topLeft.x() >= 0.0 && topLeft.y() >= 0.0);
        CHECK(bottomRight.x() <= 800.0 && bottomRight.y() <= 600.0);
        CHECK(nearPoint(view.worldToScreen(QPointF(2048, 2048)), QPointF(400, 300), 1e-6));
    }

    // fitCell(-2, 3): cell center follows the grid formula.
    {
        CellMapView view;
        view.fitCell(-2, 3, QSize(800, 600));
        CHECK(nearPoint(view.centerWorld(), QPointF(-1.5 * 4096.0, 3.5 * 4096.0), 1e-6));
    }

    // zoomAt: anchor at widget center doubles zoom and keeps center.
    {
        CellMapView view;
        view.fitCell(0, 0, QSize(800, 600));
        const double px1 = view.pxPerUnit();
        const QPointF center1 = view.centerWorld();

        view.zoomAt(QPointF(400, 300), 2.0);
        CHECK_NEAR(view.pxPerUnit(), px1 * 2.0, 1e-6);
        CHECK(nearPoint(view.centerWorld(), center1, 1e-6));

        // Anchor at (0, 0): the world point under the anchor is fixed.
        view.fitCell(1, -2, QSize(800, 600));
        const QPointF worldBefore = view.worldAt(QPointF(0, 0));
        view.zoomAt(QPointF(0, 0), 2.0);
        CHECK(nearPoint(view.worldAt(QPointF(0, 0)), worldBefore, 1e-6));
    }

    // panByPixels moves the center opposite to the pixel delta.
    {
        CellMapView view;
        view.fitCell(0, 0, QSize(800, 600));
        const double px = view.pxPerUnit();
        const QPointF center1 = view.centerWorld();

        view.panByPixels(QPointF(10, -5));
        CHECK_NEAR(view.centerWorld().x(), center1.x() - 10.0 / px, 1e-6);
        CHECK_NEAR(view.centerWorld().y(), center1.y() + 5.0 / px, 1e-6);
    }

    // hitTest on an identity transform.
    {
        CellMapView view;
        view.setWidgetSize(QSize(800, 600));
        view.setCenterWorld(QPointF(0, 0));
        view.setPxPerUnit(1.0);

        const QVector<QPointF> points = { QPointF(0, 0), QPointF(100, 100), QPointF(200, 0) };
        CHECK(view.hitTest(points, view.worldToScreen(QPointF(100, 100))) == 1);
        CHECK(view.hitTest(points, view.worldToScreen(QPointF(100, 100)), 1.0) == 1);
        CHECK(view.hitTest(points, view.worldToScreen(QPointF(1000, 1000))) == -1);
        CHECK(view.hitTest(points, view.worldToScreen(QPointF(10, 10)), 20.0) == 0);

        const QVector<QPointF> close = { QPointF(0, 0), QPointF(1, 0) };
        CHECK(view.hitTest(close, view.worldToScreen(QPointF(0, 0))) == 0);

        // Nearest wins even when the first candidate is also within range.
        const QVector<QPointF> near2 = { QPointF(0, 0), QPointF(8, 0) };
        CHECK(view.hitTest(near2, view.worldToScreen(QPointF(8, 0))) == 1);
        CHECK(view.hitTest({}, QPointF(400, 300)) == -1);
    }

    // pxPerUnit clamps to [0.01, 200.0].
    {
        CellMapView view;
        view.setPxPerUnit(1e6);
        CHECK_NEAR(view.pxPerUnit(), 200.0, 1e-12);
        view.setPxPerUnit(1e-9);
        CHECK_NEAR(view.pxPerUnit(), 0.01, 1e-12);
        view.setPxPerUnit(1.5);
        CHECK_NEAR(view.pxPerUnit(), 1.5, 1e-12);
    }

    // Negative grid cells round-trip through world<->screen.
    {
        CellMapView view;
        view.fitCell(-1, -1, QSize(800, 600));
        const QPointF world(100, -200);
        CHECK(nearPoint(view.screenToWorld(view.worldToScreen(world)), world, 1e-6));
    }

    // fitCell with empty widget size is a no-op.
    {
        CellMapView view;
        view.setWidgetSize(QSize(800, 600));
        view.setCenterWorld(QPointF(5, 5));
        view.setPxPerUnit(2.0);
        view.fitCell(0, 0, QSize());
        CHECK(view.widgetSize() == QSize(800, 600));
        CHECK(view.centerWorld() == QPointF(5, 5));
        CHECK_NEAR(view.pxPerUnit(), 2.0, 1e-12);
    }

    if (failures == 0)
    {
        std::printf("test_cellviewcanvas: all checks passed\n");
        return 0;
    }
    std::printf("test_cellviewcanvas: %d check(s) failed\n", failures);
    return 1;
}

#ifndef CELLMAPVIEW_HPP
#define CELLMAPVIEW_HPP

#include <QPointF>
#include <QSize>
#include <QVector>

/// Pure-math pan/zoom transform for the Cell View map canvas.
/// Maps between world units (game units, one cell = kCellUnits square)
/// and screen pixels (Y-down, top-left origin).
class CellMapView
{
public:
    CellMapView();

    static constexpr double kCellUnits = 4096.0;
    static constexpr double kMinPxPerUnit = 0.01;
    static constexpr double kMaxPxPerUnit = 200.0;

    // Center of the view in world units.
    QPointF centerWorld() const;
    void setCenterWorld(const QPointF& center);

    // Pixels per world unit (zoom), clamped to [kMinPxPerUnit, kMaxPxPerUnit].
    double pxPerUnit() const;
    void setPxPerUnit(double px);

    // Size of the viewport in pixels, default 800x600.
    QSize widgetSize() const;
    void setWidgetSize(const QSize& size);

    QPointF worldToScreen(const QPointF& world) const;
    QPointF screenToWorld(const QPointF& screen) const;

    // Center cell (gx, gy) and pick pxPerUnit so the 4096-unit cell
    // square fits with 10% margin. No-op if widgetSize is empty.
    void fitCell(int gx, int gy, const QSize& widgetSize);

    // Pan by a screen-space delta in pixels (positive dx pans right).
    void panByPixels(const QPointF& deltaPx);

    // Zoom by factor at an anchor screen point (the world point under
    // the cursor stays fixed under the cursor).
    void zoomAt(const QPointF& anchorScreen, double factor);

    // World point under a screen point.
    QPointF worldAt(const QPointF& screenPos) const;

    // Index of the nearest world point to screenPos, or -1 if none
    // within maxPixels.
    int hitTest(const QVector<QPointF>& worldPoints, const QPointF& screenPos,
                double maxPixels = 10.0) const;

private:
    QPointF mCenterWorld;
    double mPxPerUnit = 1.0;
    QSize mWidgetSize{800, 600};
};

#endif // CELLMAPVIEW_HPP

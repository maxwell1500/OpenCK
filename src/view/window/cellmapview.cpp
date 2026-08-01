#include "cellmapview.hpp"

#include <algorithm>

CellMapView::CellMapView() = default;

QPointF CellMapView::centerWorld() const
{
    return mCenterWorld;
}

void CellMapView::setCenterWorld(const QPointF& center)
{
    mCenterWorld = center;
}

double CellMapView::pxPerUnit() const
{
    return mPxPerUnit;
}

void CellMapView::setPxPerUnit(double px)
{
    mPxPerUnit = std::clamp(px, kMinPxPerUnit, kMaxPxPerUnit);
}

QSize CellMapView::widgetSize() const
{
    return mWidgetSize;
}

void CellMapView::setWidgetSize(const QSize& size)
{
    mWidgetSize = size;
}

QPointF CellMapView::worldToScreen(const QPointF& world) const
{
    const QPointF widgetCenter(mWidgetSize.width() / 2.0, mWidgetSize.height() / 2.0);
    return widgetCenter + (world - mCenterWorld) * mPxPerUnit;
}

QPointF CellMapView::screenToWorld(const QPointF& screen) const
{
    const QPointF widgetCenter(mWidgetSize.width() / 2.0, mWidgetSize.height() / 2.0);
    return mCenterWorld + (screen - widgetCenter) / mPxPerUnit;
}

void CellMapView::fitCell(int gx, int gy, const QSize& widgetSize)
{
    if (widgetSize.isEmpty())
        return;

    const double px = std::min(widgetSize.width() * 0.9 / kCellUnits,
                               widgetSize.height() * 0.9 / kCellUnits);
    const QPointF cellCenter((gx + 0.5) * kCellUnits, (gy + 0.5) * kCellUnits);

    setWidgetSize(widgetSize);
    setCenterWorld(cellCenter);
    setPxPerUnit(px);
}

void CellMapView::panByPixels(const QPointF& deltaPx)
{
    setCenterWorld(mCenterWorld - deltaPx / mPxPerUnit);
}

void CellMapView::zoomAt(const QPointF& anchorScreen, double factor)
{
    const QPointF worldUnder = screenToWorld(anchorScreen);
    setPxPerUnit(mPxPerUnit * factor);
    const QPointF widgetCenter(mWidgetSize.width() / 2.0, mWidgetSize.height() / 2.0);
    setCenterWorld(worldUnder - (anchorScreen - widgetCenter) / mPxPerUnit);
}

QPointF CellMapView::worldAt(const QPointF& screenPos) const
{
    return screenToWorld(screenPos);
}

int CellMapView::hitTest(const QVector<QPointF>& worldPoints, const QPointF& screenPos,
                         double maxPixels) const
{
    const double maxDistSq = maxPixels * maxPixels;
    double bestDistSq = maxDistSq;
    int bestIndex = -1;
    for (int i = 0; i < worldPoints.size(); ++i)
    {
        const QPointF delta = worldToScreen(worldPoints[i]) - screenPos;
        const double distSq = delta.x() * delta.x() + delta.y() * delta.y();
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestIndex = i;
        }
    }
    return bestIndex;
}

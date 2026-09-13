#include "galaxyviewwidget.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

#include <algorithm>
#include <cmath>

GalaxyViewWidget::GalaxyViewWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(320, 240);
    setToolTip(tr("Star systems in galaxy space. Click a system to select it."));
}

void GalaxyViewWidget::setMap(const GalaxyMap& map)
{
    m_map = map;
    if (m_selected >= m_map.systems.size())
        m_selected = -1;
    update();
}

void GalaxyViewWidget::setSelectedSystem(int index)
{
    m_selected = (index >= 0 && index < m_map.systems.size()) ? index : -1;
    update();
    emit systemSelected(m_selected);
}

QPointF GalaxyViewWidget::toScreen(double x, double y) const
{
    if (m_map.systems.isEmpty())
        return QPointF(width() / 2.0, height() / 2.0);

    double minX = m_map.systems[0].x, maxX = minX;
    double minY = m_map.systems[0].y, maxY = minY;
    for (const auto& s : m_map.systems)
    {
        minX = std::min(minX, s.x);
        maxX = std::max(maxX, s.x);
        minY = std::min(minY, s.y);
        maxY = std::max(maxY, s.y);
    }

    const double pad = 40.0;
    const double spanX = std::max(1.0, maxX - minX);
    const double spanY = std::max(1.0, maxY - minY);
    const double scale = std::min((width() - 2 * pad) / spanX,
                                  (height() - 2 * pad) / spanY);
    const double cx = (minX + maxX) / 2.0;
    const double cy = (minY + maxY) / 2.0;

    return QPointF(width() / 2.0 + (x - cx) * scale,
                   height() / 2.0 - (y - cy) * scale);
}

int GalaxyViewWidget::pickSystem(const QPoint& pos) const
{
    int best = -1;
    double bestDist = 24.0; // pixels
    for (int i = 0; i < m_map.systems.size(); ++i)
    {
        const QPointF p = toScreen(m_map.systems[i].x, m_map.systems[i].y);
        const double dx = p.x() - pos.x();
        const double dy = p.y() - pos.y();
        const double dist = std::sqrt(dx * dx + dy * dy);
        if (dist < bestDist)
        {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

void GalaxyViewWidget::mousePressEvent(QMouseEvent* event)
{
    const int index = pickSystem(event->pos());
    if (index >= 0)
        setSelectedSystem(index);
}

void GalaxyViewWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(8, 10, 20));

    painter.setPen(QColor(160, 170, 190));
    painter.drawText(rect().adjusted(8, 4, -8, 0), Qt::AlignLeft | Qt::AlignTop,
                     m_map.name.isEmpty() ? tr("Galaxy") : m_map.name);

    for (int i = 0; i < m_map.systems.size(); ++i)
    {
        const auto& s = m_map.systems[i];
        const QPointF p = toScreen(s.x, s.y);
        const double radius = 6.0 + 2.0 * std::min(6, static_cast<int>(s.planets.size()));

        const bool selected = (i == m_selected);
        painter.setPen(selected ? QColor(255, 220, 120) : QColor(140, 180, 255));
        painter.setBrush(selected ? QColor(90, 70, 20, 160) : QColor(40, 60, 120, 160));
        painter.drawEllipse(p, radius, radius);

        painter.setPen(QColor(220, 225, 235));
        painter.drawText(QRectF(p.x() + radius + 3, p.y() - 8, 220, 16),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("%1 (%2)").arg(s.name).arg(s.planets.size()));
    }
}

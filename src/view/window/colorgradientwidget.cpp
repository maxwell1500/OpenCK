#include "colorgradientwidget.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>
#include <algorithm>

ColorGradientWidget::ColorGradientWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(60);
    setFixedHeight(60);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_stops.append({0.0f, QColor(255, 255, 255)});
    m_stops.append({1.0f, QColor(255, 255, 255)});
}

void ColorGradientWidget::setGradient(const QVector<ColorStop>& stops)
{
    m_stops = stops;
    if (m_stops.isEmpty()) {
        m_stops.append({0.0f, QColor(255, 255, 255)});
        m_stops.append({1.0f, QColor(255, 255, 255)});
    }
    m_selectedStop = -1;
    update();
}

QColor ColorGradientWidget::colorAt(float position) const
{
    if (m_stops.isEmpty()) return Qt::white;
    if (m_stops.size() == 1) return m_stops[0].color;
    if (position <= m_stops[0].position) return m_stops[0].color;
    if (position >= m_stops.last().position) return m_stops.last().color;

    for (int i = 0; i < m_stops.size() - 1; ++i) {
        if (position >= m_stops[i].position && position <= m_stops[i + 1].position) {
            float t = (position - m_stops[i].position) / (m_stops[i + 1].position - m_stops[i].position);
            return QColor::fromRgbF(
                m_stops[i].color.redF() + t * (m_stops[i + 1].color.redF() - m_stops[i].color.redF()),
                m_stops[i].color.greenF() + t * (m_stops[i + 1].color.greenF() - m_stops[i].color.greenF()),
                m_stops[i].color.blueF() + t * (m_stops[i + 1].color.blueF() - m_stops[i].color.blueF()),
                m_stops[i].color.alphaF() + t * (m_stops[i + 1].color.alphaF() - m_stops[i].color.alphaF())
            );
        }
    }
    return m_stops.last().color;
}

void ColorGradientWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect gradRect(10, 4, width() - 20, 24);
    drawGradient(painter, gradRect);

    int markerY = gradRect.bottom() + 2;
    int markerH = height() - markerY - 2;

    for (int i = 0; i < m_stops.size(); ++i) {
        int x = gradRect.left() + static_cast<int>(m_stops[i].position * gradRect.width());
        QPoint top(x, markerY);
        QPoint left(x - 6, markerY + markerH);
        QPoint right(x + 6, markerY + markerH);

        if (i == m_selectedStop) {
            painter.setPen(QPen(Qt::yellow, 2));
        } else {
            painter.setPen(QPen(Qt::gray, 1));
        }
        painter.setBrush(m_stops[i].color);
        painter.drawPolygon(QPolygon() << top << left << right);
    }
}

void ColorGradientWidget::drawGradient(QPainter& painter, const QRect& rect)
{
    for (int x = 0; x < rect.width(); ++x) {
        float t = static_cast<float>(x) / rect.width();
        painter.setPen(colorAt(t));
        painter.drawLine(rect.left() + x, rect.top(), rect.left() + x, rect.bottom());
    }
    painter.setPen(QPen(Qt::darkGray, 1));
    painter.drawRect(rect);
}

int ColorGradientWidget::stopAtPosition(const QPoint& pos) const
{
    QRect gradRect(10, 4, width() - 20, 24);
    int markerY = gradRect.bottom() + 2;
    int markerH = height() - markerY - 2;

    for (int i = 0; i < m_stops.size(); ++i) {
        int x = gradRect.left() + static_cast<int>(m_stops[i].position * gradRect.width());
        QPoint top(x, markerY);
        QPoint left(x - 7, markerY + markerH + 1);
        QPoint right(x + 7, markerY + markerH + 1);

        QPolygon tri;
        tri << top << left << right;
        if (tri.containsPoint(pos, Qt::OddEvenFill)) {
            return i;
        }
    }
    return -1;
}

void ColorGradientWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int idx = stopAtPosition(event->pos());
        if (idx >= 0) {
            m_selectedStop = idx;
            m_draggingStop = idx;
        } else {
            QRect gradRect(10, 4, width() - 20, 24);
            float t = qBound(0.0f, static_cast<float>(event->pos().x() - gradRect.left()) / gradRect.width(), 1.0f);
            QColor c = colorAt(t);
            ColorStop stop{t, c};

            int insertIdx = 0;
            for (int i = 0; i < m_stops.size(); ++i) {
                if (m_stops[i].position < t) insertIdx = i + 1;
            }
            m_stops.insert(insertIdx, stop);
            m_selectedStop = insertIdx;
            update();
            emit gradientChanged();
        }
    }
    QWidget::mousePressEvent(event);
}

void ColorGradientWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_draggingStop >= 0 && (event->buttons() & Qt::LeftButton)) {
        QRect gradRect(10, 4, width() - 20, 24);
        float t = qBound(0.0f, static_cast<float>(event->pos().x() - gradRect.left()) / gradRect.width(), 1.0f);
        m_stops[m_draggingStop].position = t;

        std::sort(m_stops.begin(), m_stops.end(), [](const ColorStop& a, const ColorStop& b) {
            return a.position < b.position;
        });

        for (int i = 0; i < m_stops.size(); ++i) {
            if (qFuzzyCompare(m_stops[i].position, t) &&
                m_stops[i].color == m_stops[m_draggingStop].color) {
                m_selectedStop = i;
                m_draggingStop = i;
                break;
            }
        }

        update();
        emit gradientChanged();
    }
    QWidget::mouseMoveEvent(event);
}

void ColorGradientWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    int idx = stopAtPosition(event->pos());
    if (idx >= 0) {
        QColor c = QColorDialog::getColor(m_stops[idx].color, this, tr("Select Color"),
                                          QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            m_stops[idx].color = c;
            update();
            emit gradientChanged();
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void ColorGradientWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete && m_selectedStop >= 0) {
        if (m_stops.size() > 2) {
            m_stops.removeAt(m_selectedStop);
            m_selectedStop = -1;
            update();
            emit gradientChanged();
        }
    }
    QWidget::keyPressEvent(event);
}

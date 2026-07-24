#include "sizecurvewidget.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QInputDialog>
#include <QPainterPath>
#include <algorithm>

SizeCurveWidget::SizeCurveWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setFixedHeight(120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_points.append(QPointF(0.0f, 1.0f));
}

void SizeCurveWidget::setCurve(const QVector<QPointF>& points)
{
    m_points = points;
    if (m_points.isEmpty()) {
        m_points.append(QPointF(0.0f, 1.0f));
    }
    m_selectedPoint = -1;
    update();
}

float SizeCurveWidget::sizeAt(float time) const
{
    if (m_points.isEmpty()) return 1.0f;
    if (m_points.size() == 1) return static_cast<float>(m_points[0].y());
    if (time <= m_points[0].x()) return static_cast<float>(m_points[0].y());
    if (time >= m_points.last().x()) return static_cast<float>(m_points.last().y());

    for (int i = 0; i < m_points.size() - 1; ++i) {
        if (time >= m_points[i].x() && time <= m_points[i + 1].x()) {
            float dx = m_points[i + 1].x() - m_points[i].x();
            if (qFuzzyIsNull(dx)) return static_cast<float>(m_points[i].y());
            float t = (time - m_points[i].x()) / dx;
            return static_cast<float>(m_points[i].y() + t * (m_points[i + 1].y() - m_points[i].y()));
        }
    }
    return static_cast<float>(m_points.last().y());
}

QRect SizeCurveWidget::curveRect() const
{
    return QRect(40, 10, width() - 50, height() - 25);
}

void SizeCurveWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect cr = curveRect();

    // grid
    painter.setPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    for (int i = 1; i < 5; ++i) {
        int y = cr.top() + i * cr.height() / 4;
        painter.drawLine(cr.left(), y, cr.right(), y);
    }
    for (int i = 1; i < 4; ++i) {
        int x = cr.left() + i * cr.width() / 4;
        painter.drawLine(x, cr.top(), x, cr.bottom());
    }

    // axes
    painter.setPen(QPen(Qt::white, 1));
    painter.drawLine(cr.left(), cr.top(), cr.left(), cr.bottom());
    painter.drawLine(cr.left(), cr.bottom(), cr.right(), cr.bottom());

    // labels
    painter.setPen(Qt::lightGray);
    QFont f = painter.font();
    f.setPixelSize(9);
    painter.setFont(f);
    painter.drawText(2, cr.top() + 4, "2.0");
    painter.drawText(10, cr.top() + cr.height() / 2 + 4, "1.0");
    painter.drawText(18, cr.bottom() - 2, "0.0");
    painter.drawText(cr.left() + cr.width() / 2 - 5, cr.bottom() + 12, "0.5");
    painter.drawText(cr.right() - 5, cr.bottom() + 12, "1.0");

    // curve
    if (m_points.size() >= 2) {
        QPainterPath path;
        float startT = static_cast<float>(m_points[0].x());
        int startX = cr.left() + static_cast<int>(startT * cr.width());
        float startV = static_cast<float>(m_points[0].y());
        int startY = cr.bottom() - static_cast<int>((startV / 2.0f) * cr.height());
        path.moveTo(startX, startY);

        for (int i = 1; i < m_points.size(); ++i) {
            float t = static_cast<float>(m_points[i].x());
            float v = static_cast<float>(m_points[i].y());
            int px = cr.left() + static_cast<int>(t * cr.width());
            int py = cr.bottom() - static_cast<int>((v / 2.0f) * cr.height());
            path.lineTo(px, py);
        }

        painter.setPen(QPen(Qt::cyan, 2));
        painter.drawPath(path);
    }

    // points
    for (int i = 0; i < m_points.size(); ++i) {
        float t = static_cast<float>(m_points[i].x());
        float v = static_cast<float>(m_points[i].y());
        int px = cr.left() + static_cast<int>(t * cr.width());
        int py = cr.bottom() - static_cast<int>((v / 2.0f) * cr.height());

        if (i == m_selectedPoint) {
            painter.setPen(QPen(Qt::yellow, 2));
            painter.setBrush(Qt::yellow);
        } else {
            painter.setPen(QPen(Qt::white, 1));
            painter.setBrush(Qt::darkCyan);
        }
        painter.drawEllipse(QPoint(px, py), 5, 5);
    }
}

int SizeCurveWidget::pointAtPosition(const QPoint& pos) const
{
    QRect cr = curveRect();
    for (int i = 0; i < m_points.size(); ++i) {
        float t = static_cast<float>(m_points[i].x());
        float v = static_cast<float>(m_points[i].y());
        int px = cr.left() + static_cast<int>(t * cr.width());
        int py = cr.bottom() - static_cast<int>((v / 2.0f) * cr.height());

        int dx = pos.x() - px;
        int dy = pos.y() - py;
        if (dx * dx + dy * dy <= 64) {
            return i;
        }
    }
    return -1;
}

void SizeCurveWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int idx = pointAtPosition(event->pos());
        if (idx >= 0) {
            m_selectedPoint = idx;
            m_draggingPoint = idx;
        } else {
            QRect cr = curveRect();
            float t = qBound(0.0f, static_cast<float>(event->pos().x() - cr.left()) / cr.width(), 1.0f);
            float v = qBound(0.0f, 2.0f * static_cast<float>(cr.bottom() - event->pos().y()) / cr.height(), 2.0f);

            int insertIdx = 0;
            for (int i = 0; i < m_points.size(); ++i) {
                if (m_points[i].x() < t) insertIdx = i + 1;
            }
            m_points.insert(insertIdx, QPointF(t, v));
            m_selectedPoint = insertIdx;
            update();
            emit curveChanged();
        }
    }
    QWidget::mousePressEvent(event);
}

void SizeCurveWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_draggingPoint >= 0 && (event->buttons() & Qt::LeftButton)) {
        QRect cr = curveRect();
        float t = qBound(0.0f, static_cast<float>(event->pos().x() - cr.left()) / cr.width(), 1.0f);
        float v = qBound(0.0f, 2.0f * static_cast<float>(cr.bottom() - event->pos().y()) / cr.height(), 2.0f);

        QPointF oldPt = m_points[m_draggingPoint];
        m_points[m_draggingPoint] = QPointF(t, v);

        std::sort(m_points.begin(), m_points.end(), [](const QPointF& a, const QPointF& b) {
            return a.x() < b.x();
        });

        for (int i = 0; i < m_points.size(); ++i) {
            if (qFuzzyCompare(static_cast<double>(m_points[i].x()), static_cast<double>(t)) &&
                qFuzzyCompare(static_cast<double>(m_points[i].y()), static_cast<double>(v))) {
                m_selectedPoint = i;
                m_draggingPoint = i;
                break;
            }
        }

        update();
        emit curveChanged();
    }
    QWidget::mouseMoveEvent(event);
}

void SizeCurveWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    int idx = pointAtPosition(event->pos());
    if (idx >= 0) {
        bool ok;
        double time = QInputDialog::getDouble(this, tr("Point Time"),
            tr("Time (0-1):"), m_points[idx].x(), 0.0, 1.0, 2, &ok);
        if (!ok) return;

        double size = QInputDialog::getDouble(this, tr("Point Size"),
            tr("Size (0-2):"), m_points[idx].y(), 0.0, 2.0, 2, &ok);
        if (!ok) return;

        m_points[idx] = QPointF(time, size);
        std::sort(m_points.begin(), m_points.end(), [](const QPointF& a, const QPointF& b) {
            return a.x() < b.x();
        });
        for (int i = 0; i < m_points.size(); ++i) {
            if (qFuzzyCompare(static_cast<double>(m_points[i].x()), static_cast<double>(time)) &&
                qFuzzyCompare(static_cast<double>(m_points[i].y()), static_cast<double>(size))) {
                m_selectedPoint = i;
                break;
            }
        }
        update();
        emit curveChanged();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void SizeCurveWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete && m_selectedPoint >= 0) {
        if (m_points.size() > 1) {
            m_points.removeAt(m_selectedPoint);
            m_selectedPoint = -1;
            update();
            emit curveChanged();
        }
    }
    QWidget::keyPressEvent(event);
}

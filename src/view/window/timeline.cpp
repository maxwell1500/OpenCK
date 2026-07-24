#include "timeline.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QtMath>

#include "../../libs/files/nifanim/nifanimation.hpp"

TimelineWidget::TimelineWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(m_rulerHeight + m_channelHeight);
}

void TimelineWidget::setClip(const AnimClip* clip)
{
    m_clip = clip;
    m_selectedKeyframes.clear();
    m_markers.clear();
    m_selectedMarker = -1;
    m_dragging = false;
    m_draggingTime = false;

    if (m_clip) {
        m_duration = m_clip->duration;
        if (m_duration <= 0.0f) {
            float maxTime = 0.0f;
            for (const auto& ch : m_clip->channels) {
                for (const auto& kf : ch.keyframes) {
                    if (kf.time > maxTime)
                        maxTime = kf.time;
                }
            }
            m_duration = maxTime > 0.0f ? maxTime : 1.0f;
        }
    } else {
        m_duration = 0.0f;
    }

    update();
}

void TimelineWidget::setCurrentTime(float time)
{
    if (qFuzzyCompare(m_currentTime, time)) return;
    m_currentTime = time;
    update();
}

void TimelineWidget::setSelectedKeyframes(const QSet<QPair<QString, float>>& keyframes)
{
    m_selectedKeyframes = keyframes;
    update();
}

void TimelineWidget::setPixelsPerSecond(float pps)
{
    if (pps < 10.0f) pps = 10.0f;
    if (pps > 4000.0f) pps = 4000.0f;
    if (qFuzzyCompare(m_pixelsPerSecond, pps)) return;
    m_pixelsPerSecond = pps;
    update();
}

void TimelineWidget::addMarker(float time, const QString& name, const QColor& color)
{
    Marker m;
    m.time = time;
    m.name = name;
    m.color = color;
    m_markers.append(m);
    update();
    emit markerAdded(time, name);
}

void TimelineWidget::removeMarker(int index)
{
    if (index < 0 || index >= m_markers.size()) return;
    m_markers.removeAt(index);
    if (m_selectedMarker == index) m_selectedMarker = -1;
    else if (m_selectedMarker > index) m_selectedMarker--;
    update();
    emit markerRemoved(index);
}

void TimelineWidget::clearMarkers()
{
    m_markers.clear();
    m_selectedMarker = -1;
    update();
}

int TimelineWidget::markerAtPosition(const QPoint& pos) const
{
    if (pos.y() > m_rulerHeight) return -1;

    for (int i = 0; i < m_markers.size(); ++i) {
        int mx = timeToX(m_markers[i].time);
        int dx = qAbs(pos.x() - mx);
        if (dx <= 8 && pos.y() < 20) {
            return i;
        }
    }
    return -1;
}

// --- Coordinate conversion ---

float TimelineWidget::xToTime(int x) const
{
    return static_cast<float>(x) / m_pixelsPerSecond;
}

int TimelineWidget::timeToX(float time) const
{
    return qRound(time * m_pixelsPerSecond);
}

int TimelineWidget::channelIndexAtY(int y) const
{
    int contentY = y - m_rulerHeight;
    if (contentY < 0) return -1;
    if (!m_clip) return -1;
    int idx = contentY / m_channelHeight;
    if (idx >= m_clip->channels.size()) return -1;
    return idx;
}

TimelineWidget::KeyframePos TimelineWidget::keyframeAtPosition(const QPoint& pos) const
{
    KeyframePos result;
    result.channelIndex = -1;

    if (!m_clip) return result;

    int channelIdx = channelIndexAtY(pos.y());
    if (channelIdx < 0) return result;

    const auto& ch = m_clip->channels[channelIdx];
    int halfSize = m_keyframeSize / 2;
    int clickRadius = halfSize + 3;

    for (const auto& kf : ch.keyframes) {
        int kx = timeToX(kf.time);
        int ky = m_rulerHeight + channelIdx * m_channelHeight + m_channelHeight / 2;
        int dx = qAbs(pos.x() - kx);
        int dy = qAbs(pos.y() - ky);
        if (dx <= clickRadius && dy <= clickRadius) {
            result.boneName = ch.boneName;
            result.time = kf.time;
            result.channelIndex = channelIdx;
            return result;
        }
    }

    return result;
}

void TimelineWidget::snapToGrid(float& time) const
{
    time = qRound(time / m_snapGrid) * m_snapGrid;
    if (time < 0.0f) time = 0.0f;
}

// --- Drawing ---

void TimelineWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor(40, 40, 40));

    QRect rulerRect(0, 0, width(), m_rulerHeight);
    drawRuler(painter, rulerRect);
    drawMarkers(painter, rulerRect);

    if (m_clip && !m_clip->channels.isEmpty()) {
        int channelAreaHeight = m_clip->channels.size() * m_channelHeight;
        QRect channelRect(0, m_rulerHeight, width(), channelAreaHeight);
        drawChannels(painter, channelRect);
        drawKeyframes(painter, channelRect);
    }

    drawCurrentTimeIndicator(painter, rect());
}

void TimelineWidget::drawRuler(QPainter& painter, const QRect& rulerRect)
{
    // Ruler background
    painter.fillRect(rulerRect, QColor(55, 55, 55));

    // Bottom border
    painter.setPen(QColor(80, 80, 80));
    painter.drawLine(rulerRect.bottomLeft(), rulerRect.bottomRight());

    float startTime = xToTime(rulerRect.left());
    float endTime = xToTime(rulerRect.right());

    // Determine tick spacing based on zoom level
    float majorInterval = 0.5f;
    float minorInterval = 0.1f;

    if (m_pixelsPerSecond < 50.0f) {
        majorInterval = 2.0f;
        minorInterval = 0.5f;
    } else if (m_pixelsPerSecond < 100.0f) {
        majorInterval = 1.0f;
        minorInterval = 0.25f;
    } else if (m_pixelsPerSecond > 500.0f) {
        majorInterval = 0.25f;
        minorInterval = 0.05f;
    }

    // Draw minor ticks
    painter.setPen(QColor(100, 100, 100));
    float firstMinor = qFloor(startTime / minorInterval) * minorInterval;
    for (float t = firstMinor; t <= endTime; t += minorInterval) {
        int x = timeToX(t);
        painter.drawLine(x, rulerRect.bottom() - 5, x, rulerRect.bottom());
    }

    // Draw major ticks and labels
    painter.setPen(QColor(200, 200, 200));
    QFont rulerFont("Segoe UI", 8);
    painter.setFont(rulerFont);

    float firstMajor = qFloor(startTime / majorInterval) * majorInterval;
    for (float t = firstMajor; t <= endTime; t += majorInterval) {
        int x = timeToX(t);
        painter.drawLine(x, rulerRect.bottom() - 12, x, rulerRect.bottom());

        QString label = QString("%1").arg(t, 0, 'f', 1);
        QRectF labelRect(x - 20, 2, 40, 16);
        painter.drawText(labelRect, Qt::AlignCenter, label);
    }
}

void TimelineWidget::drawChannels(QPainter& painter, const QRect& channelRect)
{
    if (!m_clip) return;

    QFont channelFont("Segoe UI", 8);

    for (int i = 0; i < m_clip->channels.size(); ++i) {
        const auto& ch = m_clip->channels[i];
        int y = channelRect.top() + i * m_channelHeight;

        // Alternating background
        QColor bgColor = (i % 2 == 0) ? QColor(45, 45, 45) : QColor(50, 50, 50);
        painter.fillRect(QRect(channelRect.left(), y, channelRect.width(), m_channelHeight), bgColor);

        // Channel separator line
        painter.setPen(QColor(65, 65, 65));
        painter.drawLine(channelRect.left(), y + m_channelHeight, channelRect.right(), y + m_channelHeight);

        // Bone name label
        painter.setPen(QColor(180, 180, 180));
        painter.setFont(channelFont);
        QRect labelRect(channelRect.left() + 4, y + 2, channelRect.width() - 8, m_channelHeight - 4);
        painter.drawText(labelRect, Qt::AlignVCenter | Qt::AlignLeft, ch.boneName);
    }
}

void TimelineWidget::drawKeyframes(QPainter& painter, const QRect& channelRect)
{
    if (!m_clip) return;

    int halfSize = m_keyframeSize / 2;

    for (int i = 0; i < m_clip->channels.size(); ++i) {
        const auto& ch = m_clip->channels[i];
        int cy = channelRect.top() + i * m_channelHeight + m_channelHeight / 2;

        for (const auto& kf : ch.keyframes) {
            int kx = timeToX(kf.time);

            bool selected = m_selectedKeyframes.contains(qMakePair(ch.boneName, kf.time));

            // Diamond shape
            QPolygon diamond;
            diamond << QPoint(kx, cy - halfSize)
                     << QPoint(kx + halfSize, cy)
                     << QPoint(kx, cy + halfSize)
                     << QPoint(kx - halfSize, cy);

            QColor fillColor = selected ? QColor(80, 160, 255) : QColor(220, 160, 40);
            QColor borderColor = selected ? QColor(120, 190, 255) : QColor(240, 180, 60);

            painter.setPen(borderColor);
            painter.setBrush(fillColor);
            painter.drawPolygon(diamond);
        }
    }
}

void TimelineWidget::drawCurrentTimeIndicator(QPainter& painter, const QRect& fullRect)
{
    if (m_duration <= 0.0f) return;

    int x = timeToX(m_currentTime);

    // Playhead handle at top
    QPolygon handle;
    handle << QPoint(x, 0)
           << QPoint(x - 6, 0)
           << QPoint(x, 10)
           << QPoint(x + 6, 0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(220, 50, 50));
    painter.drawPolygon(handle);

    // Vertical line
    painter.setPen(QPen(QColor(220, 50, 50), 1));
    painter.drawLine(x, 10, x, fullRect.bottom());
}

void TimelineWidget::drawMarkers(QPainter& painter, const QRect& rulerRect)
{
    QFont markerFont("Segoe UI", 7);
    painter.setFont(markerFont);

    for (int i = 0; i < m_markers.size(); ++i) {
        const auto& m = m_markers[i];
        int x = timeToX(m.time);

        bool selected = (i == m_selectedMarker);

        // Small triangle pointing down at top of ruler
        QPolygon triangle;
        triangle << QPoint(x - 5, 0)
                 << QPoint(x + 5, 0)
                 << QPoint(x, 10);

        painter.setPen(selected ? QColor(255, 255, 255) : QColor(0, 0, 0));
        painter.setBrush(m.color);
        painter.drawPolygon(triangle);

        // Marker name below triangle
        painter.setPen(m.color);
        QRectF nameRect(x - 30, 11, 60, 14);
        painter.drawText(nameRect, Qt::AlignHCenter | Qt::AlignTop, m.name);
    }
}

// --- Mouse interaction ---

void TimelineWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_clip) return;

    QPoint pos = event->pos();

    // Middle mouse button = pan
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_panStartX = pos.x();
        m_panStartPPS = m_pixelsPerSecond;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    // Right-click on ruler = context menu for adding marker
    if (event->button() == Qt::RightButton && pos.y() < m_rulerHeight) {
        int markerIdx = markerAtPosition(pos);
        if (markerIdx >= 0) {
            m_selectedMarker = markerIdx;
            update();
            QMenu menu(this);
            menu.addAction(tr("Remove Marker"), this, [this, markerIdx]() {
                removeMarker(markerIdx);
            });
            menu.exec(event->globalPos());
        } else {
            float time = xToTime(pos.x());
            snapToGrid(time);
            if (time > m_duration) time = m_duration;

            QMenu menu(this);
            menu.addAction(tr("Add Marker"), this, [this, time]() {
                addMarker(time, QString("Marker %1").arg(m_markers.size() + 1));
            });
            menu.exec(event->globalPos());
        }
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    // Click on ruler area = check for marker first, then set current time
    if (pos.y() < m_rulerHeight) {
        int markerIdx = markerAtPosition(pos);
        if (markerIdx >= 0) {
            m_selectedMarker = markerIdx;
            update();
            return;
        }
        m_selectedMarker = -1;
        float time = xToTime(pos.x());
        snapToGrid(time);
        if (time > m_duration) time = m_duration;
        m_currentTime = time;
        emit timeChanged(m_currentTime);
        update();
        return;
    }

    // Click on channel area
    KeyframePos kf = keyframeAtPosition(pos);
    if (kf.channelIndex >= 0) {
        // Clicked on a keyframe
        m_selectedKeyframes.clear();
        m_selectedKeyframes.insert(qMakePair(kf.boneName, kf.time));
        emit keyframeSelected(kf.boneName, kf.time);

        // Start drag
        m_dragging = true;
        m_dragStart = kf;
        m_dragStartTime = kf.time;
        m_dragOffsetX = pos.x() - timeToX(kf.time);
        update();
    } else {
        // Clicked on empty area - set current time
        int channelIdx = channelIndexAtY(pos.y());
        if (channelIdx >= 0) {
            float time = xToTime(pos.x());
            snapToGrid(time);
            if (time > m_duration) time = m_duration;
            m_currentTime = time;
            emit timeChanged(m_currentTime);
            m_selectedKeyframes.clear();
            update();
        }
    }
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_clip) return;

    QPoint pos = event->pos();

    // Panning
    if (m_panning) {
        int dx = pos.x() - m_panStartX;
        float newPPS = m_panStartPPS + static_cast<float>(dx) * 0.5f;
        setPixelsPerSecond(newPPS);
        return;
    }

    // Dragging a keyframe
    if (m_dragging) {
        float newTime = xToTime(pos.x() - m_dragOffsetX);
        snapToGrid(newTime);
        if (newTime < 0.0f) newTime = 0.0f;
        if (newTime > m_duration) newTime = m_duration;

        // Update selection with new time
        m_selectedKeyframes.clear();
        m_selectedKeyframes.insert(qMakePair(m_dragStart.boneName, newTime));

        // Temporarily move the keyframe for visual feedback
        // We store the time and update on release
        m_dragStartTime = newTime;
        update();
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        unsetCursor();
        return;
    }

    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        float newTime = m_dragStartTime;

        if (!qFuzzyCompare(m_dragStart.time, newTime)) {
            emit keyframeMoved(m_dragStart.boneName, m_dragStart.time, newTime);
        }
        update();
    }
}

void TimelineWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_clip) return;
    if (event->button() != Qt::LeftButton) return;

    QPoint pos = event->pos();
    if (pos.y() < m_rulerHeight) return;

    int channelIdx = channelIndexAtY(pos.y());
    if (channelIdx < 0) return;

    KeyframePos kf = keyframeAtPosition(pos);
    if (kf.channelIndex >= 0) return; // Double-clicked on existing keyframe, ignore

    float time = xToTime(pos.x());
    snapToGrid(time);
    if (time > m_duration) time = m_duration;

    const auto& ch = m_clip->channels[channelIdx];
    emit keyframeAdded(ch.boneName, time);
}

void TimelineWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // Delete selected marker first if one is selected
        if (m_selectedMarker >= 0) {
            removeMarker(m_selectedMarker);
            return;
        }
        if (!m_selectedKeyframes.isEmpty()) {
            auto keyframes = m_selectedKeyframes;
            for (const auto& kf : keyframes) {
                emit keyframeRemoved(kf.first, kf.second);
            }
            m_selectedKeyframes.clear();
            update();
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

void TimelineWidget::wheelEvent(QWheelEvent* event)
{
    QPoint pos = event->position().toPoint();
    float timeBefore = xToTime(pos.x());

    float delta = event->angleDelta().y() > 0 ? 1.15f : 1.0f / 1.15f;
    setPixelsPerSecond(m_pixelsPerSecond * delta);

    // Adjust scroll so the time under cursor stays the same
    float timeAfter = xToTime(pos.x());
    if (!qFuzzyCompare(timeBefore, timeAfter)) {
        float timeDiff = timeAfter - timeBefore;
        // We could scroll horizontally here, but for simplicity just let it be
        Q_UNUSED(timeDiff)
    }

    update();
}

#pragma once

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QString>
#include <QPointF>
#include <QRectF>
#include <QSet>
#include <QPair>
#include <QColor>
#include <QMenu>

struct AnimChannel;
struct AnimClip;

class TimelineWidget : public QWidget
{
    Q_OBJECT

public:
    struct KeyframePos {
        QString boneName;
        float time;
        int channelIndex;
    };

    struct Marker {
        float time;
        QString name;
        QColor color;
    };

    explicit TimelineWidget(QWidget* parent = nullptr);

    void setClip(const AnimClip* clip);
    void setCurrentTime(float time);
    void setSelectedKeyframes(const QSet<QPair<QString, float>>& keyframes);
    float duration() const { return m_duration; }
    float pixelsPerSecond() const { return m_pixelsPerSecond; }
    float currentTime() const { return m_currentTime; }

    void setPixelsPerSecond(float pps);

    void addMarker(float time, const QString& name, const QColor& color = QColor(255, 255, 0));
    void removeMarker(int index);
    void clearMarkers();
    const QVector<Marker>& markers() const { return m_markers; }
    int selectedMarkerIndex() const { return m_selectedMarker; }

signals:
    void timeChanged(float time);
    void keyframeSelected(const QString& boneName, float time);
    void keyframeMoved(const QString& boneName, float oldTime, float newTime);
    void keyframeAdded(const QString& boneName, float time);
    void keyframeRemoved(const QString& boneName, float time);
    void rangeSelected(float startTime, float endTime);
    void markerAdded(float time, const QString& name);
    void markerRemoved(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    QSize minimumSizeHint() const override { return QSize(200, 100); }

private:
    void drawRuler(QPainter& painter, const QRect& rulerRect);
    void drawChannels(QPainter& painter, const QRect& channelRect);
    void drawKeyframes(QPainter& painter, const QRect& channelRect);
    void drawCurrentTimeIndicator(QPainter& painter, const QRect& fullRect);
    void drawMarkers(QPainter& painter, const QRect& rulerRect);

    float xToTime(int x) const;
    int timeToX(float time) const;
    int channelIndexAtY(int y) const;
    KeyframePos keyframeAtPosition(const QPoint& pos) const;
    int markerAtPosition(const QPoint& pos) const;
    void snapToGrid(float& time) const;

    const AnimClip* m_clip = nullptr;
    float m_duration = 0.0f;
    float m_currentTime = 0.0f;
    float m_pixelsPerSecond = 200.0f;
    int m_rulerHeight = 30;
    int m_channelHeight = 24;
    int m_keyframeSize = 8;
    float m_snapGrid = 0.01f;

    QSet<QPair<QString, float>> m_selectedKeyframes;

    // Markers
    QVector<Marker> m_markers;
    int m_selectedMarker = -1;

    // Drag state
    bool m_dragging = false;
    bool m_draggingTime = false;
    KeyframePos m_dragStart;
    float m_dragStartTime = 0.0f;
    int m_dragOffsetX = 0;

    // Panning
    bool m_panning = false;
    int m_panStartX = 0;
    float m_panStartPPS = 0.0f;
};

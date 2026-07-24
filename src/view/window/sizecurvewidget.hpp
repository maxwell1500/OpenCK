#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>

class SizeCurveWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SizeCurveWidget(QWidget* parent = nullptr);

    void setCurve(const QVector<QPointF>& points);
    QVector<QPointF> curve() const { return m_points; }

    float sizeAt(float time) const;

signals:
    void curveChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void drawCurve(QPainter& painter, const QRect& rect);
    int pointAtPosition(const QPoint& pos) const;
    QRect curveRect() const;

    QVector<QPointF> m_points; // x=time(0-1), y=size(0-2)
    int m_selectedPoint = -1;
    int m_draggingPoint = -1;
};

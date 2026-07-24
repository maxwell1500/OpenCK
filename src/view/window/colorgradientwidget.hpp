#pragma once

#include <QWidget>
#include <QVector>
#include <QColor>

class ColorGradientWidget : public QWidget
{
    Q_OBJECT
public:
    struct ColorStop {
        float position; // 0.0 to 1.0
        QColor color;
    };

    explicit ColorGradientWidget(QWidget* parent = nullptr);

    void setGradient(const QVector<ColorStop>& stops);
    QVector<ColorStop> gradient() const { return m_stops; }

    QColor colorAt(float position) const;

signals:
    void gradientChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void drawGradient(QPainter& painter, const QRect& rect);
    int stopAtPosition(const QPoint& pos) const;

    QVector<ColorStop> m_stops;
    int m_selectedStop = -1;
    int m_draggingStop = -1;
};

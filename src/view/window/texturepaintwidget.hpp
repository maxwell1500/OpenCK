#ifndef TEXTUREPAINTWIDGET_HPP
#define TEXTUREPAINTWIDGET_HPP

#include <QWidget>
#include <QImage>
#include <QPointF>
#include <QVector>

class QSpinBox;
class QToolButton;

class TexturePaintWidget : public QWidget
{
    Q_OBJECT

public:
    enum Tool { Brush, Eraser, Picker };

    explicit TexturePaintWidget(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    QImage getImage() const { return m_image; }
    bool saveImage(const QString& path);

    void blur(int radius);
    void sharpen(int amount);
    void brightness(int amount);
    void contrast(int amount);

    static QVector<QImage> generateMipmaps(const QImage& source);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void paintStroke(const QPointF& from, const QPointF& to);

    QImage m_image;
    QImage m_strokeBuffer;
    Tool m_tool = Brush;
    QColor m_brushColor = Qt::white;
    int m_brushSize = 8;
    qreal m_zoom = 1.0;
    QPointF m_panOffset;
    bool m_painting = false;
    bool m_panning = false;
    QPointF m_lastPos;

    QSpinBox* m_brushSizeSpin = nullptr;
    QToolButton* m_brushBtn = nullptr;
    QToolButton* m_eraserBtn = nullptr;
    QToolButton* m_pickerBtn = nullptr;
    QToolButton* m_colorBtn = nullptr;

    friend class TextureEditorDialog;
};

#endif // TEXTUREPAINTWIDGET_HPP

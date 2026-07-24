#include "texturepaintwidget.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QFileDialog>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QSpinBox>
#include <QLabel>
#include <QScrollArea>
#include <QtMath>
#include <algorithm>

namespace {

int clampInt(int v, int lo, int hi) { return qMax(lo, qMin(v, hi)); }

} // namespace

TexturePaintWidget::TexturePaintWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(256, 256);

    m_image = QImage(256, 256, QImage::Format_ARGB32);
    m_image.fill(Qt::transparent);
}

void TexturePaintWidget::setImage(const QImage& image)
{
    if (image.isNull()) return;
    m_image = image.convertToFormat(QImage::Format_ARGB32);
    m_panOffset = QPointF(0, 0);
    m_zoom = qMin(512.0 / m_image.width(), 512.0 / m_image.height());
    if (m_zoom > 1.0) m_zoom = 1.0;
    update();
}

bool TexturePaintWidget::saveImage(const QString& path)
{
    if (path.isEmpty()) return false;
    return m_image.save(path);
}

void TexturePaintWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    // Checker background
    const int tileSize = 16;
    for (int y = 0; y < height(); y += tileSize) {
        for (int x = 0; x < width(); x += tileSize) {
            bool light = ((x / tileSize) + (y / tileSize)) % 2 == 0;
            painter.fillRect(x, y, tileSize, tileSize, light ? QColor(200, 200, 200) : QColor(160, 160, 160));
        }
    }

    // Draw image
    QPointF imgPos = m_panOffset;
    QSizeF imgSize = QSizeF(m_image.width(), m_image.height()) * m_zoom;

    // Center image if it fits
    if (imgSize.width() < width()) {
        imgPos.setX((width() - imgSize.width()) / 2.0 + m_panOffset.x());
    }
    if (imgSize.height() < height()) {
        imgPos.setY((height() - imgSize.height()) / 2.0 + m_panOffset.y());
    }

    painter.drawImage(QRectF(imgPos, imgSize), m_image, QRectF(m_image.rect()));
}

void TexturePaintWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_painting = true;

        if (m_tool == Picker) {
            // Color picker
            QSizeF imgSize = QSizeF(m_image.width(), m_image.height()) * m_zoom;
            QPointF imgPos(0, 0);
            if (imgSize.width() < width())
                imgPos.setX((width() - imgSize.width()) / 2.0 + m_panOffset.x());
            if (imgSize.height() < height())
                imgPos.setY((height() - imgSize.height()) / 2.0 + m_panOffset.y());

            qreal ix = (event->pos().x() - imgPos.x()) / m_zoom;
            qreal iy = (event->pos().y() - imgPos.y()) / m_zoom;
            int px = qBound(0, static_cast<int>(ix), m_image.width() - 1);
            int py = qBound(0, static_cast<int>(iy), m_image.height() - 1);

            m_brushColor = QColor(m_image.pixel(px, py));
            if (m_colorBtn) {
                m_colorBtn->setStyleSheet(
                    QString("background-color: %1").arg(m_brushColor.name()));
            }
            m_tool = Brush;
            if (m_brushBtn) m_brushBtn->setChecked(true);
            m_painting = false;
            update();
            return;
        }

        m_strokeBuffer = m_image.copy();
        QPointF imgPos(0, 0);
        QSizeF imgSize = QSizeF(m_image.width(), m_image.height()) * m_zoom;
        if (imgSize.width() < width())
            imgPos.setX((width() - imgSize.width()) / 2.0 + m_panOffset.x());
        if (imgSize.height() < height())
            imgPos.setY((height() - imgSize.height()) / 2.0 + m_panOffset.y());

        qreal ix = (event->pos().x() - imgPos.x()) / m_zoom;
        qreal iy = (event->pos().y() - imgPos.y()) / m_zoom;
        paintStroke(QPointF(ix, iy), QPointF(ix, iy));
        update();
    }
}

void TexturePaintWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning) {
        QPointF delta = event->pos() - m_lastPos;
        m_panOffset += delta;
        m_lastPos = event->pos();
        update();
        return;
    }

    if (m_painting && (event->buttons() & Qt::LeftButton)) {
        QSizeF imgSize = QSizeF(m_image.width(), m_image.height()) * m_zoom;
        QPointF imgPos(0, 0);
        if (imgSize.width() < width())
            imgPos.setX((width() - imgSize.width()) / 2.0 + m_panOffset.x());
        if (imgSize.height() < height())
            imgPos.setY((height() - imgSize.height()) / 2.0 + m_panOffset.y());

        qreal ix = (event->pos().x() - imgPos.x()) / m_zoom;
        qreal iy = (event->pos().y() - imgPos.y()) / m_zoom;

        qreal prevIx = (m_lastPos.x() - imgPos.x()) / m_zoom;
        qreal prevIy = (m_lastPos.y() - imgPos.y()) / m_zoom;

        paintStroke(QPointF(prevIx, prevIy), QPointF(ix, iy));
        m_lastPos = event->pos();
        update();
    }
}

void TexturePaintWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        return;
    }

    if (event->button() == Qt::LeftButton && m_painting) {
        m_painting = false;
    }
}

void TexturePaintWidget::wheelEvent(QWheelEvent* event)
{
    const qreal factor = (event->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
    qreal newZoom = m_zoom * factor;
    newZoom = qBound(0.01, newZoom, 20.0);

    // Zoom towards mouse cursor
    QPointF mousePos = event->position();
    QSizeF imgSize = QSizeF(m_image.width(), m_image.height()) * m_zoom;
    QPointF imgPos(0, 0);
    if (imgSize.width() < width())
        imgPos.setX((width() - imgSize.width()) / 2.0 + m_panOffset.x());
    if (imgSize.height() < height())
        imgPos.setY((height() - imgSize.height()) / 2.0 + m_panOffset.y());

    QPointF imgCoords = mousePos - imgPos;

    m_zoom = newZoom;

    QSizeF newImgSize = QSizeF(m_image.width(), m_image.height()) * m_zoom;
    QPointF newImgPos(0, 0);
    if (newImgSize.width() < width())
        newImgPos.setX((width() - newImgSize.width()) / 2.0 + m_panOffset.x());
    if (newImgSize.height() < height())
        newImgPos.setY((height() - newImgSize.height()) / 2.0 + m_panOffset.y());

    m_panOffset += mousePos - (newImgPos + imgCoords * (newZoom / (newZoom / factor)));
    // Simplify: just recalculate pan to keep mouse position stable
    QPointF worldBefore = (mousePos - imgPos) / m_zoom;
    m_panOffset = mousePos - QPointF(m_image.width() / 2.0, m_image.height() / 2.0) * m_zoom;
    // Correct for centering
    if (newImgSize.width() < width())
        m_panOffset.setX((width() - newImgSize.width()) / 2.0);
    if (newImgSize.height() < height())
        m_panOffset.setY((height() - newImgSize.height()) / 2.0);

    m_panOffset += mousePos - newImgPos - worldBefore * m_zoom;

    update();
}

void TexturePaintWidget::paintStroke(const QPointF& from, const QPointF& to)
{
    QPainter painter(&m_image);
    painter.setRenderHint(QPainter::Antialiasing, false);

    if (m_tool == Eraser) {
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::transparent);
    } else {
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_brushColor);
    }

    const int radius = m_brushSize / 2;

    if (from == to) {
        painter.drawEllipse(from, radius, radius);
        return;
    }

    // Draw along line with spacing = radius/2 for smooth strokes
    const qreal dist = qSqrt(qPow(to.x() - from.x(), 2) + qPow(to.y() - from.y(), 2));
    const qreal spacing = qMax(1.0, radius / 2.0);
    const int steps = qMax(1, static_cast<int>(dist / spacing));

    for (int i = 0; i <= steps; ++i) {
        const qreal t = (steps == 0) ? 1.0 : static_cast<qreal>(i) / steps;
        const qreal x = from.x() + (to.x() - from.x()) * t;
        const qreal y = from.y() + (to.y() - from.y()) * t;
        painter.drawEllipse(QPointF(x, y), radius, radius);
    }
}

// --- Texture Filters ---

void TexturePaintWidget::blur(int radius)
{
    if (m_image.isNull() || radius < 1) return;

    const int w = m_image.width();
    const int h = m_image.height();
    QImage result = m_image.copy();

    // Horizontal pass
    QImage temp(w, h, QImage::Format_ARGB32);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int r = 0, g = 0, b = 0, a = 0, count = 0;
            for (int k = -radius; k <= radius; ++k) {
                int sx = qBound(0, x + k, w - 1);
                QRgb pixel = result.pixel(sx, y);
                r += qRed(pixel);
                g += qGreen(pixel);
                b += qBlue(pixel);
                a += qAlpha(pixel);
                ++count;
            }
            temp.setPixel(x, y, qRgba(r / count, g / count, b / count, a / count));
        }
    }

    // Vertical pass
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int r = 0, g = 0, b = 0, a = 0, count = 0;
            for (int k = -radius; k <= radius; ++k) {
                int sy = qBound(0, y + k, h - 1);
                QRgb pixel = temp.pixel(x, sy);
                r += qRed(pixel);
                g += qGreen(pixel);
                b += qBlue(pixel);
                a += qAlpha(pixel);
                ++count;
            }
            result.setPixel(x, y, qRgba(r / count, g / count, b / count, a / count));
        }
    }

    m_image = result;
    update();
}

void TexturePaintWidget::sharpen(int amount)
{
    if (m_image.isNull() || amount < 1) return;

    // Unsharp mask: original + amount * (original - blurred)
    QImage original = m_image.copy();

    // Create blurred copy without modifying m_image
    QImage blurred = m_image;
    {
        const int blurRadius = 2;
        const int w = blurred.width();
        const int h = blurred.height();
        QImage temp(w, h, QImage::Format_ARGB32);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int r = 0, g = 0, b = 0, a = 0, count = 0;
                for (int k = -blurRadius; k <= blurRadius; ++k) {
                    int sx = qBound(0, x + k, w - 1);
                    QRgb pixel = blurred.pixel(sx, y);
                    r += qRed(pixel);
                    g += qGreen(pixel);
                    b += qBlue(pixel);
                    a += qAlpha(pixel);
                    ++count;
                }
                temp.setPixel(x, y, qRgba(r / count, g / count, b / count, a / count));
            }
        }
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int r = 0, g = 0, b = 0, a = 0, count = 0;
                for (int k = -blurRadius; k <= blurRadius; ++k) {
                    int sy = qBound(0, y + k, h - 1);
                    QRgb pixel = temp.pixel(x, sy);
                    r += qRed(pixel);
                    g += qGreen(pixel);
                    b += qBlue(pixel);
                    a += qAlpha(pixel);
                    ++count;
                }
                blurred.setPixel(x, y, qRgba(r / count, g / count, b / count, a / count));
            }
        }
    }

    const int w = m_image.width();
    const int h = m_image.height();
    QImage sharp = original.copy();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            QRgb orig = original.pixel(x, y);
            QRgb blurPx = blurred.pixel(x, y);
            int r = qBound(0, qRed(orig) + amount * (qRed(orig) - qRed(blurPx)) / 100, 255);
            int g = qBound(0, qGreen(orig) + amount * (qGreen(orig) - qGreen(blurPx)) / 100, 255);
            int b = qBound(0, qBlue(orig) + amount * (qBlue(orig) - qBlue(blurPx)) / 100, 255);
            int a = qBound(0, qAlpha(orig) + amount * (qAlpha(orig) - qAlpha(blurPx)) / 100, 255);
            sharp.setPixel(x, y, qRgba(r, g, b, a));
        }
    }

    m_image = sharp;
    update();
}

void TexturePaintWidget::brightness(int amount)
{
    if (m_image.isNull()) return;

    const int w = m_image.width();
    const int h = m_image.height();
    QImage result = m_image.copy();

    const int shift = amount * 255 / 100;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            QRgb px = m_image.pixel(x, y);
            int r = qBound(0, qRed(px) + shift, 255);
            int g = qBound(0, qGreen(px) + shift, 255);
            int b = qBound(0, qBlue(px) + shift, 255);
            result.setPixel(x, y, qRgba(r, g, b, qAlpha(px)));
        }
    }

    m_image = result;
    update();
}

void TexturePaintWidget::contrast(int amount)
{
    if (m_image.isNull()) return;

    const int w = m_image.width();
    const int h = m_image.height();
    QImage result = m_image.copy();

    const qreal factor = (259.0 * (amount + 255)) / (255.0 * (259 - amount));

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            QRgb px = m_image.pixel(x, y);
            int r = qBound(0, static_cast<int>(factor * (qRed(px) - 128) + 128), 255);
            int g = qBound(0, static_cast<int>(factor * (qGreen(px) - 128) + 128), 255);
            int b = qBound(0, static_cast<int>(factor * (qBlue(px) - 128) + 128), 255);
            result.setPixel(x, y, qRgba(r, g, b, qAlpha(px)));
        }
    }

    m_image = result;
    update();
}

// --- Mipmap Generation ---

QVector<QImage> TexturePaintWidget::generateMipmaps(const QImage& source)
{
    QVector<QImage> levels;
    if (source.isNull()) return levels;

    QImage current = source.convertToFormat(QImage::Format_ARGB32);
    levels.append(current);

    while (current.width() > 1 || current.height() > 1) {
        int newW = qMax(1, current.width() / 2);
        int newH = qMax(1, current.height() / 2);

        QImage next(newW, newH, QImage::Format_ARGB32);
        next.fill(Qt::transparent);

        QPainter painter(&next);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(QRectF(0, 0, newW, newH), current, QRectF(current.rect()));
        painter.end();

        levels.append(next);
        current = next;
    }

    return levels;
}

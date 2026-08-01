#include "brushalphamask.hpp"

#include <QFile>
#include <QImageReader>

#include "libs/files/nif/ddsdecoder.hpp"
#include "libs/files/log/logger.hpp"

bool BrushAlphaMask::load(const QString& path)
{
    QImage image;
    if (path.endsWith(QStringLiteral(".dds"), Qt::CaseInsensitive)) {
        image = DdsDecoder::decodeFile(path);
        if (image.isNull()) {
            LOG_WARNING(QString("BrushAlphaMask: failed to decode DDS %1").arg(path));
            return false;
        }
    } else {
        QImageReader reader(path);
        if (!reader.canRead() || !reader.read(&image)) {
            LOG_WARNING(QString("BrushAlphaMask: failed to read image %1").arg(path));
            return false;
        }
    }

    setImage(image);
    mPath = path;
    LOG_INFO(QString("BrushAlphaMask: loaded %1x%2 mask from %3")
                 .arg(mAlpha.width()).arg(mAlpha.height()).arg(path));
    return true;
}

void BrushAlphaMask::setImage(const QImage& image)
{
    mAlpha = image.convertToFormat(QImage::Format_RGBA8888);
    mValid = !mAlpha.isNull();
    mPath.clear();
}

float BrushAlphaMask::valueAt(float nx, float ny) const
{
    if (!mValid) {
        return 1.0f;
    }

    // Normalized brush coordinates are -1..1; map onto the texture so the
    // brush circle touches the texture edges.
    const float u = (nx + 1.0f) * 0.5f;  // 0..1
    const float v = (ny + 1.0f) * 0.5f;  // 0..1
    return sampleBilinear(u, v);
}

float BrushAlphaMask::sampleBilinear(float u, float v) const
{
    const int w = mAlpha.width();
    const int h = mAlpha.height();
    if (w == 0 || h == 0) {
        return 0.0f;
    }

    const float fx = u * (w - 1);
    const float fy = v * (h - 1);
    const int x0 = qBound(0, static_cast<int>(fx), w - 1);
    const int y0 = qBound(0, static_cast<int>(fy), h - 1);
    const int x1 = qMin(x0 + 1, w - 1);
    const int y1 = qMin(y0 + 1, h - 1);
    const float tx = fx - x0;
    const float ty = fy - y0;

    const uchar* line0 = mAlpha.constScanLine(y0);
    const uchar* line1 = mAlpha.constScanLine(y1);
    const int stride = 4;

    const float a00 = line0[x0 * stride + 3] / 255.0f;
    const float a10 = line0[x1 * stride + 3] / 255.0f;
    const float a01 = line1[x0 * stride + 3] / 255.0f;
    const float a11 = line1[x1 * stride + 3] / 255.0f;

    const float top = a00 + (a10 - a00) * tx;
    const float bot = a01 + (a11 - a01) * tx;
    return qBound(0.0f, top + (bot - top) * ty, 1.0f);
}

void BrushAlphaMask::clear()
{
    mAlpha = QImage();
    mValid = false;
    mPath.clear();
}

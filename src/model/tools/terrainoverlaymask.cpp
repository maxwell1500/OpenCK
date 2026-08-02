#include "terrainoverlaymask.hpp"

#include <QFile>
#include <QImageReader>

#include "libs/files/nif/ddsdecoder.hpp"
#include "libs/files/log/logger.hpp"

bool TerrainOverlayMask::load(const QString& path)
{
    QImage image;
    if (path.endsWith(QStringLiteral(".dds"), Qt::CaseInsensitive))
    {
        image = DdsDecoder::decodeFile(path);
        if (image.isNull())
        {
            LOG_WARNING(QString("TerrainOverlayMask: failed to decode DDS %1").arg(path));
            return false;
        }
    }
    else
    {
        QImageReader reader(path);
        if (!reader.canRead() || !reader.read(&image))
        {
            LOG_WARNING(QString("TerrainOverlayMask: failed to read image %1").arg(path));
            return false;
        }
    }

    setImage(image);
    mPath = path;
    LOG_INFO(QString("TerrainOverlayMask: loaded %1x%2 mask from %3")
                 .arg(mAlpha.width()).arg(mAlpha.height()).arg(path));
    return true;
}

void TerrainOverlayMask::setImage(const QImage& image)
{
    mAlpha = image.convertToFormat(QImage::Format_RGBA8888);
    mValid = !mAlpha.isNull();
    mPath.clear();
}

float TerrainOverlayMask::sample(float u, float v) const
{
    if (!mValid)
        return 0.0f;
    return sampleBilinear(qBound(0.0f, u, 1.0f), qBound(0.0f, v, 1.0f));
}

float TerrainOverlayMask::sampleBilinear(float u, float v) const
{
    const int w = mAlpha.width();
    const int h = mAlpha.height();
    if (w == 0 || h == 0)
        return 0.0f;

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

QVector<float> TerrainOverlayMask::resample(int size) const
{
    QVector<float> result(size * size, 0.0f);
    if (!mValid || size <= 0)
        return result;
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const float u = (size > 1) ? static_cast<float>(x) / (size - 1) : 0.5f;
            const float v = (size > 1) ? static_cast<float>(y) / (size - 1) : 0.5f;
            result[y * size + x] = sample(u, v);
        }
    }
    return result;
}

void TerrainOverlayMask::clear()
{
    mAlpha = QImage();
    mValid = false;
    mPath.clear();
}

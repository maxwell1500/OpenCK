#include "ddsencoder.hpp"

#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

// DDS pixel format flags
constexpr quint32 DDPF_ALPHAPIXELS = 0x1;
constexpr quint32 DDPF_FOURCC = 0x4;

// FourCC codes
constexpr quint32 FOURCC_DXT1 = 0x31545844; // 'DXT1'
constexpr quint32 FOURCC_DXT5 = 0x35545844; // 'DXT5'

struct DdsHeader {
    quint32 dwSize;
    quint32 dwFlags;
    quint32 dwHeight;
    quint32 dwWidth;
    quint32 dwPitchOrLinearSize;
    quint32 dwDepth;
    quint32 dwMipMapCount;
    quint32 dwReserved1[11];
    struct {
        quint32 dwSize;
        quint32 dwFlags;
        quint32 dwFourCC;
        quint32 dwRGBBitCount;
        quint32 dwRBitMask;
        quint32 dwGBitMask;
        quint32 dwBBitMask;
        quint32 dwABitMask;
    } ddpfPixelFormat;
    quint32 dwCaps;
    quint32 dwCaps2;
    quint32 dwCaps3;
    quint32 dwCaps4;
    quint32 dwReserved2;
};

static_assert(sizeof(DdsHeader) == 124, "DDS header must be 124 bytes");

quint16 colorTo565(const DdsEncoder::Color& c) {
    int r = qBound(0, static_cast<int>(c.r * 255.0f), 255);
    int g = qBound(0, static_cast<int>(c.g * 255.0f), 255);
    int b = qBound(0, static_cast<int>(c.b * 255.0f), 255);
    return static_cast<quint16>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

DdsEncoder::Color expand565(quint16 c) {
    DdsEncoder::Color col;
    col.r = (((c >> 11) & 0x1F) * 255 + 16) / 31.0f;
    col.g = (((c >> 5) & 0x3F) * 255 + 32) / 63.0f;
    col.b = ((c & 0x1F) * 255 + 16) / 31.0f;
    return col;
}

static float colorDistance(const DdsEncoder::Color& a, const DdsEncoder::Color& b) {
    float dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
    return dr * dr + dg * dg + db * db;
}

static DdsEncoder::Color lerp(const DdsEncoder::Color& a, const DdsEncoder::Color& b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}

void encodeDxtBlock(const quint8* pixels, int width, int y0, int x0,
                    quint8* out, bool dxt5)
{
    float colors[16][3];
    float alphas[16] = {1.0f};

    for (int by = 0; by < 4; ++by) {
        for (int bx = 0; bx < 4; ++bx) {
            int px = x0 + bx, py = y0 + by;
            if (px >= 0 && py >= 0 && px < width && py < static_cast<int>(width)) {
                QRgb rgb = pixels[py * width * 4 + px * 4];
                colors[by * 4 + bx][0] = qRed(rgb) / 255.0f;
                colors[by * 4 + bx][1] = qGreen(rgb) / 255.0f;
                colors[by * 4 + bx][2] = qBlue(rgb) / 255.0f;
                alphas[by * 4 + bx] = qAlpha(rgb) / 255.0f;
            } else {
                colors[by * 4 + bx][0] = 0;
                colors[by * 4 + bx][1] = 0;
                colors[by * 4 + bx][2] = 0;
                alphas[by * 4 + bx] = 0;
            }
        }
    }

    if (dxt5) {
        quint8 alphaValues[8];
        std::sort(alphas, alphas + 16);
        alphaValues[0] = static_cast<quint8>(alphas[0] * 255.0f);
        alphaValues[7] = static_cast<quint8>(alphas[15] * 255.0f);

        float a0 = alphaValues[0], a7 = alphaValues[7];
        if (a0 > a7) {
            for (int i = 0; i < 6; ++i)
                alphaValues[1 + i] = static_cast<quint8>(((6 - i) * a0 + (1 + i) * a7) / 7.0f);
        } else {
            for (int i = 0; i < 4; ++i)
                alphaValues[1 + i] = static_cast<quint8>(((4 - i) * a0 + (1 + i) * a7) / 5.0f);
            alphaValues[5] = 0;
            alphaValues[6] = 255;
        }

        out[0] = alphaValues[0];
        out[1] = alphaValues[7];

        quint64 alphaBits = 0;
        for (int i = 0; i < 16; ++i) {
            int bestIdx = 0;
            float bestDist = 999.0f;
            for (int j = 0; j < 8; ++j) {
                float d = std::abs(alphas[i] * 255.0f - alphaValues[j]);
                if (d < bestDist) { bestDist = d; bestIdx = j; }
            }
            alphaBits |= static_cast<quint64>(bestIdx) << (3 * i);
        }
        for (int i = 0; i < 6; ++i)
            out[2 + i] = static_cast<quint8>((alphaBits >> (8 * i)) & 0xFF);

        float bestC0r = 0, bestC0g = 0, bestC0b = 0;
        float bestC1r = 0, bestC1g = 0, bestC1b = 0;
        quint32 bestIndices = 0;
        float bestError = 999999.0f;

        for (int i0 = 0; i0 < 16; ++i0) {
            for (int i1 = 0; i1 < 16; ++i1) {
                DdsEncoder::Color c0 = {colors[i0][0], colors[i0][1], colors[i0][2]};
                DdsEncoder::Color c1 = {colors[i1][0], colors[i1][1], colors[i1][2]};
                quint16 e0 = colorTo565(c0), e1 = colorTo565(c1);

                bool fourColors = (e0 > e1) || ((e0 == e1) && alphaValues[0] < 255);

                float error = 0;
                quint32 indices = 0;
                for (int p = 0; p < 16; ++p) {
                    DdsEncoder::Color target = {colors[p][0], colors[p][1], colors[p][2]};
                    DdsEncoder::Color bestColor = c0;
                    int bestIdx = 0;

                    if (fourColors) {
                        DdsEncoder::Color c2 = lerp(c0, c1, 1.0f / 3.0f);
                        DdsEncoder::Color c3 = lerp(c0, c1, 2.0f / 3.0f);
                        float d0 = colorDistance(target, c0), d1 = colorDistance(target, c1);
                        float d2 = colorDistance(target, c2), d3 = colorDistance(target, c3);
                        if (d1 < d0) { std::swap(d0, d1); std::swap(bestColor, c1); }
                        if (d2 < d0) { std::swap(d0, d2); std::swap(bestColor, c2); bestIdx = 2; }
                        else if (d2 < d1) { std::swap(d1, d2); std::swap(c1, c2); bestIdx = 2; }
                        if (d3 < d0) { std::swap(d0, d3); std::swap(bestColor, c3); bestIdx = 3; }
                        else if (d3 < d1) { std::swap(d1, d3); std::swap(c1, c3); bestIdx = 3; }
                        else if (d3 < d2) { std::swap(d2, d3); std::swap(c2, c3); bestIdx = 3; }
                    } else {
                        DdsEncoder::Color c2 = lerp(c0, c1, 0.5f);
                        float d0 = colorDistance(target, c0), d1 = colorDistance(target, c1);
                        float d2 = colorDistance(target, c2);
                        if (d1 < d0) { std::swap(d0, d1); std::swap(bestColor, c1); }
                        if (d2 < d0) { std::swap(d0, d2); std::swap(bestColor, c2); bestIdx = 2; }
                        else if (d2 < d1) { std::swap(d1, d2); std::swap(c1, c2); bestIdx = 2; }
                    }

                    error += colorDistance(target, bestColor);
                    indices |= static_cast<quint32>(bestIdx) << (2 * p);
                }

                if (error < bestError) {
                    bestError = error;
                    bestC0r = c0.r; bestC0g = c0.g; bestC0b = c0.b;
                    bestC1r = c1.r; bestC1g = c1.g; bestC1b = c1.b;
                    bestIndices = indices;
                }
            }
        }

        quint16 e0 = colorTo565({bestC0r, bestC0g, bestC0b});
        quint16 e1 = colorTo565({bestC1r, bestC1g, bestC1b});
        memcpy(out + 8, &e0, 2);
        memcpy(out + 10, &e1, 2);
        memcpy(out + 12, &bestIndices, 4);

    } else {
        float bestC0r = 0, bestC0g = 0, bestC0b = 0;
        float bestC1r = 0, bestC1g = 0, bestC1b = 0;
        quint32 bestIndices = 0;
        float bestError = 999999.0f;

        for (int i0 = 0; i0 < 16; ++i0) {
            for (int i1 = 0; i1 < 16; ++i1) {
                DdsEncoder::Color c0 = {colors[i0][0], colors[i0][1], colors[i0][2]};
                DdsEncoder::Color c1 = {colors[i1][0], colors[i1][1], colors[i1][2]};
                quint16 e0 = colorTo565(c0), e1 = colorTo565(c1);

                float error = 0;
                quint32 indices = 0;
                for (int p = 0; p < 16; ++p) {
                    DdsEncoder::Color target = {colors[p][0], colors[p][1], colors[p][2]};
                    DdsEncoder::Color bestColor = c0;
                    int bestIdx = 0;

                    if (e0 > e1) {
                        DdsEncoder::Color c2 = lerp(c0, c1, 1.0f / 3.0f);
                        DdsEncoder::Color c3 = lerp(c0, c1, 2.0f / 3.0f);
                        float d[4] = {colorDistance(target, c0), colorDistance(target, c1),
                                      colorDistance(target, c2), colorDistance(target, c3)};
                        for (int j = 1; j < 4; ++j) {
                            if (d[j] < d[bestIdx]) bestIdx = j;
                        }
                        bestColor = (bestIdx == 0) ? c0 : (bestIdx == 1) ? c1 :
                                    (bestIdx == 2) ? c2 : c3;
                    } else {
                        DdsEncoder::Color c2 = lerp(c0, c1, 0.5f);
                        float d[3] = {colorDistance(target, c0), colorDistance(target, c1),
                                      colorDistance(target, c2)};
                        for (int j = 1; j < 3; ++j) {
                            if (d[j] < d[bestIdx]) bestIdx = j;
                        }
                        bestColor = (bestIdx == 0) ? c0 : (bestIdx == 1) ? c1 : c2;
                    }

                    error += colorDistance(target, bestColor);
                    indices |= static_cast<quint32>(bestIdx) << (2 * p);
                }

                if (error < bestError) {
                    bestError = error;
                    bestC0r = c0.r; bestC0g = c0.g; bestC0b = c0.b;
                    bestC1r = c1.r; bestC1g = c1.g; bestC1b = c1.b;
                    bestIndices = indices;
                }
            }
        }

        quint16 e0 = colorTo565({bestC0r, bestC0g, bestC0b});
        quint16 e1 = colorTo565({bestC1r, bestC1g, bestC1b});
        memcpy(out, &e0, 2);
        memcpy(out + 2, &e1, 2);
        memcpy(out + 4, &bestIndices, 4);
    }
}

} // namespace

bool DdsEncoder::encode(const QImage& src, const QString& outPath, int dxtFormat)
{
    if (src.isNull()) return false;

    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    int width = img.width(), height = img.height();

    if (width == 0 || height == 0 || width > 16384 || height > 16384) return false;

    int paddedWidth = (width + 3) & ~3;
    int paddedHeight = (height + 3) & ~3;
    int numBlocksX = paddedWidth / 4;
    int numBlocksY = paddedHeight / 4;

    int blockSize = (dxtFormat == 5) ? 16 : 8;
    QByteArray data(numBlocksX * numBlocksY * blockSize, 0);

    const quint8* pixels = img.bits();
    for (int by = 0; by < numBlocksY; ++by) {
        for (int bx = 0; bx < numBlocksX; ++bx) {
            encodeDxtBlock(pixels, paddedWidth, by * 4, bx * 4,
                          reinterpret_cast<quint8*>(data.data() + (by * numBlocksX + bx) * blockSize), dxtFormat == 5);
        }
    }

    DdsHeader header{};
    header.dwSize = sizeof(DdsHeader);
    header.dwFlags = 0x1003;
    header.dwHeight = height;
    header.dwWidth = width;
    header.dwPitchOrLinearSize = numBlocksX * numBlocksY * blockSize;
    header.ddpfPixelFormat.dwSize = sizeof(quint32) * 8;
    header.ddpfPixelFormat.dwFlags = DDPF_FOURCC | DDPF_ALPHAPIXELS;
    header.ddpfPixelFormat.dwFourCC = (dxtFormat == 1) ? FOURCC_DXT1 : FOURCC_DXT5;
    header.ddpfPixelFormat.dwRGBBitCount = 0;
    header.ddpfPixelFormat.dwRBitMask = 0;
    header.ddpfPixelFormat.dwGBitMask = 0;
    header.ddpfPixelFormat.dwBBitMask = 0;
    header.ddpfPixelFormat.dwABitMask = 0;
    header.dwCaps = 0x1000;
    header.dwCaps2 = 0;

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    file.write("DDS ", 4);
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(data);
    file.close();

    return true;
}

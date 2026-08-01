#include "ddsdecoder.hpp"

#include <QFile>
#include <QtGlobal>

namespace {

inline quint32 expand565(quint16 c)
{
    const quint32 r = (c >> 11) & 0x1F;
    const quint32 g = (c >> 5) & 0x3F;
    const quint32 b = c & 0x1F;
    const quint32 rr = (r << 3) | (r >> 2);
    const quint32 gg = (g << 2) | (g >> 4);
    const quint32 bb = (b << 3) | (b >> 2);
    return (0xFFu << 24) | (rr << 16) | (gg << 8) | bb; // ARGB
}

inline int ctz32(quint32 v)
{
    int n = 0;
    while (!(v & 1) && n < 32) { v >>= 1; ++n; }
    return n;
}

} // namespace

QString DdsDecoder::fourCC(const QByteArray& data)
{
    if (data.size() < 84 || data.mid(0, 4) != QByteArray("DDS ", 4))
        return QString();
    const quint32* pf = reinterpret_cast<const quint32*>(data.constData() + 4 + 19 * 4);
    const quint32 pfFlags = pf[0];
    if (!(pfFlags & 0x4))
        return QString();
    return QString::fromLatin1(data.mid(4 + 20 * 4, 4));
}

QImage DdsDecoder::decodeFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QImage();
    const QByteArray data = file.readAll();
    file.close();
    return decode(data);
}

QImage DdsDecoder::decode(const QByteArray& data)
{
    if (data.size() < 128 || data.mid(0, 4) != QByteArray("DDS ", 4))
        return QImage();

    const quint32* h = reinterpret_cast<const quint32*>(data.constData() + 4);
    const quint32 height = h[2];
    const quint32 width = h[3];
    if (width == 0 || height == 0 || width > 16384 || height > 16384)
        return QImage();

    const quint32* pf = reinterpret_cast<const quint32*>(data.constData() + 4 + 19 * 4);
    const quint32 pfFlags = pf[0];
    const quint8* fourCC = reinterpret_cast<const quint8*>(&pf[1]);
    const quint32 rgbBitCount = pf[2];
    const quint32 rMask = pf[3], gMask = pf[4], bMask = pf[5], aMask = pf[6];

    if (pfFlags & 0x4)
    {
        const QByteArray cc(reinterpret_cast<const char*>(fourCC), 4);
        int format = -1;
        if (cc == "DXT1") format = 0;
        else if (cc == "DXT2" || cc == "DXT3") format = 1; // BC2
        else if (cc == "DXT4" || cc == "DXT5") format = 2; // BC3
        else if (cc == "BC4U" || cc == "ATI1") format = 3;
        else if (cc == "BC5U" || cc == "ATI2") format = 4;
        if (format < 0)
            return QImage();
        return decodeBlockCompressed(data, width, height, format);
    }

    return decodeUncompressed(data, width, height, rgbBitCount, rMask, gMask, bMask, aMask);
}

QImage DdsDecoder::decodeBlockCompressed(const QByteArray& data, quint32 width, quint32 height, int format)
{
    const quint8* pixels = reinterpret_cast<const quint8*>(data.constData()) + 128;
    const int blockSize = (format == 0 || format == 3) ? 8 : 16; // BC1/BC4 = 8 bytes

    QImage img(width, height, QImage::Format_ARGB32);
    for (quint32 y = 0; y < height; y += 4)
    {
        for (quint32 x = 0; x < width; x += 4)
        {
            const qint64 blockIndex = (y / 4) * (width / 4) + (x / 4);
            const quint8* block = pixels + blockIndex * blockSize;
            quint32 out[16];

            switch (format)
            {
            case 0: // BC1 / DXT1
                decodeBc1Block(block, out);
                break;
            case 1: // BC2 / DXT3
            {
                // 64-bit alpha (4 bits/texel) then a BC1 color block.
                quint64 alpha = 0;
                for (int i = 0; i < 8; ++i)
                    alpha |= static_cast<quint64>(block[i]) << (8 * i);
                quint32 colors[16];
                decodeBc1Block(block + 8, colors);
                for (int i = 0; i < 16; ++i)
                {
                    const int a4 = static_cast<int>((alpha >> (4 * i)) & 0xF);
                    out[i] = (static_cast<quint32>(a4 * 17) << 24) | (colors[i] & 0x00FFFFFF);
                }
                break;
            }
            case 2: // BC3 / DXT5
                decodeBc3Block(block, out, true);
                break;
            case 3: // BC4 (single-channel, replicated to RGB)
            {
                const quint8 a0 = block[0], a1 = block[1];
                quint8 a[8];
                a[0] = a0; a[1] = a1;
                if (a0 > a1)
                    for (int i = 0; i < 6; ++i)
                        a[2 + i] = static_cast<quint8>(((6 - i) * a0 + (1 + i) * a1) / 7);
                else
                {
                    for (int i = 0; i < 4; ++i)
                        a[2 + i] = static_cast<quint8>(((4 - i) * a0 + (1 + i) * a1) / 5);
                    a[6] = 0; a[7] = 255;
                }
                quint64 bits = 0;
                for (int i = 0; i < 6; ++i)
                    bits |= static_cast<quint64>(block[2 + i]) << (8 * i);
                for (int i = 0; i < 16; ++i)
                {
                    const int idx = static_cast<int>((bits >> (3 * i)) & 0x7);
                    const quint8 v = a[idx];
                    out[i] = (0xFFu << 24) | (v << 16) | (v << 8) | v;
                }
                break;
            }
            case 4: // BC5 (two channels; R stored, G stored, B=0)
            {
                // BC5 block: BC4 (R) followed by BC4 (G) for 8 bytes each.
                quint8 ar[8];
                {
                    const quint8 a0 = block[0], a1 = block[1];
                    ar[0] = a0; ar[1] = a1;
                    if (a0 > a1)
                        for (int i = 0; i < 6; ++i)
                            ar[2 + i] = static_cast<quint8>(((6 - i) * a0 + (1 + i) * a1) / 7);
                    else
                    {
                        for (int i = 0; i < 4; ++i)
                            ar[2 + i] = static_cast<quint8>(((4 - i) * a0 + (1 + i) * a1) / 5);
                        ar[6] = 0; ar[7] = 255;
                    }
                }
                quint8 ag[8];
                {
                    const quint8 a0 = block[8], a1 = block[9];
                    ag[0] = a0; ag[1] = a1;
                    if (a0 > a1)
                        for (int i = 0; i < 6; ++i)
                            ag[2 + i] = static_cast<quint8>(((6 - i) * a0 + (1 + i) * a1) / 7);
                    else
                    {
                        for (int i = 0; i < 4; ++i)
                            ag[2 + i] = static_cast<quint8>(((4 - i) * a0 + (1 + i) * a1) / 5);
                        ag[6] = 0; ag[7] = 255;
                    }
                }
                quint64 rbits = 0, gbits = 0;
                for (int i = 0; i < 6; ++i)
                {
                    rbits |= static_cast<quint64>(block[2 + i]) << (8 * i);
                    gbits |= static_cast<quint64>(block[10 + i]) << (8 * i);
                }
                for (int i = 0; i < 16; ++i)
                {
                    const int ri = static_cast<int>((rbits >> (3 * i)) & 0x7);
                    const int gi = static_cast<int>((gbits >> (3 * i)) & 0x7);
                    const quint8 r = ar[ri], g = ag[gi];
                    out[i] = (0xFFu << 24) | (r << 16) | (g << 8) | 0;
                }
                break;
            }
            default:
                return QImage();
            }

            for (int by = 0; by < 4; ++by)
            {
                for (int bx = 0; bx < 4; ++bx)
                {
                    const int px = static_cast<int>(x) + bx;
                    const int py = static_cast<int>(y) + by;
                    if (px < static_cast<int>(width) && py < static_cast<int>(height))
                        img.setPixel(px, py, out[by * 4 + bx]);
                }
            }
        }
    }
    return img;
}

void DdsDecoder::decodeBc1Block(const quint8* block, quint32* outRGBA)
{
    const quint16 c0 = *reinterpret_cast<const quint16*>(block);
    const quint16 c1 = *reinterpret_cast<const quint16*>(block + 2);
    quint32 colors[4];
    colors[0] = expand565(c0);
    colors[1] = expand565(c1);
    if (c0 > c1)
    {
        // 4-color mode: c2 = 2/3 c0 + 1/3 c1, c3 = 1/3 c0 + 2/3 c1
        const quint32 r0 = (colors[0] & 0xFF), g0 = (colors[0] >> 8) & 0xFF, b0 = (colors[0] >> 16) & 0xFF;
        const quint32 r1 = (colors[1] & 0xFF), g1 = (colors[1] >> 8) & 0xFF, b1 = (colors[1] >> 16) & 0xFF;
        colors[2] = (0xFFu << 24) | (((2 * b0 + b1) / 3) << 16) | (((2 * g0 + g1) / 3) << 8) | ((2 * r0 + r1) / 3);
        colors[3] = (0xFFu << 24) | (((b0 + 2 * b1) / 3) << 16) | (((g0 + 2 * g1) / 3) << 8) | ((r0 + 2 * r1) / 3);
    }
    else
    {
        // 3-color mode: c2 = average, c3 = transparent
        const quint32 r0 = (colors[0] & 0xFF), g0 = (colors[0] >> 8) & 0xFF, b0 = (colors[0] >> 16) & 0xFF;
        const quint32 r1 = (colors[1] & 0xFF), g1 = (colors[1] >> 8) & 0xFF, b1 = (colors[1] >> 16) & 0xFF;
        colors[2] = (0xFFu << 24) | (((b0 + b1) >> 1) << 16) | (((g0 + g1) >> 1) << 8) | ((r0 + r1) >> 1);
        colors[3] = 0x00000000u;
    }

    const quint32 indices = *reinterpret_cast<const quint32*>(block + 4);
    for (int i = 0; i < 16; ++i)
    {
        const int ci = (indices >> (2 * i)) & 0x3;
        outRGBA[i] = (colors[ci] & 0x00FFFFFF) | (ci == 3 && c0 <= c1 ? 0x00000000u : 0xFF000000u);
    }
}

void DdsDecoder::decodeBc3Block(const quint8* block, quint32* outRGBA, bool)
{
    const quint8 a0 = block[0];
    const quint8 a1 = block[1];
    quint8 a[8];
    a[0] = a0; a[1] = a1;
    if (a0 > a1)
    {
        for (int i = 0; i < 6; ++i)
            a[2 + i] = static_cast<quint8>(((6 - i) * a0 + (1 + i) * a1) / 7);
    }
    else
    {
        for (int i = 0; i < 4; ++i)
            a[2 + i] = static_cast<quint8>(((4 - i) * a0 + (1 + i) * a1) / 5);
        a[6] = 0; a[7] = 255;
    }
    quint64 bits = 0;
    for (int i = 0; i < 6; ++i)
        bits |= static_cast<quint64>(block[2 + i]) << (8 * i);
    quint8 alpha[16];
    for (int i = 0; i < 16; ++i)
        alpha[i] = a[(bits >> (3 * i)) & 0x7];

    quint32 colors[16];
    decodeBc1Block(block + 8, colors);
    for (int i = 0; i < 16; ++i)
        outRGBA[i] = (static_cast<quint32>(alpha[i]) << 24) | (colors[i] & 0x00FFFFFF);
}

QImage DdsDecoder::decodeUncompressed(const QByteArray& data, quint32 width, quint32 height,
                                      quint32 bitCount, quint32 rMask, quint32 gMask,
                                      quint32 bMask, quint32 aMask)
{
    const quint8* pixels = reinterpret_cast<const quint8*>(data.constData()) + 128;
    const bool hasAlpha = (aMask != 0);
    QImage img(width, height, QImage::Format_ARGB32);
    const int bpp = (bitCount + 7) / 8;
    if (bpp <= 0) return QImage();
    for (quint32 y = 0; y < height; ++y)
    {
        for (quint32 x = 0; x < width; ++x)
        {
            const quint8* p = pixels + (y * width + x) * bpp;
            quint32 val = 0;
            for (int i = 0; i < bpp; ++i)
                val |= static_cast<quint32>(p[i]) << (8 * i);
            const quint8 r = static_cast<quint8>(((val & rMask) >> ctz32(rMask)) & 0xFF);
            const quint8 g = static_cast<quint8>(((val & gMask) >> ctz32(gMask)) & 0xFF);
            const quint8 b = static_cast<quint8>(((val & bMask) >> ctz32(bMask)) & 0xFF);
            const quint8 a = hasAlpha ? static_cast<quint8>(((val & aMask) >> ctz32(aMask)) & 0xFF) : 255;
            img.setPixel(x, y, qRgba(r, g, b, a));
        }
    }
    return img;
}

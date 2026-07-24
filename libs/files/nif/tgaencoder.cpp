#include "tgaencoder.hpp"

#include <QFile>
#include <QDataStream>
#include <cstring>

bool TgaEncoder::encode(const QImage& src, const QString& outPath, bool useRle)
{
    if (src.isNull()) return false;

    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    const int w = img.width();
    const int h = img.height();
    if (w == 0 || h == 0) return false;

    const bool hasAlpha = !img.allGray();
    bool imageHasAlpha = false;
    for (int y = 0; y < h && !imageHasAlpha; ++y) {
        for (int x = 0; x < w && !imageHasAlpha; ++x) {
            if (qAlpha(img.pixel(x, y)) != 255) {
                imageHasAlpha = true;
            }
        }
    }

    const int bpp = imageHasAlpha ? 32 : 24;
    const int bytesPerPixel = bpp / 8;

    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    // Write 18-byte TGA header
    char header[18] = {};
    header[0] = 0; // ID length
    header[1] = 0; // Color map type (none)
    header[2] = useRle ? 10 : 2; // Image type: 2=uncompressed, 10=RLE
    // Bytes 3-7: color map spec (empty)
    // Bytes 8-9: x-origin
    header[10] = 0;
    header[11] = 0;
    // Bytes 10-11: y-origin
    header[12] = static_cast<char>(w & 0xFF);
    header[13] = static_cast<char>((w >> 8) & 0xFF);
    header[14] = static_cast<char>(h & 0xFF);
    header[15] = static_cast<char>((h >> 8) & 0xFF);
    header[16] = static_cast<char>(bpp);
    // Image descriptor: origin in upper-left (bit 5 = 0), alpha channel bits
    header[17] = imageHasAlpha ? 0x08 : 0x00;

    file.write(header, 18);

    if (useRle) {
        // RLE encoding
        QVector<quint8> pixels;
        pixels.reserve(w * h * bytesPerPixel);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                QRgb rgba = img.pixel(x, y);
                pixels.append(qBlue(rgba));
                pixels.append(qGreen(rgba));
                pixels.append(qRed(rgba));
                if (imageHasAlpha) {
                    pixels.append(qAlpha(rgba));
                }
            }
        }

        // Encode RLE packets
        QByteArray rleData;
        rleData.reserve(pixels.size());

        int pos = 0;
        const int totalPixels = w * h;
        while (pos < totalPixels) {
            // Look ahead for runs
            int runStart = pos;
            int runLen = 1;

            if (pos + 1 < totalPixels) {
                bool same = true;
                for (int b = 0; b < bytesPerPixel; ++b) {
                    if (pixels[(pos) * bytesPerPixel + b] != pixels[(pos + 1) * bytesPerPixel + b]) {
                        same = false;
                        break;
                    }
                }
                if (same) {
                    while (runLen < 128 && pos + runLen < totalPixels) {
                        bool match = true;
                        for (int b = 0; b < bytesPerPixel; ++b) {
                            if (pixels[runStart * bytesPerPixel + b] != pixels[(pos + runLen) * bytesPerPixel + b]) {
                                match = false;
                                break;
                            }
                        }
                        if (!match) break;
                        ++runLen;
                    }
                    quint8 packet = static_cast<quint8>(0x80 | (runLen - 1));
                    rleData.append(static_cast<char>(packet));
                    for (int b = 0; b < bytesPerPixel; ++b) {
                        rleData.append(static_cast<char>(pixels[runStart * bytesPerPixel + b]));
                    }
                    pos += runLen;
                } else {
                    // Look ahead for raw span
                    int rawLen = 1;
                    while (rawLen < 128 && pos + rawLen < totalPixels) {
                        bool sameAsNext = true;
                        for (int b = 0; b < bytesPerPixel; ++b) {
                            if (pixels[(pos + rawLen - 1) * bytesPerPixel + b] !=
                                pixels[(pos + rawLen) * bytesPerPixel + b]) {
                                sameAsNext = false;
                                break;
                            }
                        }
                        if (sameAsNext) break;
                        ++rawLen;
                    }
                    quint8 packet = static_cast<quint8>(rawLen - 1);
                    rleData.append(static_cast<char>(packet));
                    for (int i = 0; i < rawLen; ++i) {
                        for (int b = 0; b < bytesPerPixel; ++b) {
                            rleData.append(static_cast<char>(pixels[(pos + i) * bytesPerPixel + b]));
                        }
                    }
                    pos += rawLen;
                }
            } else {
                // Last pixel - raw packet
                quint8 packet = 0;
                rleData.append(static_cast<char>(packet));
                for (int b = 0; b < bytesPerPixel; ++b) {
                    rleData.append(static_cast<char>(pixels[runStart * bytesPerPixel + b]));
                }
                ++pos;
            }
        }

        file.write(rleData);
    } else {
        // Uncompressed
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                QRgb rgba = img.pixel(x, y);
                quint8 bgr[4];
                bgr[0] = qBlue(rgba);
                bgr[1] = qGreen(rgba);
                bgr[2] = qRed(rgba);
                if (imageHasAlpha) {
                    bgr[3] = qAlpha(rgba);
                }
                file.write(reinterpret_cast<const char*>(bgr), bytesPerPixel);
            }
        }
    }

    file.close();
    return true;
}

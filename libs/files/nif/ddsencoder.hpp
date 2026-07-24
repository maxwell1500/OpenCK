#pragma once

#include <QImage>
#include <QString>

class DdsEncoder {
public:
    struct Color { float r, g, b; };

    // Encode a QImage to DXT-compressed DDS format.
    // Returns true on success, writes binary DDS to 'outPath'.
    static bool encode(const QImage& src, const QString& outPath, int dxtFormat = 3);
    // dxtFormat: 1=DXT1, 3=DXT5 (default)
};

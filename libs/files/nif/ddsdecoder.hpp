#pragma once

#include <QImage>
#include <QString>
#include <QByteArray>

// DDS texture decoder for UI display. Decodes the common Creation Kit
// texture formats: uncompressed RGB(A), BC1/DXT1, BC2/DXT3, BC3/DXT5,
// BC4, and BC5. BC7 (bptc) is not decoded (returns empty image).
// This is the promoted, reusable version of the viewport's inline loader.
class DdsDecoder
{
public:
    // Decodes a DDS file into an ARGB32 QImage. Returns a null image if
    // the file is not a supported DDS.
    static QImage decodeFile(const QString& path);

    // Decodes DDS bytes (including the header) into an ARGB32 QImage.
    // Handles legacy 128-byte headers and the DX10 extended header (BC1-5).
    // Returns a null image on unsupported input.
    static QImage decode(const QByteArray& data);

    // The FourCC format identifier of the data, or empty for uncompressed.
    static QString fourCC(const QByteArray& data);

private:
    static QImage decodeBlockCompressed(const QByteArray& data, quint32 width, quint32 height,
                                        int format, int dataOffset); // 0=BC1 1=BC2 2=BC3 3=BC4 4=BC5
    static QImage decodeUncompressed(const QByteArray& data, quint32 width, quint32 height,
                                     quint32 bitCount, quint32 rMask, quint32 gMask,
                                     quint32 bMask, quint32 aMask);
    static void decodeBc1Block(const quint8* block, quint32* outRGBA);
    static void decodeBc3Block(const quint8* block, quint32* outRGBA, bool dxt5); // dxt5 == BC3
};

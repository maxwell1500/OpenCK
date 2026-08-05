#include "ba2archive.hpp"

#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#include <cstring>

#include <zlib.h>

#include "logger.hpp"

namespace {

constexpr quint32 BA2_MAGIC = 0x20584142; // 'BA2 ' in little-endian
constexpr quint32 BTDX_MAGIC = 0x58445442; // 'BTDX'
constexpr quint32 TYPE_GNRL = 0x4C524E47;  // 'GNRL'
constexpr quint32 TYPE_DX10 = 0x30315844;  // 'DX10'
constexpr quint32 BAADF00D = 0xBAADF00D;

// Raw LZ4 block decoder (the format lz4_flex uses for Starfield v3 archives).
bool lz4BlockDecode(const uchar* src, qsizetype srcLen, uchar* dst, qsizetype dstLen)
{
    qsizetype si = 0;
    qsizetype di = 0;
    while (si < srcLen)
    {
        if (si + 1 > srcLen) return false;
        const uchar token = src[si++];
        qsizetype litLen = (token >> 4) & 0x0F;
        if (litLen == 15)
        {
            uchar b;
            do {
                if (si >= srcLen) return false;
                b = src[si++];
                litLen += b;
            } while (b == 255);
        }
        if (litLen > 0)
        {
            if (si + litLen > srcLen) return false;
            if (di + litLen > dstLen) return false;
            memcpy(dst + di, src + si, static_cast<size_t>(litLen));
            si += litLen;
            di += litLen;
        }
        if (si >= srcLen) break; // end of block
        if (si + 2 > srcLen) return false;
        const quint32 matchOffset = static_cast<quint32>(src[si]) | (static_cast<quint32>(src[si + 1]) << 8);
        si += 2;
        if (matchOffset == 0) return false;
        qsizetype matchLen = (token & 0x0F);
        if (matchLen == 15)
        {
            uchar b;
            do {
                if (si >= srcLen) return false;
                b = src[si++];
                matchLen += b;
            } while (b == 255);
        }
        matchLen += 4;
        if (di + matchLen > dstLen) return false;
        if (matchOffset > static_cast<quint32>(di)) return false;
        for (qsizetype i = 0; i < matchLen; ++i)
            dst[di + i] = dst[di + i - matchOffset];
        di += matchLen;
    }
    return di == dstLen;
}

// DDS constants for header reconstruction (DDS_HEADER + DDS_HEADER_DXT10).
constexpr quint32 DDSD_CAPS       = 0x1;
constexpr quint32 DDSD_HEIGHT     = 0x2;
constexpr quint32 DDSD_WIDTH      = 0x4;
constexpr quint32 DDSD_PITCH      = 0x8;
constexpr quint32 DDSD_PIXELFORMAT= 0x1000;
constexpr quint32 DDSD_MIPMAPCOUNT= 0x20000;
constexpr quint32 DDSD_LINEARSIZE = 0x80000;
constexpr quint32 DDPF_FOURCC     = 0x4;
constexpr quint32 FOURCC_DX10     = 0x30315844; // 'DX10'
constexpr quint32 DDSCAPS_COMPLEX = 0x8;
constexpr quint32 DDSCAPS_TEXTURE = 0x1000;
constexpr quint32 DDSCAPS_MIPMAP  = 0x400000;
constexpr quint32 DDSCAPS2_CUBEMAP_ALLFACES = 0xFE00;

void putU32(QByteArray& out, quint32 v)
{
    out.append(static_cast<char>(v & 0xFF));
    out.append(static_cast<char>((v >> 8) & 0xFF));
    out.append(static_cast<char>((v >> 16) & 0xFF));
    out.append(static_cast<char>((v >> 24) & 0xFF));
}

// Bytes per 4x4 block for block-compressed DXGI formats, or 0 otherwise.
quint32 blockBytes(quint32 dxgi)
{
    if ((dxgi >= 70 && dxgi <= 72) || (dxgi >= 79 && dxgi <= 81)) return 8;
    if ((dxgi >= 73 && dxgi <= 78) || (dxgi >= 82 && dxgi <= 84) || (dxgi >= 94 && dxgi <= 99)) return 16;
    return 0;
}

// Bits per pixel for the uncompressed DXGI formats, or 0 if unknown.
quint32 bitsPerPixel(quint32 dxgi)
{
    if ((dxgi >= 23 && dxgi <= 38) || (dxgi >= 87 && dxgi <= 93)) return 32;
    if ((dxgi >= 48 && dxgi <= 59) || dxgi == 85 || dxgi == 86) return 16;
    if (dxgi >= 60 && dxgi <= 65) return 8;
    return 0;
}

// Build the 148-byte DDS header (magic + DDS_HEADER + DDS_HEADER_DXT10).
QByteArray buildDdsHeader(quint32 width, quint32 height, quint32 mips, quint32 dxgi, bool cubemap)
{
    const quint32 mipCount = qMax(mips, 1u);
    quint32 pitchOrLinear = 0;
    quint32 sizeFlag = 0;
    if (const quint32 block = blockBytes(dxgi); block != 0)
    {
        const quint64 bw = (width + 3) / 4;
        const quint64 bh = (height + 3) / 4;
        pitchOrLinear = static_cast<quint32>(bw * bh * block);
        sizeFlag = DDSD_LINEARSIZE;
    }
    else if (const quint32 bits = bitsPerPixel(dxgi); bits != 0)
    {
        pitchOrLinear = (width * bits + 7) / 8;
        sizeFlag = DDSD_PITCH;
    }

    quint32 flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | sizeFlag;
    if (mipCount > 1) flags |= DDSD_MIPMAPCOUNT;

    quint32 caps = DDSCAPS_TEXTURE;
    if (mipCount > 1) caps |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    if (cubemap) caps |= DDSCAPS_COMPLEX;
    const quint32 caps2 = cubemap ? DDSCAPS2_CUBEMAP_ALLFACES : 0;
    const quint32 miscFlag = cubemap ? 0x4 : 0;

    QByteArray h;
    putU32(h, 0x20534444);            // 'DDS '
    putU32(h, 124);                    // dwSize
    putU32(h, flags);
    putU32(h, height);
    putU32(h, width);
    putU32(h, pitchOrLinear);
    putU32(h, 0);                      // dwDepth
    putU32(h, mipCount);
    for (int i = 0; i < 11; ++i) putU32(h, 0);
    putU32(h, 32);                     // pf.dwSize
    putU32(h, DDPF_FOURCC);
    putU32(h, FOURCC_DX10);
    for (int i = 0; i < 5; ++i) putU32(h, 0);
    putU32(h, caps);
    putU32(h, caps2);
    for (int i = 0; i < 3; ++i) putU32(h, 0);
    putU32(h, dxgi);                   // dxgiFormat
    putU32(h, 3);                      // resourceDimension (D3D10_RESOURCE_DIMENSION_TEXTURE2D)
    putU32(h, miscFlag);
    putU32(h, 1);                      // arraySize
    putU32(h, 0);                      // miscFlags2
    return h;
}

// Parse a DDS texture file's header: dimensions, mip count, DXGI format,
// cubemap flag, and the start offset of the pixel data.
struct ParsedDdsTexture {
    quint16 width = 0;
    quint16 height = 0;
    quint8 numMips = 0;
    quint8 format = 0;   // DXGI format code
    quint8 flags = 0;    // bit 0 = cubemap
    quint8 tileMode = 8; // linear
    int dataOffset = 148;
};

bool parseDdsTexture(const QByteArray& data, ParsedDdsTexture& out)
{
    if (data.size() < 148 || data.mid(0, 4) != QByteArray("DDS ", 4))
        return false;
    const quint32* h = reinterpret_cast<const quint32*>(data.constData() + 4);
    const quint32 height = h[2];
    const quint32 width = h[3];
    const quint32 mipRaw = h[6];
    if (width == 0 || height == 0 || width > 16384 || height > 16384 || width > 65535 || height > 65535)
        return false;

    const quint32* pf = reinterpret_cast<const quint32*>(data.constData() + 4 + 19 * 4);
    const quint32 pfFlags = pf[0];
    const quint32 caps2 = h[27];

    const bool isDx10 = (pfFlags & 0x4) && data.mid(4 + 20 * 4, 4) == QByteArray("DX10", 4);
    quint32 dxgi = 0;
    if (isDx10)
    {
        dxgi = *reinterpret_cast<const quint32*>(data.constData() + 0x80);
        if (dxgi == 0 || dxgi > 0xFF)
            return false;
        out.dataOffset = 148;
        out.flags = ((*reinterpret_cast<const quint32*>(data.constData() + 0x88)) & 0x4) ? 1 : 0;
    }
    else
    {
        const QByteArray cc = data.mid(4 + 20 * 4, 4);
        if (cc == "DXT1") dxgi = 71;
        else if (cc == "DXT3") dxgi = 74;
        else if (cc == "DXT5") dxgi = 77;
        else if (cc == "BC4U" || cc == "ATI1") dxgi = 80;
        else if (cc == "BC5U" || cc == "ATI2") dxgi = 83;
        else if (cc == "BC4S") dxgi = 81;
        else if (cc == "BC5S") dxgi = 84;
        else
            return false;
        out.dataOffset = 128;
        out.flags = (caps2 & 0xFE00) ? 1 : 0;
    }

    const quint32 mipCount = (h[1] & 0x20000) ? qMax(mipRaw, 1u) : 1u;
    out.width = static_cast<quint16>(width);
    out.height = static_cast<quint16>(height);
    out.numMips = static_cast<quint8>(qMin(mipCount, 255u));
    out.format = static_cast<quint8>(dxgi);
    return true;
}

} // namespace

Ba2Archive::Ba2Archive()
{
}

Ba2Archive::~Ba2Archive()
{
    if (mFile) {
        if (mMappedData) mFile->unmap(mMappedData);
        delete mFile;
    }
}

quint32 Ba2Archive::readU32(const uchar* p)
{
    quint32 v = 0;
    memcpy(&v, p, sizeof(v));
    return v;
}

quint64 Ba2Archive::readU64(const uchar* p)
{
    quint64 v = 0;
    memcpy(&v, p, sizeof(v));
    return v;
}

void Ba2Archive::failOpen()
{
    if (mFile) {
        if (mMappedData) mFile->unmap(mMappedData);
        mMappedData = nullptr;
        delete mFile;
        mFile = nullptr;
    }
    mEntries.clear();
}

bool Ba2Archive::open(const QString& path)
{
    mFile = new QFile(path);
    if (!mFile->open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open BA2 archive: %1").arg(path));
        delete mFile;
        mFile = nullptr;
        return false;
    }

    mFileSize = mFile->size();
    mMappedData = mFile->map(0, mFileSize);
    if (!mMappedData) {
        LOG_ERROR(QString("Failed to memory-map BA2 file: %1").arg(path));
        delete mFile;
        mFile = nullptr;
        return false;
    }

    if (mFileSize < 36) {
        LOG_ERROR("BA2 archive too small");
        failOpen();
        return false;
    }

    mName = QFileInfo(path).completeBaseName();

    const quint32 magic = readU32(mMappedData);
    if (magic == BTDX_MAGIC)
        return openBtdx(path);
    if (magic == BA2_MAGIC)
        return openLegacy(path);

    LOG_ERROR(QString("Invalid BA2 magic: 0x%1").arg(magic, 8, 16, QChar('0')));
    failOpen();
    return false;
}

bool Ba2Archive::openBtdx(const QString&)
{
    const quint32 version = readU32(mMappedData + 4);
    const quint32 type = readU32(mMappedData + 8);
    mFileCount = readU32(mMappedData + 12);
    const quint64 nameOffs = readU64(mMappedData + 16);

    // v1 / v7 / v8 use a 24-byte header; v2 uses 32 bytes; v3 uses 36 bytes
    // (an extra compression-method u32: 0 = zlib, 3 = raw LZ4 block).
    quint32 hdrSize = 24;
    if (version == 2) hdrSize = 32;
    else if (version == 3) hdrSize = 36;

    mIsDx10 = (type == TYPE_DX10);
    mUseLz4 = false;
    if (version == 3)
    {
        const quint32 method = readU32(mMappedData + 32);
        if (method == 3) mUseLz4 = true;
        else if (method != 0)
        {
            LOG_ERROR(QString("BA2 v3 compression method %1 not supported").arg(method));
            failOpen();
            return false;
        }
    }

    LOG_INFO(QString("BA2 archive: %1 v%2 type=%3 files=%4 nameOffs=%5%6")
        .arg(mName).arg(version).arg(type, 4, 16, QChar('0')).arg(mFileCount).arg(nameOffs)
        .arg(mUseLz4 ? QString(" codec=LZ4") : QString(" codec=zlib")));

    if (type == TYPE_GNRL)
        return parseBtdxGeneral(hdrSize, nameOffs);
    if (type == TYPE_DX10)
        return parseBtdxTextures(hdrSize, nameOffs);

    LOG_ERROR(QString("BA2 archive type 0x%1 not supported").arg(type, 8, 16, QChar('0')));
    failOpen();
    return false;
}

bool Ba2Archive::parseBtdxGeneral(quint32 hdrSize, quint64 nameOffs)
{
    mEntries.clear();
    mEntries.reserve(mFileCount);

    // File names are a u16-length-prefixed table (length includes the NUL).
    QVector<QString> names;
    names.reserve(mFileCount);
    quint64 pos = nameOffs;
    for (quint32 i = 0; i < mFileCount && pos + 2 <= mFileSize; ++i)
    {
        const quint16 len = static_cast<quint16>(readU32(mMappedData + pos));
        pos += 2;
        if (pos + len > mFileSize) break;
        QString name = QString::fromLatin1(
            reinterpret_cast<const char*>(mMappedData + pos), len);
        while (name.endsWith(QLatin1Char('\0'))) name.chop(1);
        names.append(name);
        pos += len;
    }
    if (names.size() < static_cast<int>(mFileCount))
    {
        LOG_ERROR(QString("BA2 name table truncated (%1 of %2 names)")
            .arg(names.size()).arg(mFileCount));
        failOpen();
        return false;
    }

    // 36-byte file records: CRC32 base, ext FourCC, CRC32 dir, flags,
    // u64 data offset, packed size, unpacked size, 0xBAADF00D.
    for (quint32 i = 0; i < mFileCount; ++i)
    {
        const quint64 rec = static_cast<quint64>(hdrSize) + static_cast<quint64>(i) * 36;
        if (rec + 36 > mFileSize) break;
        const uchar* p = mMappedData + rec;
        const quint32 sentinel = readU32(p + 32);
        if (sentinel != BAADF00D)
        {
            LOG_WARNING(QString("BA2 record %1 has bad sentinel 0x%2")
                .arg(i).arg(sentinel, 8, 16, QChar('0')));
            continue;
        }

        Ba2FileEntry entry;
        entry.relativePath = names[i];
        entry.fileOffset = readU64(p + 16);
        entry.compressedSize = readU32(p + 24);
        entry.uncompressedSize = readU32(p + 28);
        entry.compressed = (entry.compressedSize != 0);
        mEntries.append(entry);
    }

    LOG_INFO(QString("Parsed %1 file entries from BA2 archive").arg(mEntries.size()));
    return !mEntries.isEmpty();
}

bool Ba2Archive::parseBtdxTextures(quint32 hdrSize, quint64 nameOffs)
{
    mEntries.clear();
    mDx10Entries.clear();
    mEntries.reserve(mFileCount);
    mDx10Entries.reserve(mFileCount);

    // File names are a u16-length-prefixed table (length includes the NUL),
    // identical to the GNRL name table.
    QVector<QString> names;
    names.reserve(mFileCount);
    quint64 pos = nameOffs;
    for (quint32 i = 0; i < mFileCount && pos + 2 <= mFileSize; ++i)
    {
        const quint16 len = static_cast<quint16>(readU32(mMappedData + pos));
        pos += 2;
        if (pos + len > mFileSize) break;
        QString name = QString::fromLatin1(
            reinterpret_cast<const char*>(mMappedData + pos), len);
        while (name.endsWith(QLatin1Char('\0'))) name.chop(1);
        names.append(name);
        pos += len;
    }
    if (names.size() < static_cast<int>(mFileCount))
    {
        LOG_ERROR(QString("BA2 texture name table truncated (%1 of %2 names)")
            .arg(names.size()).arg(mFileCount));
        failOpen();
        return false;
    }

    // Each file entry is a 24-byte DX10 record followed by N 24-byte chunk
    // records. The DX10 record holds texture metadata; chunks point at the
    // per-mip-range payloads.
    quint64 rec = hdrSize;
    for (quint32 i = 0; i < mFileCount; ++i)
    {
        if (rec + 24 > static_cast<quint64>(mFileSize)) break;
        const uchar* p = mMappedData + rec;
        const quint8 numChunks = p[13];
        const quint16 chunkHdrSize = static_cast<quint16>(readU32(p + 14));
        if (chunkHdrSize != 24)
        {
            LOG_WARNING(QString("BA2 texture record %1 has unexpected chunk header size %2")
                .arg(i).arg(chunkHdrSize));
            failOpen();
            return false;
        }

        Ba2Dx10Entry entry;
        entry.relativePath = names[i];
        entry.height = static_cast<quint16>(readU32(p + 16));
        entry.width = static_cast<quint16>(readU32(p + 18));
        entry.numMips = p[20];
        entry.format = p[21];
        entry.flags = p[22];
        entry.tileMode = p[23];
        rec += 24;

        for (quint32 c = 0; c < numChunks; ++c)
        {
            if (rec + 24 > static_cast<quint64>(mFileSize)) break;
            const uchar* cp = mMappedData + rec;
            Ba2Dx10Chunk chunk;
            chunk.fileOffset = readU64(cp);
            chunk.packedSize = readU32(cp + 8);
            chunk.unpackedSize = readU32(cp + 12);
            chunk.startMip = static_cast<quint16>(readU32(cp + 16));
            chunk.endMip = static_cast<quint16>(readU32(cp + 18));
            entry.chunks.append(chunk);
            rec += 24;
        }

        mDx10Entries.append(entry);
        // Mirror the path into the generic entry list so entries()/fileCount()
        // keep working for the archive browser.
        Ba2FileEntry fe;
        fe.relativePath = names[i];
        mEntries.append(fe);
    }

    if (mDx10Entries.size() < static_cast<int>(mFileCount))
    {
        LOG_ERROR(QString("BA2 texture record table truncated (%1 of %2)")
            .arg(mDx10Entries.size()).arg(mFileCount));
        failOpen();
        return false;
    }

    LOG_INFO(QString("Parsed %1 texture entries from BA2 archive").arg(mDx10Entries.size()));
    return !mDx10Entries.isEmpty();
}

bool Ba2Archive::openLegacy(const QString&)
{
    // Self-written "BA2 " archives (the legacy create() format).
    mFileTableOffset = readU32(mMappedData + 8);
    mFileCount = readU32(mMappedData + 12);

    mEntries.clear();
    mEntries.reserve(mFileCount);

    quint32 pos = mFileTableOffset;
    for (quint32 i = 0; i < mFileCount && pos + 24 <= mFileSize; ++i) {
        Ba2FileEntry entry;

        if (pos + 4 > mFileSize) break;
        quint32 nameLen = readU32(mMappedData + pos);
        pos += 4;

        if (pos + nameLen > mFileSize) break;
        entry.relativePath = QString::fromLatin1(reinterpret_cast<const char*>(mMappedData + pos), nameLen);
        pos += nameLen;

        if (pos + 4 > mFileSize) break;
        entry.compressedSize = readU32(mMappedData + pos);
        pos += 4;

        if (pos + 4 > mFileSize) break;
        entry.uncompressedSize = readU32(mMappedData + pos);
        pos += 4;

        if (pos + 4 > mFileSize) break;
        quint32 flags = readU32(mMappedData + pos);
        entry.compressed = (flags & 0x1) != 0;
        pos += 4;

        if (pos + 8 > mFileSize) break;
        entry.fileOffset = readU64(mMappedData + pos);
        pos += 8;

        mEntries.append(entry);
    }

    LOG_INFO(QString("Parsed %1 file entries from BA2 archive").arg(mEntries.size()));
    return true;
}

bool Ba2Archive::extract(quint32 index, const QString& outputPath) const
{
    if (mIsDx10)
        return extractTexture(index, outputPath);

    if (index >= mEntries.size()) {
        LOG_ERROR(QString("BA2 extract: index %1 out of range").arg(index));
        return false;
    }

    const auto& entry = mEntries[index];

    // Bounds check against the on-disk extent (compressed size for packed
    // files, unpacked size for stored files).
    const quint64 onDiskSize = entry.compressed
        ? static_cast<quint64>(entry.compressedSize)
        : static_cast<quint64>(entry.uncompressedSize);
    if (entry.fileOffset + onDiskSize > mFileSize) {
        LOG_ERROR(QString("BA2 extract: file data out of range for %1")
                     .arg(entry.relativePath));
        return false;
    }

    // Create output directory
    QFileInfo outInfo(outputPath);
    if (!outInfo.dir().mkpath(".")) {
        LOG_ERROR(QString("BA2 extract: cannot create directory for %1").arg(outputPath));
        return false;
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("BA2 extract: cannot write to %1").arg(outputPath));
        return false;
    }

    const uchar* fileDataPtr = mMappedData + static_cast<qint64>(entry.fileOffset);

    if (entry.compressed) {
        quint32 compressedSize = entry.compressedSize;
        if (compressedSize == 0) {
            if (index + 1 < mEntries.size()) {
                compressedSize = static_cast<quint32>(mEntries[index + 1].fileOffset - entry.fileOffset);
            } else {
                compressedSize = static_cast<quint32>(mFileSize - entry.fileOffset);
            }
        }

        QByteArray decompressedBuf;
        if (!decompressChunk(fileDataPtr, compressedSize, entry.uncompressedSize,
                             decompressedBuf, mUseLz4))
        {
            return false;
        }
        outFile.write(decompressedBuf);
    } else {
        outFile.write(reinterpret_cast<const char*>(fileDataPtr), entry.uncompressedSize);
    }

    outFile.close();
    LOG_INFO(QString("Extracted: %1 -> %2").arg(entry.relativePath).arg(outputPath));
    return true;
}

bool Ba2Archive::decompressChunk(const uchar* data, quint32 packedSize, quint32 unpackedSize,
                                 QByteArray& out, bool lz4)
{
    out.clear();
    if (packedSize == 0)
    {
        // stored uncompressed
        out = QByteArray(reinterpret_cast<const char*>(data), unpackedSize);
        return true;
    }

    out.resize(unpackedSize);
    if (lz4)
    {
        if (!lz4BlockDecode(data, packedSize, reinterpret_cast<uchar*>(out.data()), unpackedSize))
        {
            LOG_ERROR(QString("BA2 LZ4 decompression failed (%1 -> %2)")
                .arg(packedSize).arg(unpackedSize));
            out.clear();
            return false;
        }
        return true;
    }

    uLongf destLen = unpackedSize;
    const int ret = uncompress(reinterpret_cast<Bytef*>(out.data()), &destLen,
                               reinterpret_cast<const Bytef*>(data), packedSize);
    if (ret != Z_OK)
    {
        LOG_ERROR(QString("BA2 zlib decompression failed: %1").arg(ret));
        out.clear();
        return false;
    }
    out.resize(static_cast<int>(destLen));
    return true;
}

bool Ba2Archive::extractTexture(quint32 index, const QString& outputPath) const
{
    if (index >= mDx10Entries.size())
    {
        LOG_ERROR(QString("BA2 texture extract: index %1 out of range").arg(index));
        return false;
    }
    const auto& entry = mDx10Entries[index];
    if (entry.width == 0 || entry.height == 0)
    {
        LOG_ERROR(QString("BA2 texture extract: %1 has zero dimensions").arg(entry.relativePath));
        return false;
    }
    if (entry.chunks.isEmpty())
    {
        LOG_ERROR(QString("BA2 texture extract: %1 has no chunks").arg(entry.relativePath));
        return false;
    }
    if (entry.tileMode != 8)
    {
        LOG_ERROR(QString("BA2 texture extract: %1 uses non-linear tile mode %2")
            .arg(entry.relativePath).arg(entry.tileMode));
        return false;
    }

    const bool cubemap = (entry.flags & 0x1) != 0;
    QByteArray dds = buildDdsHeader(entry.width, entry.height, entry.numMips,
                                    entry.format, cubemap);

    for (const auto& chunk : entry.chunks)
    {
        const quint64 stored = (chunk.packedSize != 0) ? chunk.packedSize : chunk.unpackedSize;
        if (chunk.fileOffset + stored > static_cast<quint64>(mFileSize))
        {
            LOG_ERROR(QString("BA2 texture extract: chunk data out of range for %1")
                .arg(entry.relativePath));
            return false;
        }
        QByteArray data;
        if (!decompressChunk(mMappedData + static_cast<qint64>(chunk.fileOffset),
                             chunk.packedSize, chunk.unpackedSize, data, mUseLz4))
        {
            return false;
        }
        dds.append(data);
    }

    QFileInfo outInfo(outputPath);
    if (!outInfo.dir().mkpath("."))
    {
        LOG_ERROR(QString("BA2 texture extract: cannot create directory for %1").arg(outputPath));
        return false;
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly))
    {
        LOG_ERROR(QString("BA2 texture extract: cannot write to %1").arg(outputPath));
        return false;
    }
    outFile.write(dds);
    outFile.close();
    LOG_INFO(QString("Extracted texture: %1 -> %2 (%3 bytes)")
        .arg(entry.relativePath).arg(outputPath).arg(dds.size()));
    return true;
}

bool Ba2Archive::create(const QStringList& filePaths, const QString& outputPath,
                         bool compress, const QString& archiveType)
{
    if (filePaths.isEmpty()) {
        LOG_ERROR("BA2 create: no files to archive");
        return false;
    }

    if (archiveType.toUpper() == "DX10")
        return createDx10(filePaths, outputPath, compress);

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("BA2 create: cannot open output file: %1").arg(outputPath));
        return false;
    }

    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);

    QFileInfo outInfo(outputPath);
    QString baseDir = outInfo.absolutePath();

    struct InputFile {
        QString relativePath; // forward-slash, lowercased for CRC
        QString onDiskPath;
        QByteArray data;
    };
    QVector<InputFile> inputs;
    inputs.reserve(filePaths.size());

    for (const QString& filePath : filePaths) {
        QFileInfo fi(filePath);
        QString relPath = fi.fileName();
        QString absPath = fi.absoluteFilePath();
        if (absPath.startsWith(baseDir, Qt::CaseInsensitive)) {
            relPath = absPath.mid(baseDir.length() + 1).replace('\\', '/');
        }

        QFile inFile(filePath);
        if (!inFile.open(QIODevice::ReadOnly)) {
            LOG_WARNING(QString("BA2 create: skipping unreadable file: %1").arg(filePath));
            continue;
        }
        InputFile input;
        input.relativePath = relPath;
        input.onDiskPath = filePath;
        input.data = inFile.readAll();
        inFile.close();
        if (input.data.isEmpty()) {
            LOG_WARNING(QString("BA2 create: skipping empty file: %1").arg(filePath));
            continue;
        }
        inputs.append(input);
    }

    if (inputs.isEmpty()) {
        LOG_ERROR("BA2 create: no valid files to archive");
        outFile.close();
        return false;
    }

    const quint32 n = static_cast<quint32>(inputs.size());
    constexpr quint32 HDR_SIZE = 32;
    constexpr quint32 RECORD_SIZE = 36;
    const quint32 recordsSize = n * RECORD_SIZE;
    const quint32 nameTableOffset = HDR_SIZE + recordsSize;

    // Name table: u16 length (incl NUL) + bytes, per file.
    QVector<QByteArray> nameBytes;
    nameBytes.reserve(n);
    quint32 nameTableSize = 0;
    for (const auto& input : inputs) {
        QByteArray nb = input.relativePath.toLower().toUtf8();
        nb.append('\0');
        nameBytes.append(nb);
        nameTableSize += 2 + static_cast<quint32>(nb.size());
    }

    // Data section follows the name table.
    quint64 dataOffset = static_cast<quint64>(nameTableOffset) + nameTableSize;

    struct TableEntry {
        QByteArray name;       // on-disk table name (incl NUL)
        QByteArray dirLower;
        QByteArray baseLower;
        QByteArray extLower;
        quint32 flags = 0;
        quint64 fileOffset = 0;
        quint32 packedSize = 0;
        quint32 unpackedSize = 0;
        QByteArray fileData;
    };
    QVector<TableEntry> tableEntries;
    tableEntries.reserve(n);

    for (int i = 0; i < inputs.size(); ++i) {
        const InputFile& input = inputs[i];
        TableEntry entry;
        entry.name = nameBytes[i];
        entry.unpackedSize = static_cast<quint32>(input.data.size());

        const QByteArray lower = input.relativePath.toLower().toUtf8();
        int slash = lower.lastIndexOf('/');
        entry.dirLower = slash >= 0 ? lower.left(slash) : QByteArray();
        QByteArray fileName = slash >= 0 ? lower.mid(slash + 1) : lower;
        int dot = fileName.lastIndexOf('.');
        entry.baseLower = dot > 0 ? fileName.left(dot) : fileName;
        entry.extLower = dot > 0 ? fileName.mid(dot + 1) : QByteArray();

        if (compress) {
            QByteArray compressed;
            compressed.resize(input.data.size() + input.data.size() / 100 + 600);
            uLongf destLen = compressed.size();
            int ret = compress2(reinterpret_cast<Bytef*>(compressed.data()), &destLen,
                                reinterpret_cast<const Bytef*>(input.data.constData()),
                                input.data.size(), Z_DEFAULT_COMPRESSION);
            if (ret == Z_OK && destLen < static_cast<uLongf>(input.data.size())) {
                compressed.resize(destLen);
                entry.fileData = compressed;
                entry.packedSize = static_cast<quint32>(destLen);
                entry.flags = 0; // stored-compressed: packedSize != 0 signals zlib
            } else {
                entry.fileData = input.data;
                entry.packedSize = 0; // stored uncompressed
                entry.flags = 0;
            }
        } else {
            entry.fileData = input.data;
            entry.packedSize = 0;
            entry.flags = 0;
        }

        entry.fileOffset = dataOffset;
        dataOffset += (entry.packedSize != 0) ? entry.packedSize : entry.unpackedSize;
        tableEntries.append(entry);
    }

    // --- BTDX v2 header (32 bytes) ---
    out.writeRawData("BTDX", 4);                 // magic
    quint32 version = 2;
    out << version;
    QByteArray typeBytes = archiveType.toLatin1().leftJustified(4, ' ', true);
    out.writeRawData(typeBytes.data(), 4);       // GNRL
    out << n;                                    // file count
    out << static_cast<quint64>(nameTableOffset);
    quint32 one = 1;
    out << one;
    quint32 zero = 0;
    out << zero;

    // File records.
    for (const auto& entry : tableEntries) {
        out << static_cast<quint32>(crc32(0, reinterpret_cast<const Bytef*>(entry.baseLower.constData()),
                     static_cast<uInt>(entry.baseLower.size())));
        QByteArray ext = entry.extLower;
        ext.resize(4, '\0');
        out.writeRawData(ext.constData(), 4);
        out << static_cast<quint32>(crc32(0, reinterpret_cast<const Bytef*>(entry.dirLower.constData()),
                     static_cast<uInt>(entry.dirLower.size())));
        out << entry.flags;
        out << entry.fileOffset;
        out << entry.packedSize;
        out << entry.unpackedSize;
        out << static_cast<quint32>(BAADF00D);
    }

    // Name table.
    for (const auto& entry : tableEntries) {
        quint16 len = static_cast<quint16>(entry.name.size());
        out << len;
        out.writeRawData(entry.name.constData(), entry.name.size());
    }

    // File data.
    for (const auto& entry : tableEntries) {
        outFile.write(entry.fileData);
    }

    outFile.close();

    LOG_INFO(QString("BA2 create: wrote %1 files to %2 (type=%3, compressed=%4)")
                .arg(n).arg(outputPath).arg(archiveType).arg(compress));
    return true;
}

bool Ba2Archive::createDx10(const QStringList& filePaths, const QString& outputPath, bool compress)
{
    struct InputTexture {
        QString relativePath;
        ParsedDdsTexture info;
        QByteArray data; // full DDS bytes
    };
    QVector<InputTexture> inputs;

    QFileInfo outInfo(outputPath);
    const QString baseDir = outInfo.absolutePath();

    for (const QString& filePath : filePaths)
    {
        QFileInfo fi(filePath);
        QString relPath = fi.fileName();
        const QString absPath = fi.absoluteFilePath();
        if (absPath.startsWith(baseDir, Qt::CaseInsensitive))
            relPath = absPath.mid(baseDir.length() + 1).replace('\\', '/');

        QFile inFile(filePath);
        if (!inFile.open(QIODevice::ReadOnly))
        {
            LOG_WARNING(QString("BA2 create(DX10): skipping unreadable file: %1").arg(filePath));
            continue;
        }
        QByteArray data = inFile.readAll();
        inFile.close();

        ParsedDdsTexture info;
        if (!parseDdsTexture(data, info))
        {
            LOG_WARNING(QString("BA2 create(DX10): skipping non-DDS file: %1").arg(filePath));
            continue;
        }
        InputTexture input;
        input.relativePath = relPath;
        input.info = info;
        input.data = data;
        inputs.append(input);
    }

    if (inputs.isEmpty())
    {
        LOG_ERROR("BA2 create(DX10): no valid DDS files to archive");
        return false;
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly))
    {
        LOG_ERROR(QString("BA2 create(DX10): cannot open output file: %1").arg(outputPath));
        return false;
    }
    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);

    const quint32 n = static_cast<quint32>(inputs.size());
    constexpr quint32 HDR_SIZE = 32;
    // Each file: one 24-byte DX10 record + one 24-byte chunk (all mips in one chunk).
    const quint32 recordsSize = n * 48;
    const quint32 nameTableOffset = HDR_SIZE + recordsSize;

    QVector<QByteArray> nameBytes;
    nameBytes.reserve(n);
    quint32 nameTableSize = 0;
    for (const auto& input : inputs)
    {
        QByteArray nb = input.relativePath.toLower().toUtf8();
        nb.append('\0');
        nameBytes.append(nb);
        nameTableSize += 2 + static_cast<quint32>(nb.size());
    }

    quint64 dataOffset = static_cast<quint64>(nameTableOffset) + nameTableSize;

    struct ChunkEntry {
        quint64 fileOffset = 0;
        quint32 packedSize = 0;
        quint32 unpackedSize = 0;
        QByteArray fileData;
    };
    QVector<ChunkEntry> chunks;
    chunks.reserve(n);

    for (const auto& input : inputs)
    {
        ChunkEntry entry;
        const QByteArray payload = input.data.mid(input.info.dataOffset);
        entry.unpackedSize = static_cast<quint32>(payload.size());

        if (compress)
        {
            QByteArray compressed;
            compressed.resize(payload.size() + payload.size() / 100 + 600);
            uLongf destLen = compressed.size();
            const int ret = compress2(reinterpret_cast<Bytef*>(compressed.data()), &destLen,
                                      reinterpret_cast<const Bytef*>(payload.constData()),
                                      payload.size(), Z_DEFAULT_COMPRESSION);
            if (ret == Z_OK && destLen < static_cast<uLongf>(payload.size()))
            {
                compressed.resize(destLen);
                entry.fileData = compressed;
                entry.packedSize = static_cast<quint32>(destLen);
            }
            else
            {
                entry.fileData = payload;
                entry.packedSize = 0;
            }
        }
        else
        {
            entry.fileData = payload;
            entry.packedSize = 0;
        }

        entry.fileOffset = dataOffset;
        dataOffset += (entry.packedSize != 0) ? entry.packedSize : entry.unpackedSize;
        chunks.append(entry);
    }

    // BTDX v2 header (32 bytes).
    out.writeRawData("BTDX", 4);
    out << static_cast<quint32>(2);              // version 2 (zlib)
    out.writeRawData("DX10", 4);
    out << n;
    out << static_cast<quint64>(nameTableOffset);
    out << static_cast<quint32>(1);
    out << static_cast<quint32>(0);

    // DX10 records + chunks.
    for (int i = 0; i < inputs.size(); ++i)
    {
        const auto& input = inputs[i];
        // 24-byte record: 12 pad + numChunks(u8=1) + chunkHdrSize(u16=24) +
        // height(u16) + width(u16) + numMips(u8) + format(u8) + flags(u8) + tileMode(u8).
        out << static_cast<quint32>(0);
        out << static_cast<quint32>(0);
        out << static_cast<quint32>(0);
        out << static_cast<quint8>(0);           // pad
        out << static_cast<quint8>(1);           // numChunks
        out << static_cast<quint16>(24);         // chunk header size
        out << input.info.height;
        out << input.info.width;
        out << input.info.numMips;
        out << input.info.format;
        out << input.info.flags;
        out << input.info.tileMode;

        // 24-byte chunk: offset(u64) + packed(u32) + unpacked(u32) +
        // startMip(u16) + endMip(u16) + BAADF00D.
        const ChunkEntry& chunk = chunks[i];
        out << chunk.fileOffset;
        out << chunk.packedSize;
        out << chunk.unpackedSize;
        out << static_cast<quint16>(0);          // startMip
        out << static_cast<quint16>(qMax(input.info.numMips, static_cast<quint8>(1)) - 1); // endMip
        out << static_cast<quint32>(BAADF00D);
    }

    // Name table.
    for (int i = 0; i < inputs.size(); ++i)
    {
        const quint16 len = static_cast<quint16>(nameBytes[i].size());
        out << len;
        out.writeRawData(nameBytes[i].constData(), nameBytes[i].size());
    }

    // Chunk data.
    for (const auto& chunk : chunks)
        outFile.write(chunk.fileData);

    outFile.close();
    LOG_INFO(QString("BA2 create(DX10): wrote %1 textures to %2 (compressed=%3)")
        .arg(n).arg(outputPath).arg(compress));
    return true;
}

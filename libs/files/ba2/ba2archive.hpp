#pragma once

#include <QString>
#include <QVector>
#include <QByteArray>
#include <QMap>

class QFile;

struct Ba2FileEntry {
    QString relativePath;
    quint32 compressedSize = 0;
    quint32 uncompressedSize = 0;
    quint64 fileOffset = 0;
    bool compressed = false;
};

/// One mip-range chunk of a DX10 (texture) archive file record.
struct Ba2Dx10Chunk {
    quint64 fileOffset = 0;
    quint32 packedSize = 0;
    quint32 unpackedSize = 0;
    quint16 startMip = 0;
    quint16 endMip = 0;
};

/// One DX10 (texture) archive file record: a DX10 header plus its mip chunks.
struct Ba2Dx10Entry {
    QString relativePath;
    quint16 height = 0;
    quint16 width = 0;
    quint8 numMips = 0;
    quint8 format = 0;   // DXGI format code
    quint8 flags = 0;    // bit 0 set = cubemap
    quint8 tileMode = 0; // 8 = linear
    QVector<Ba2Dx10Chunk> chunks;
};

/// BA2 (Bethesda Archive 2) reader and writer. Handles both extraction
/// and creation of BA2 archives used by Skyrim SE, Fallout 4, Starfield.
class Ba2Archive {
public:
    Ba2Archive();
    ~Ba2Archive();

    Ba2Archive(const Ba2Archive&) = delete;
    Ba2Archive& operator=(const Ba2Archive&) = delete;
    Ba2Archive(Ba2Archive&&) = delete;
    Ba2Archive& operator=(Ba2Archive&&) = delete;

    // Open a BA2 archive file for reading. Returns true on success.
    bool open(const QString& path);

    // Get all file entries in the archive.
    const QVector<Ba2FileEntry>& entries() const { return mEntries; }

    // True when the archive is a DX10 (texture) archive.
    bool isTexture() const { return mIsDx10; }

    // Get all DX10 texture entries (valid when isTexture()).
    const QVector<Ba2Dx10Entry>& textureEntries() const { return mDx10Entries; }

    // Extract a file by index to the given output path.
    // Returns true on success. For uncompressed files, copies directly.
    bool extract(quint32 index, const QString& outputPath) const;

    // Get the archive name.
    QString name() const { return mName; }

    // Number of files in the archive.
    quint32 fileCount() const { return static_cast<quint32>(mEntries.size()); }

    // Create a new BA2 archive from a list of files. Returns true on success.
    // archiveType: "GNRL" for general files, "DX10" for textures.
    bool create(const QStringList& filePaths, const QString& outputPath,
                bool compress = true, const QString& archiveType = "GNRL");

private:
    bool openBtdx(const QString& path);
    bool parseBtdxGeneral(quint32 hdrSize, quint64 nameOffs);
    bool parseBtdxTextures(quint32 hdrSize, quint64 nameOffs);
    bool openLegacy(const QString& path);
    static quint32 readU32(const uchar* p);
    static quint64 readU64(const uchar* p);
    void failOpen();
    bool extractTexture(quint32 index, const QString& outputPath) const;
    static bool decompressChunk(const uchar* data, quint32 packedSize, quint32 unpackedSize,
                                QByteArray& out, bool lz4);
    bool createDx10(const QStringList& filePaths, const QString& outputPath, bool compress);

    QString mName;
    QVector<Ba2FileEntry> mEntries;
    QVector<Ba2Dx10Entry> mDx10Entries;
    bool mIsDx10 = false;
    bool mUseLz4 = false;
    QFile* mFile = nullptr;
    uchar* mMappedData = nullptr;
    qint64 mFileSize = 0;
    quint32 mFileTableOffset = 0;
    quint32 mFileCount = 0;
};

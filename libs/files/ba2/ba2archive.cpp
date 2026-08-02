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

    // v1 / v7 / v8 use a 24-byte header; v2 / v3 use 32 bytes (the file
    // records start right after the header).
    const quint32 hdrSize = (version == 1 || version == 7 || version == 8) ? 24 : 32;
    LOG_INFO(QString("BA2 archive: %1 v%2 type=%3 files=%4 nameOffs=%5")
        .arg(mName).arg(version).arg(type, 4, 16, QChar('0')).arg(mFileCount).arg(nameOffs));

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
    // DX10 archives store per-chunk texture data; not yet supported.
    Q_UNUSED(hdrSize);
    Q_UNUSED(nameOffs);
    LOG_WARNING("BA2 DX10 (texture) archives not yet supported");
    failOpen();
    return false;
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
        decompressedBuf.resize(entry.uncompressedSize);

        uLongf destLen = entry.uncompressedSize;
        int ret = uncompress(reinterpret_cast<Bytef*>(decompressedBuf.data()), &destLen,
                             reinterpret_cast<const Bytef*>(fileDataPtr), compressedSize);

        if (ret != Z_OK) {
            LOG_ERROR(QString("BA2 decompression failed for %1: %2").arg(entry.relativePath).arg(ret));
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

bool Ba2Archive::create(const QStringList& filePaths, const QString& outputPath,
                         bool compress, const QString& archiveType)
{
    if (filePaths.isEmpty()) {
        LOG_ERROR("BA2 create: no files to archive");
        return false;
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("BA2 create: cannot open output file: %1").arg(outputPath));
        return false;
    }

    QDataStream out(&outFile);
    out.setByteOrder(QDataStream::LittleEndian);

    // Determine a base directory for computing relative paths
    QFileInfo outInfo(outputPath);
    QString baseDir = outInfo.absolutePath();

    // Collect file data and relative paths
    struct InputFile {
        QString relativePath;
        QByteArray data;
    };
    QVector<InputFile> inputs;
    inputs.reserve(filePaths.size());

    for (const QString& filePath : filePaths) {
        QFileInfo fi(filePath);
        QString relPath = fi.fileName();

        // Try to compute a relative path from the base dir
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

    // --- BA2 Header (24 bytes) ---
    // Magic: 'BA2 ' (0x20424132 big-endian, but we write little-endian 0x20584142 is wrong)
    // Actual BA2 magic is 0x42413220 ("BA2 " as bytes, little-endian read gives 0x20324142)
    // The correct magic bytes are: 0x42 0x41 0x32 0x20 ("BA2 ")
    out.writeRawData("BA2 ", 4);

    // Version (4 bytes)
    quint32 version = 1;
    out << version;

    // Archive type (4 bytes): "GNRL" or "DX10"
    QByteArray typeBytes = archiveType.toLatin1().leftJustified(4, ' ', true);
    out.writeRawData(typeBytes.data(), 4);

    // --- File table ---
    // For each file: nameLen(4) + name(nameLen) + compressedSize(4) + uncompressedSize(4) + flags(4) + offset(8)
    // We write the file table first, then data after it.

    quint32 fileCount = static_cast<quint32>(inputs.size());

    // Calculate table size to know where file data starts
    quint64 tableSize = 0;
    for (const auto& input : inputs) {
        tableSize += 4 + input.relativePath.length() + 4 + 4 + 4 + 8;
    }

    quint64 dataOffset = 24 + tableSize; // header (24) + table
    quint64 currentDataOffset = dataOffset;

    // Write file table entries and collect compressed data
    struct TableEntry {
        QString relativePath;
        quint32 compressedSize;
        quint32 uncompressedSize;
        quint32 flags;
        quint64 fileOffset;
        QByteArray fileData;
    };
    QVector<TableEntry> tableEntries;

    for (const auto& input : inputs) {
        TableEntry entry;
        entry.relativePath = input.relativePath;
        entry.uncompressedSize = static_cast<quint32>(input.data.size());
        entry.flags = compress ? 0x01 : 0x00;

        if (compress) {
            // Compress with zlib
            QByteArray compressed;
            compressed.resize(input.data.size() + input.data.size() / 100 + 600);
            uLongf destLen = compressed.size();
            int ret = compress2(reinterpret_cast<Bytef*>(compressed.data()), &destLen,
                               reinterpret_cast<const Bytef*>(input.data.constData()),
                               input.data.size(), Z_DEFAULT_COMPRESSION);
            if (ret == Z_OK && destLen < static_cast<uLongf>(input.data.size())) {
                compressed.resize(destLen);
                entry.fileData = compressed;
                entry.compressedSize = static_cast<quint32>(destLen);
            } else {
                // Compression failed or didn't help — store uncompressed
                entry.fileData = input.data;
                entry.compressedSize = entry.uncompressedSize;
                entry.flags = 0x00;
            }
        } else {
            entry.fileData = input.data;
            entry.compressedSize = entry.uncompressedSize;
        }

        entry.fileOffset = currentDataOffset;
        currentDataOffset += entry.compressedSize;
        tableEntries.append(entry);
    }

    // Write file table
    for (const auto& entry : tableEntries) {
        QByteArray nameBytes = entry.relativePath.toLatin1();
        quint32 nameLen = static_cast<quint32>(nameBytes.size());
        out << nameLen;
        out.writeRawData(nameBytes.data(), nameLen);
        out << entry.compressedSize;
        out << entry.uncompressedSize;
        out << entry.flags;
        out << entry.fileOffset;
    }

    // Write file data
    for (const auto& entry : tableEntries) {
        outFile.write(entry.fileData);
    }

    outFile.close();

    LOG_INFO(QString("BA2 create: wrote %1 files to %2 (type=%3, compressed=%4)")
                .arg(fileCount).arg(outputPath).arg(archiveType).arg(compress));
    return true;
}

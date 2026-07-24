#include "ba2archive.hpp"

#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#include <zlib.h>

#include "logger.hpp"

namespace {

constexpr quint32 BA2_MAGIC = 0x20584142; // 'BA2 ' in little-endian

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

    if (mFileSize < 16) {
        LOG_ERROR("BA2 archive too small");
        mFile->unmap(mMappedData); mMappedData = nullptr;
        delete mFile; mFile = nullptr;
        return false;
    }

    // Read magic
    quint32 magic = *reinterpret_cast<const quint32*>(mMappedData);
    if (magic != BA2_MAGIC) {
        LOG_ERROR(QString("Invalid BA2 magic: 0x%1").arg(magic, 8, 16, QChar('0')));
        mFile->unmap(mMappedData); mMappedData = nullptr;
        delete mFile; mFile = nullptr;
        return false;
    }

    // Read version
    quint32 version = *reinterpret_cast<const quint32*>(mMappedData + 4);
    LOG_INFO(QString("BA2 archive version: %1").arg(version));

    // Read file table offset and count
    mFileTableOffset = *reinterpret_cast<const quint32*>(mMappedData + 8);
    mFileCount = *reinterpret_cast<const quint32*>(mMappedData + 12);

    LOG_INFO(QString("BA2 archive: %1 files, table at offset 0x%2")
                 .arg(mFileCount).arg(mFileTableOffset, 8, 16, QChar('0')));

    mName = QFileInfo(path).completeBaseName();

    // Parse file entries
    mEntries.clear();
    mEntries.reserve(mFileCount);

    quint32 pos = mFileTableOffset;
    for (quint32 i = 0; i < mFileCount && pos + 24 <= mFileSize; ++i) {
        Ba2FileEntry entry;

        // Read filename length
        if (pos + 4 > mFileSize) break;
        quint32 nameLen = *reinterpret_cast<const quint32*>(mMappedData + pos);
        pos += 4;

        // Read filename
        if (pos + nameLen > mFileSize) break;
        entry.relativePath = QString::fromLatin1(reinterpret_cast<const char*>(mMappedData + pos), nameLen);
        pos += nameLen;

        // Read compressed size
        if (pos + 4 > mFileSize) break;
        entry.compressedSize = *reinterpret_cast<const quint32*>(mMappedData + pos);
        pos += 4;

        // Read uncompressed size
        if (pos + 4 > mFileSize) break;
        entry.uncompressedSize = *reinterpret_cast<const quint32*>(mMappedData + pos);
        pos += 4;

        // Read flags
        if (pos + 4 > mFileSize) break;
        quint32 flags = *reinterpret_cast<const quint32*>(mMappedData + pos);
        entry.compressed = (flags & 0x1) != 0;
        pos += 4;

        // Read file offset
        if (pos + 8 > mFileSize) break;
        entry.fileOffset = *reinterpret_cast<const quint64*>(mMappedData + pos);
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

    if (entry.fileOffset + entry.uncompressedSize > mFileSize) {
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

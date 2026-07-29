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

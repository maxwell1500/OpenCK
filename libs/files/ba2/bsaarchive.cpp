#include "bsaarchive.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>

#include "../log/logger.hpp"
#include <zlib.h>

namespace {

constexpr quint32 MAGIC_BSA = 0x00415342;          // 'BSA\0'
constexpr quint32 MAGIC_TES3 = 0x00000100;         // '\0\1\0\0'
constexpr quint32 FLAG_PATHNAMES = 0x0001;
constexpr quint32 FLAG_FILENAMES = 0x0002;
constexpr quint32 FLAG_COMPRESS = 0x0004;
constexpr quint32 FILE_SIZE_COMPRESS = 0x40000000;

// LZ4 block decompression (the format Skyrim SE uses for compressed BSA
// entries). Implements the LZ4 raw block format: a stream of sequences, each
// a token byte (high nibble = literal length, low nibble = match length,
// 15 = extended with additional length bytes), literals, then a 2-byte LE
// match offset and match. Returns the number of bytes written to dst (which
// has dstCapacity bytes available), or -1 on malformed input.
int lz4DecompressBlock(const QByteArray& src, char* dst, int dstCapacity)
{
    const int srcSize = src.size();
    int ip = 0;
    int op = 0;

    while (ip < srcSize)
    {
        const quint8 token = static_cast<quint8>(src.at(ip++));

        // Literal length.
        int literalLength = (token >> 4) & 0x0F;
        if (literalLength == 15)
        {
            quint8 lenByte = 0;
            do {
                if (ip >= srcSize) return -1;
                lenByte = static_cast<quint8>(src.at(ip++));
                literalLength += lenByte;
            } while (lenByte == 255);
        }

        // Copy literals.
        if (op + literalLength > dstCapacity) return -1;
        if (ip + literalLength > srcSize) return -1;
        if (literalLength > 0)
        {
            memcpy(dst + op, src.constData() + ip, literalLength);
            ip += literalLength;
            op += literalLength;
        }

        if (ip >= srcSize) break; // last sequence may have only literals

        // Match offset (2 bytes LE).
        if (ip + 2 > srcSize) return -1;
        const quint16 offset = static_cast<quint8>(src.at(ip))
            | (static_cast<quint8>(src.at(ip + 1)) << 8);
        ip += 2;
        if (offset == 0 || offset > op) return -1;

        // Match length.
        int matchLength = (token & 0x0F) + 4; // minimum match is 4 bytes
        if ((token & 0x0F) == 15)
        {
            quint8 lenByte = 0;
            do {
                if (ip >= srcSize) return -1;
                lenByte = static_cast<quint8>(src.at(ip++));
                matchLength += lenByte;
            } while (lenByte == 255);
        }

        if (op + matchLength > dstCapacity) return -1;
        // Copy match (may overlap, so byte-by-byte).
        const int matchPos = op - offset;
        for (int i = 0; i < matchLength; ++i)
            dst[op + i] = dst[matchPos + i];
        op += matchLength;
    }

    return op;
}

// LZ4 frame decompression (the format Skyrim SE uses for compressed BSA
// entries). Layout per the LZ4 frame specification:
//   magic 0x184D2204 (4), FLG (1), BD (1),
//   [content size (8) if FLG bit3], [dictID (4) if FLG bit1], HC (1),
//   then blocks: [block size u32][data] (size 0 = end; block checksum after
//   each block if FLG bit4), then [content checksum (4) if FLG bit2].
// Block data is the raw LZ4 block format. Returns the number of bytes written
// to dst (dstCapacity available), or -1 on malformed input.
int lz4DecompressFrame(const QByteArray& src, char* dst, int dstCapacity)
{
    const int srcSize = src.size();
    int ip = 0;

    auto u32 = [&src, &ip, srcSize](quint32& out) -> bool {
        if (ip + 4 > srcSize) return false;
        out = static_cast<quint8>(src.at(ip))
            | (static_cast<quint8>(src.at(ip + 1)) << 8)
            | (static_cast<quint8>(src.at(ip + 2)) << 16)
            | (static_cast<quint8>(src.at(ip + 3)) << 24);
        ip += 4;
        return true;
    };

    // Magic.
    quint32 magic = 0;
    if (!u32(magic)) return -1;
    if (magic != 0x184D2204u) return -1;

    // FLG / BD.
    if (ip + 2 > srcSize) return -1;
    const quint8 flg = static_cast<quint8>(src.at(ip++));
    const quint8 bd = static_cast<quint8>(src.at(ip++));
    const bool hasContentSize = (flg & 0x08) != 0;
    const bool hasContentChecksum = (flg & 0x04) != 0;
    const bool hasBlockChecksum = (flg & 0x10) != 0;
    const bool hasDictId = (flg & 0x02) != 0;
    (void)bd;

    if (hasContentSize)
        ip += 8;
    if (hasDictId)
        ip += 4;
    if (ip + 1 > srcSize) return -1;
    ip += 1; // header checksum (skipped; not validated)

    // Blocks.
    int op = 0;
    while (true)
    {
        quint32 blockSize = 0;
        if (!u32(blockSize)) return -1;
        if (blockSize == 0) break; // end mark

        if (static_cast<qint64>(ip) + blockSize > srcSize)
            return -1;
        const QByteArray block = src.mid(ip, static_cast<int>(blockSize));
        ip += static_cast<int>(blockSize);
        if (hasBlockChecksum)
            ip += 4; // block checksum (skipped)

        const int written = lz4DecompressBlock(block, dst + op, dstCapacity - op);
        if (written < 0) return -1;
        op += written;
        if (op > dstCapacity) return -1;
    }

    if (hasContentChecksum)
        ip += 4;

    return op;
}

quint16 readU16(QDataStream& ds)
{
    quint16 v = 0;
    ds >> v;
    return v;
}

quint32 readU32(QDataStream& ds)
{
    quint32 v = 0;
    ds >> v;
    return v;
}

quint64 readU64(QDataStream& ds)
{
    quint64 v = 0;
    ds >> v;
    return v;
}

QString readLenPrefixed(QDataStream& ds, bool& ok)
{
    quint8 len = 0;
    ds.readRawData(reinterpret_cast<char*>(&len), 1);
    QByteArray raw(len, '\0');
    qint64 got = ds.readRawData(raw.data(), len);
    if (got != len) { ok = false; return QString(); }
    // The length prefix counts the null terminator as well.
    if (!raw.isEmpty() && raw.at(raw.size() - 1) == '\0')
        raw.chop(1);
    return QString::fromUtf8(raw);
}

QString readNullTerminated(QDataStream& ds, bool& ok)
{
    QByteArray raw;
    char c = 0;
    while (true) {
        if (ds.readRawData(&c, 1) != 1) { ok = false; return QString(); }
        if (c == '\0') break;
        raw.append(c);
    }
    return QString::fromUtf8(raw);
}

} // namespace

BsaArchive::BsaArchive()
{
}

BsaArchive::~BsaArchive()
{
    if (mFile) {
        mFile->close();
        delete mFile;
        mFile = nullptr;
    }
}

bool BsaArchive::open(const QString& path)
{
    mEntries.clear();
    mName = QFileInfo(path).completeBaseName();

    mFile = new QFile(path);
    if (!mFile->open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("BsaArchive: cannot open %1").arg(path));
        delete mFile;
        mFile = nullptr;
        return false;
    }
    mFileSize = mFile->size();
    QDataStream ds(mFile);
    ds.setByteOrder(QDataStream::LittleEndian);

    quint32 magic = readU32(ds);
    if (magic != MAGIC_BSA && magic != MAGIC_TES3) {
        LOG_ERROR(QString("BsaArchive: invalid magic 0x%1 (expected 'BSA\\0' or TES3)").arg(magic, 8, 16, QChar('0')));
        mFile->close();
        delete mFile;
        mFile = nullptr;
        return false;
    }

    if (magic == MAGIC_TES3)
    {
        // Morrowind BSA: header (HashOffset, FileCount) then per-file records.
        readU32(ds); // hash offset
        const quint32 fileCount = readU32(ds);

        struct Tes3File { quint32 size; quint32 offset; QString name; };
        QVector<Tes3File> files;
        files.reserve(fileCount);
        for (quint32 i = 0; i < fileCount; ++i) {
            Tes3File f;
            f.size = readU32(ds);
            f.offset = readU32(ds);
            files.append(f);
        }
        // Skip name-offset table, then read names.
        ds.device()->seek(12 + 8 * static_cast<qint64>(fileCount) + 4 * static_cast<qint64>(fileCount));
        for (quint32 i = 0; i < fileCount; ++i) {
            bool ok = true;
            files[i].name = readNullTerminated(ds, ok);
            if (!ok) break;
        }
        // Data offsets are relative to the end of the table (magic + header +
        // file records + name offsets + names + hashes).
        qint64 dataOffset = 12 + 8 * static_cast<qint64>(fileCount)
                          + 4 * static_cast<qint64>(fileCount);
        for (int i = 0; i < files.size(); ++i)
            dataOffset += files[i].name.size() + 1;
        dataOffset += 8 * static_cast<qint64>(fileCount);

        for (const Tes3File& f : files) {
            BsaFileEntry entry;
            entry.fileName = f.name;
            entry.fullPath = f.name;
            entry.size = f.size;
            entry.offset = f.offset + static_cast<quint32>(dataOffset);
            entry.compressed = false;
            mEntries.append(entry);
        }
        mVersion = 0;
        mFlags = 0;
        LOG_INFO(QString("BsaArchive: loaded %1 TES3 files from %2").arg(mEntries.size()).arg(path));
        return !mEntries.isEmpty();
    }

    mVersion = readU32(ds);

    // 28-byte header: FoldersOffset, Flags, FolderCount, FileCount,
    // FolderNamesLength, FileNamesLength, FileFlags
    const quint32 foldersOffset = readU32(ds);
    mFlags = readU32(ds);
    const quint32 folderCount = readU32(ds);
    const quint32 fileCount = readU32(ds);
    readU32(ds); // folder names length
    readU32(ds); // file names length
    readU32(ds); // file flags

    LOG_DEBUG(QString("BsaArchive: version=%1 foldersOffset=%2 flags=0x%3 folders=%4 files=%5")
        .arg(mVersion).arg(foldersOffset).arg(mFlags, 8, 16, QChar('0')).arg(folderCount).arg(fileCount));

    const bool isSse = (mVersion == 0x69);

    struct Folder {
        QString name;
        quint32 fileCount = 0;
        QVector<BsaFileEntry> files;
    };
    QVector<Folder> folders;
    folders.reserve(folderCount);

    // All folder records first: Hash u64, FileCount u32, [Unk u32], Offset.
    ds.device()->seek(foldersOffset);
    for (quint32 i = 0; i < folderCount; ++i) {
        Folder folder;
        readU64(ds); // folder name hash
        folder.fileCount = readU32(ds);
        if (isSse) {
            readU32(ds); // unk
            readU64(ds); // offset (i64)
        } else {
            readU32(ds); // offset (u32)
        }
        folders.append(folder);
    }

    // Then per folder: name (u8 len + bytes), then that folder's file records
    // (16 bytes each: Hash u64, Size u32, Offset u32). Matches xEdit.
    for (int i = 0; i < folders.size(); ++i) {
        bool ok = true;
        folders[i].name = readLenPrefixed(ds, ok);
        folders[i].files.reserve(static_cast<int>(folders[i].fileCount));
        for (quint32 j = 0; j < folders[i].fileCount; ++j) {
            BsaFileEntry entry;
            entry.nameHash = readU64(ds);
            entry.size = readU32(ds);
            entry.offset = readU32(ds);
            entry.compressed = false;
            folders[i].files.append(entry);
        }
    }

    // Then all file names (null-terminated).
    for (int i = 0; i < folders.size(); ++i) {
        for (int j = 0; j < folders[i].files.size(); ++j) {
            bool ok = true;
            folders[i].files[j].fileName = readNullTerminated(ds, ok);
            if (!ok) break;
        }
    }

    // Flatten into the entries list, computing full paths and compression.
    const bool archiveCompresses = (mFlags & FLAG_COMPRESS) != 0;
    for (const Folder& folder : folders) {
        for (const BsaFileEntry& file : folder.files) {
            BsaFileEntry entry = file;
            entry.folderName = folder.name;
            const bool fileCompressed = (entry.size & FILE_SIZE_COMPRESS) != 0;
            entry.compressed = archiveCompresses != fileCompressed;
            if (!folder.name.isEmpty())
                entry.fullPath = folder.name + QLatin1Char('\\') + entry.fileName;
            else
                entry.fullPath = entry.fileName;
            mEntries.append(entry);
        }
    }

    LOG_INFO(QString("BsaArchive: loaded %1 files from %2").arg(mEntries.size()).arg(path));
    return !mEntries.isEmpty();
}

bool BsaArchive::readData(quint32 index, QByteArray& out) const
{
    if (index >= static_cast<quint32>(mEntries.size())) {
        LOG_ERROR(QString("BsaArchive: index %1 out of range").arg(index));
        return false;
    }
    const BsaFileEntry& entry = mEntries[index];
    return entry.compressed ? readCompressed(index, out) : readUncompressed(index, out);
}

bool BsaArchive::readUncompressed(quint32 index, QByteArray& out) const
{
    const BsaFileEntry& entry = mEntries[index];
    const quint32 size = entry.rawSize();
    if (static_cast<qint64>(entry.offset) + size > mFileSize) {
        LOG_ERROR(QString("BsaArchive: data out of range for %1").arg(entry.fullPath));
        return false;
    }
    if (!mFile->seek(entry.offset)) {
        LOG_ERROR(QString("BsaArchive: seek failed for %1").arg(entry.fullPath));
        return false;
    }
    out.resize(size);
    const qint64 got = mFile->read(out.data(), size);
    return got == size;
}

bool BsaArchive::readCompressed(quint32 index, QByteArray& out) const
{
    const BsaFileEntry& entry = mEntries[index];
    const quint32 packedSize = entry.rawSize();
    if (packedSize < 4) {
        LOG_ERROR(QString("BsaArchive: compressed entry too small: %1").arg(entry.fullPath));
        return false;
    }
    if (static_cast<qint64>(entry.offset) + packedSize > mFileSize) {
        LOG_ERROR(QString("BsaArchive: compressed data out of range for %1").arg(entry.fullPath));
        return false;
    }
    if (!mFile->seek(entry.offset)) {
        LOG_ERROR(QString("BsaArchive: seek failed for %1").arg(entry.fullPath));
        return false;
    }
    QByteArray packed(packedSize, '\0');
    if (mFile->read(packed.data(), packedSize) != packedSize)
        return false;

    // First 4 bytes are the uncompressed size (LE).
    const quint32 uncompressedSize = static_cast<quint32>(
        static_cast<quint8>(packed[0]) |
        (static_cast<quint8>(packed[1]) << 8) |
        (static_cast<quint8>(packed[2]) << 16) |
        (static_cast<quint8>(packed[3]) << 24));
    if (uncompressedSize > 0x7FFFFFFF) {
        LOG_ERROR(QString("BsaArchive: implausible uncompressed size %1 for %2")
            .arg(uncompressedSize).arg(entry.fullPath));
        return false;
    }

    QByteArray decompressed(static_cast<int>(uncompressedSize), '\0');
    int written = -1;
    if (mVersion == 0x69) // SSE: LZ4 frame compression
    {
        const QByteArray frame = packed.mid(4);
        written = lz4DecompressFrame(frame, decompressed.data(), decompressed.size());
    }
    else
    {
        uLongf destLen = uncompressedSize;
        const int ret = uncompress(
            reinterpret_cast<Bytef*>(decompressed.data()), &destLen,
            reinterpret_cast<const Bytef*>(packed.constData() + 4),
            static_cast<uLong>(packedSize - 4));
        if (ret != Z_OK) {
            LOG_ERROR(QString("BsaArchive: decompression failed for %1: %2")
                .arg(entry.fullPath).arg(ret));
            return false;
        }
        written = static_cast<int>(destLen);
    }
    if (written < 0 || written > decompressed.size())
    {
        LOG_ERROR(QString("BsaArchive: LZ4 decompression failed for %1").arg(entry.fullPath));
        return false;
    }
    decompressed.resize(written);
    out = decompressed;
    return true;
}

bool BsaArchive::extract(quint32 index, const QString& outputPath) const
{
    QByteArray data;
    if (!readData(index, data))
        return false;

    QFileInfo outInfo(outputPath);
    if (!outInfo.dir().mkpath(".")) {
        LOG_ERROR(QString("BsaArchive: cannot create directory for %1").arg(outputPath));
        return false;
    }

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("BsaArchive: cannot write to %1").arg(outputPath));
        return false;
    }
    outFile.write(data);
    outFile.close();
    return true;
}

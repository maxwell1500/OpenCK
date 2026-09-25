#include "bsaarchive.hpp"

#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>

#include <algorithm>
#include <limits>

#include "../log/logger.hpp"
#include <zlib.h>

namespace {

constexpr quint32 MAGIC_BSA = 0x00415342;          // 'BSA\0'
constexpr quint32 MAGIC_BTDX = 0x58445442;         // 'BTDX' (Starfield)
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

quint32 loadLe32(const char* data)
{
    return static_cast<quint8>(data[0])
        | (static_cast<quint32>(static_cast<quint8>(data[1])) << 8)
        | (static_cast<quint32>(static_cast<quint8>(data[2])) << 16)
        | (static_cast<quint32>(static_cast<quint8>(data[3])) << 24);
}

void appendLe16(QByteArray& output, quint16 value)
{
    output.append(static_cast<char>(value & 0xFFu));
    output.append(static_cast<char>((value >> 8) & 0xFFu));
}

void appendLe32(QByteArray& output, quint32 value)
{
    output.append(static_cast<char>(value & 0xFFu));
    output.append(static_cast<char>((value >> 8) & 0xFFu));
    output.append(static_cast<char>((value >> 16) & 0xFFu));
    output.append(static_cast<char>((value >> 24) & 0xFFu));
}

void appendLe64(QByteArray& output, quint64 value)
{
    for (int i = 0; i < 8; ++i)
        output.append(static_cast<char>((value >> (i * 8)) & 0xFFu));
}

quint32 rotl32(quint32 value, int shift)
{
    return (value << shift) | (value >> (32 - shift));
}

quint32 xxhash32(const QByteArray& data)
{
    constexpr quint32 prime1 = 2654435761u;
    constexpr quint32 prime2 = 2246822519u;
    constexpr quint32 prime3 = 3266489917u;
    constexpr quint32 prime4 = 668265263u;
    constexpr quint32 prime5 = 374761393u;
    const auto* bytes = reinterpret_cast<const quint8*>(data.constData());
    const int size = data.size();
    int offset = 0;
    quint32 hash;
    if (size >= 16) {
        quint32 v1 = prime1 + prime2;
        quint32 v2 = prime2;
        quint32 v3 = 0;
        quint32 v4 = static_cast<quint32>(0) - prime1;
        const int limit = size - 16;
        while (offset <= limit) {
            v1 = rotl32(v1 + loadLe32(reinterpret_cast<const char*>(bytes + offset)) * prime2, 13) * prime1;
            v2 = rotl32(v2 + loadLe32(reinterpret_cast<const char*>(bytes + offset + 4)) * prime2, 13) * prime1;
            v3 = rotl32(v3 + loadLe32(reinterpret_cast<const char*>(bytes + offset + 8)) * prime2, 13) * prime1;
            v4 = rotl32(v4 + loadLe32(reinterpret_cast<const char*>(bytes + offset + 12)) * prime2, 13) * prime1;
            offset += 16;
        }
        hash = rotl32(v1, 1) + rotl32(v2, 7) + rotl32(v3, 12) + rotl32(v4, 18);
    } else {
        hash = prime5;
    }
    hash += static_cast<quint32>(size);
    while (offset + 4 <= size) {
        hash = rotl32(hash + loadLe32(reinterpret_cast<const char*>(bytes + offset)) * prime3, 17) * prime4;
        offset += 4;
    }
    while (offset < size) {
        hash = rotl32(hash + static_cast<quint32>(bytes[offset]) * prime5, 11) * prime1;
        ++offset;
    }
    hash ^= hash >> 15;
    hash *= prime2;
    hash ^= hash >> 13;
    hash *= prime3;
    hash ^= hash >> 16;
    return hash;
}

void appendLz4Length(QByteArray& output, quint32 length)
{
    if (length <= 15)
        return;
    quint32 remaining = length - 15;
    while (remaining >= 255) {
        output.append(static_cast<char>(255));
        remaining -= 255;
    }
    output.append(static_cast<char>(remaining));
}

QByteArray lz4CompressBlock(const QByteArray& source)
{
    QByteArray output;
    output.reserve(source.size() + source.size() / 255 + 16);
    QVector<int> hashTable(1 << 16, -1);
    const auto* data = reinterpret_cast<const quint8*>(source.constData());
    const int size = source.size();
    int anchor = 0;
    int position = 0;

    while (position + 4 <= size) {
        const quint32 value = loadLe32(reinterpret_cast<const char*>(data + position));
        const int hash = static_cast<int>((value * 2654435761u) >> 16);
        const int candidate = hashTable.at(hash);
        hashTable[hash] = position;
        if (candidate < 0 || position - candidate > 65535 || position == candidate
            || loadLe32(reinterpret_cast<const char*>(data + candidate)) != value) {
            ++position;
            continue;
        }

        const quint32 literalLength = static_cast<quint32>(position - anchor);
        int matchLength = 4;
        while (position + matchLength < size
            && data[candidate + matchLength] == data[position + matchLength]) {
            ++matchLength;
        }

        const quint32 encodedMatchLength = static_cast<quint32>(matchLength - 4);
        quint8 token = static_cast<quint8>((qMin(literalLength, 15u) << 4)
            | qMin(encodedMatchLength, 15u));
        output.append(static_cast<char>(token));
        appendLz4Length(output, literalLength);
        if (literalLength > 0)
            output.append(reinterpret_cast<const char*>(data + anchor), literalLength);
        appendLe16(output, static_cast<quint16>(position - candidate));
        appendLz4Length(output, encodedMatchLength);
        position += matchLength;
        anchor = position;
    }

    if (anchor < size) {
        const quint32 literalLength = static_cast<quint32>(size - anchor);
        output.append(static_cast<char>(qMin(literalLength, 15u) << 4));
        appendLz4Length(output, literalLength);
        output.append(reinterpret_cast<const char*>(data + anchor), literalLength);
    }
    return output;
}

QByteArray lz4CompressFrame(const QByteArray& source)
{
    QByteArray frame;
    appendLe32(frame, 0x184D2204u);
    const quint8 flags = 0x68;
    const quint8 blockDescriptor = 0x40;
    frame.append(static_cast<char>(flags));
    frame.append(static_cast<char>(blockDescriptor));
    const QByteArray header = frame.mid(4, 2);
    frame.append(static_cast<char>((xxhash32(header) >> 8) & 0xFFu));
    appendLe64(frame, static_cast<quint64>(source.size()));

    constexpr int blockSize = 1 << 16;
    for (int offset = 0; offset < source.size(); offset += blockSize) {
        const QByteArray block = source.mid(offset, blockSize);
        const QByteArray compressed = lz4CompressBlock(block);
        if (compressed.size() < block.size()) {
            appendLe32(frame, static_cast<quint32>(compressed.size()));
            frame.append(compressed);
        } else {
            appendLe32(frame, static_cast<quint32>(block.size()));
            frame.append(block);
        }
    }
    appendLe32(frame, 0);
    return frame;
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

// Starfield name table entry: u16 length + path bytes.
QString readU16LenString(QDataStream& ds, bool& ok)
{
    ok = false;
    const quint16 len = readU16(ds);
    if (ds.status() != QDataStream::Ok || len == 0 || len > 1024)
        return QString();
    QByteArray bytes(len, 0);
    if (ds.readRawData(bytes.data(), len) != len)
        return QString();
    ok = true;
    return QString::fromLatin1(bytes);
}

QString readLenPrefixed(QDataStream& ds, bool& ok, qint64* encodedLength = nullptr)
{
    if (encodedLength)
        *encodedLength = 0;

    quint8 len = 0;
    if (ds.readRawData(reinterpret_cast<char*>(&len), 1) != 1 || len == 0) {
        ok = false;
        return QString();
    }

    QByteArray raw(len, '\0');
    const qint64 got = ds.readRawData(raw.data(), len);
    if (got != len || raw.at(raw.size() - 1) != '\0') {
        ok = false;
        return QString();
    }
    if (encodedLength)
        *encodedLength = len;
    raw.chop(1);
    return QString::fromUtf8(raw);
}

QString readNullTerminated(QDataStream& ds, bool& ok, qint64* encodedLength = nullptr)
{
    QByteArray raw;
    char c = 0;
    while (true) {
        if (ds.readRawData(&c, 1) != 1) { ok = false; return QString(); }
        if (encodedLength)
            ++*encodedLength;
        if (c == '\0') break;
        raw.append(c);
    }
    return QString::fromUtf8(raw);
}

bool writeU8(QFile& file, quint8 value)
{
    return file.write(reinterpret_cast<const char*>(&value), 1) == 1;
}

bool writeU32(QFile& file, quint32 value)
{
    const char bytes[4] = {
        static_cast<char>(value & 0xFFu),
        static_cast<char>((value >> 8) & 0xFFu),
        static_cast<char>((value >> 16) & 0xFFu),
        static_cast<char>((value >> 24) & 0xFFu)
    };
    return file.write(bytes, sizeof(bytes)) == sizeof(bytes);
}

bool writeU64(QFile& file, quint64 value)
{
    const char bytes[8] = {
        static_cast<char>(value & 0xFFu),
        static_cast<char>((value >> 8) & 0xFFu),
        static_cast<char>((value >> 16) & 0xFFu),
        static_cast<char>((value >> 24) & 0xFFu),
        static_cast<char>((value >> 32) & 0xFFu),
        static_cast<char>((value >> 40) & 0xFFu),
        static_cast<char>((value >> 48) & 0xFFu),
        static_cast<char>((value >> 56) & 0xFFu)
    };
    return file.write(bytes, sizeof(bytes));
}

QByteArray zlibCompress(const QByteArray& source)
{
    uLongf destinationSize = compressBound(static_cast<uLong>(source.size()));
    QByteArray output(static_cast<int>(destinationSize), '\0');
    const int result = compress2(reinterpret_cast<Bytef*>(output.data()), &destinationSize,
        reinterpret_cast<const Bytef*>(source.constData()), static_cast<uLong>(source.size()),
        Z_BEST_SPEED);
    if (result != Z_OK) return {};
    output.resize(static_cast<int>(destinationSize));
    return output;
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

// Starfield 'BTDX' general (GNRL) archives. Layout, verified against the
// shipped archives and matching the community format notes:
//
//   header (32 bytes, or 36 for the LZ4 texture variant)
//     magic "BTDX", version u32, type "GNRL",
//     file count u32, name table offset u64, unknown u64 (1; v3 adds a u32
//     compression method)
//   file count declarations of 36 bytes each
//     file hash u32, extension char[4], directory hash u32,
//     0 u8, 1 u8, 0x0010 u16, data offset u64,
//     packed length u32 (0 = stored), unpacked length u32, 0xBAADF00D u32
//   the file data
//   the name table: file count x (u16 length + path bytes)
//
// Version 2 compresses with zlib, version 3 with a raw LZ4 block. Starfield
// writes paths with forward slashes.
bool BsaArchive::readBtdx()
{
    const auto fail = [this]() {
        mEntries.clear();
        return false;
    };

    QDataStream ds(mFile);
    ds.setByteOrder(QDataStream::LittleEndian);
    if (!mFile->seek(4))
        return fail();

    mVersion = readU32(ds);
    QByteArray type(4, 0);
    ds.readRawData(type.data(), 4);
    const quint32 fileCount = readU32(ds);
    const quint64 nameTableOffset = readU64(ds);
    // Starfield appends a trailing u64 to the classic 24-byte header, plus a
    // u32 compression method on the v3 (LZ4) variant. Declarations start after
    // it, so the tail must be consumed even when its value is unused.
    readU64(ds);
    if (mVersion >= 3) readU32(ds);
    if (ds.status() != QDataStream::Ok)
        return fail();
    mBtdx = true;
    mBtdxLz4 = (mVersion >= 3);

    if (type != QByteArrayLiteral("GNRL")) {
        LOG_ERROR(QString("BsaArchive: BTDX type '%1' is not supported (only GNRL)")
                      .arg(QString::fromLatin1(type)));
        return fail();
    }
    if (fileCount == 0 || fileCount > 20'000'000u
        || nameTableOffset > static_cast<quint64>(mFileSize)) {
        LOG_ERROR(QString("BsaArchive: implausible BTDX header: files=%1 namesAt=%2 size=%3")
                      .arg(fileCount).arg(nameTableOffset).arg(mFileSize));
        return fail();
    }

    struct Declaration {
        quint64 offset = 0;
        quint32 packed = 0;
        quint32 unpacked = 0;
        QString extension;
    };
    QVector<Declaration> declarations;
    declarations.reserve(static_cast<int>(fileCount));
    for (quint32 i = 0; i < fileCount; ++i) {
        Declaration decl;
        const quint32 fileHash = readU32(ds);
        QByteArray extension(4, 0);
        ds.readRawData(extension.data(), 4);
        const quint32 dirHash = readU32(ds);
        char flags[2] = {0, 0};
        ds.readRawData(flags, 2);   // 0, 1
        readU16(ds);                // declaration header size
        decl.offset = readU64(ds);
        decl.packed = readU32(ds);
        decl.unpacked = readU32(ds);
        const quint32 sentinel = readU32(ds);
        if (ds.status() != QDataStream::Ok)
            return fail();
        if (sentinel != 0xBAADF00Du) {
            LOG_ERROR(QString("BsaArchive: BTDX declaration %1 missing 0xBAADF00D marker (got 0x%2)")
                          .arg(i).arg(sentinel, 8, 16, QChar('0')));
            return fail();
        }
        if (decl.offset > static_cast<quint64>(mFileSize)
            || (decl.packed ? decl.packed : decl.unpacked) > mFileSize - static_cast<qint64>(decl.offset)) {
            LOG_ERROR(QString("BsaArchive: BTDX declaration %1 points outside the archive")
                          .arg(i));
            return fail();
        }
        // The extension is stored without a NUL; the full path comes from the
        // name table, so this is only a fallback for archives without one.
        decl.extension = QString::fromLatin1(extension).trimmed();
        Q_UNUSED(fileHash);
        Q_UNUSED(dirHash);
        declarations.append(decl);
    }

    if (!mFile->seek(static_cast<qint64>(nameTableOffset)))
        return fail();
    ds.device()->seek(static_cast<qint64>(nameTableOffset));
    for (int i = 0; i < declarations.size(); ++i) {
        bool ok = true;
        const QString path = readU16LenString(ds, ok);
        if (!ok) {
            LOG_ERROR(QString("BsaArchive: BTDX name table entry %1 is unreadable").arg(i));
            return fail();
        }
        Declaration& decl = declarations[i];
        BsaFileEntry entry;
        entry.fileName = path.section(QLatin1Char('/'), -1);
        entry.folderName = path.contains(QLatin1Char('/'))
            ? path.left(path.lastIndexOf(QLatin1Char('/')))
            : QString();
        entry.fullPath = path;
        entry.offset = decl.offset;
        entry.size = decl.unpacked;
        entry.packedSize = decl.packed;
        // A zero packed length means the file is stored uncompressed.
        entry.compressed = decl.packed != 0;
        if (entry.fileName.isEmpty())
            entry.fileName = decl.extension;
        mEntries.append(entry);
    }

    LOG_INFO(QString("BsaArchive: BTDX v%1 %2 files, %3 compression, name table at %4")
                 .arg(mVersion).arg(mEntries.size())
                 .arg(mBtdxLz4 ? QStringLiteral("LZ4") : QStringLiteral("zlib"))
                 .arg(nameTableOffset));
    return !mEntries.isEmpty();
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
    const auto fail = [this]() {
        mFile->close();
        delete mFile;
        mFile = nullptr;
        mFileSize = 0;
        return false;
    };
    QDataStream ds(mFile);
    ds.setByteOrder(QDataStream::LittleEndian);

    quint32 magic = readU32(ds);
    if (magic != MAGIC_BSA && magic != MAGIC_BTDX && magic != MAGIC_TES3) {
        LOG_ERROR(QString("BsaArchive: invalid magic 0x%1 (expected 'BSA\\0', 'BTDX' or TES3)").arg(magic, 8, 16, QChar('0')));
        return fail();
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

    // Starfield archives use the 'BTDX' container: a 32-byte header, a table
    // of fixed 36-byte file declarations, the file data, and a trailing name
    // table. The version is not comparable with the classic family's (Starfield
    // is 2 while Skyrim SE is 0x69), so the magic is the discriminator.
    if (magic == MAGIC_BTDX)
        return readBtdx() ? true : fail();

    mVersion = readU32(ds);
    const quint32 foldersOffset = readU32(ds);
    mFlags = readU32(ds);
    const quint32 folderCount = readU32(ds);
    const quint32 fileCount = readU32(ds);
    const quint32 folderNamesLength = readU32(ds);
    const quint32 fileNamesLength = readU32(ds);
    readU32(ds); // file flags + padding

    LOG_DEBUG(QString("BsaArchive: version=%1 foldersOffset=%2 flags=0x%3 folders=%4 files=%5")
        .arg(mVersion).arg(foldersOffset).arg(mFlags, 8, 16, QChar('0')).arg(folderCount).arg(fileCount));

    const bool isSse = (mVersion == 0x69);
    const quint32 folderRecordSize = isSse ? 24 : 16;
    const quint64 folderBlocksStart = static_cast<quint64>(foldersOffset)
        + static_cast<quint64>(folderCount) * folderRecordSize;
    if (ds.status() != QDataStream::Ok
        || foldersOffset < 36
        || folderCount > static_cast<quint32>(std::numeric_limits<int>::max())
        || fileCount > static_cast<quint32>(std::numeric_limits<int>::max())
        || folderBlocksStart > mFileSize) {
        return fail();
    }

    struct Folder {
        QString name;
        quint32 fileCount = 0;
        quint64 blockOffset = 0;
        QVector<BsaFileEntry> files;
    };
    QVector<Folder> folders;
    folders.reserve(static_cast<int>(folderCount));

    if (!ds.device()->seek(foldersOffset))
        return fail();

    for (quint32 i = 0; i < folderCount; ++i) {
        Folder folder;
        readU64(ds);
        folder.fileCount = readU32(ds);
        quint32 storedOffset = 0;
        if (isSse) {
            readU32(ds);
            storedOffset = readU32(ds);
            readU32(ds);
        } else {
            storedOffset = readU32(ds);
        }
        if (storedOffset < fileNamesLength) {
            return fail();
        }
        folder.blockOffset = static_cast<quint64>(storedOffset) - fileNamesLength;
        if (folder.blockOffset < folderBlocksStart || folder.blockOffset >= mFileSize) {
            return fail();
        }
        folders.append(folder);
    }
    if (ds.status() != QDataStream::Ok)
        return fail();

    quint64 totalFileCount = 0;
    quint64 totalFolderNameLength = 0;
    quint64 folderBlocksEnd = folderBlocksStart;
    for (Folder& folder : folders) {
        if (folder.fileCount > static_cast<quint32>(std::numeric_limits<int>::max()))
            return fail();
        if (!ds.device()->seek(static_cast<qint64>(folder.blockOffset)))
            return fail();

        qint64 encodedNameLength = 0;
        if (mFlags & FLAG_PATHNAMES) {
            bool ok = true;
            folder.name = readLenPrefixed(ds, ok, &encodedNameLength);
            if (!ok)
                return fail();
            totalFolderNameLength += static_cast<quint64>(encodedNameLength);
        }

        const quint64 blockSize = (mFlags & FLAG_PATHNAMES ? 1 : 0)
            + static_cast<quint64>(encodedNameLength)
            + static_cast<quint64>(folder.fileCount) * 16;
        if (folder.blockOffset + blockSize > mFileSize) {
            LOG_DEBUG(QString("BSA folder block exceeds file: block=%1 size=%2 fileSize=%3")
                .arg(folder.blockOffset).arg(blockSize).arg(mFileSize));
            return fail();
        }
        folderBlocksEnd = std::max(folderBlocksEnd, folder.blockOffset + blockSize);

        folder.files.reserve(static_cast<int>(folder.fileCount));
        for (quint32 j = 0; j < folder.fileCount; ++j) {
            BsaFileEntry entry;
            entry.nameHash = readU64(ds);
            entry.size = readU32(ds);
            entry.offset = readU32(ds);
            entry.compressed = false;
            if (static_cast<qint64>(entry.offset) + (entry.size & 0x3FFFFFFFu) > mFileSize) {
                LOG_DEBUG(QString("BSA file payload out of range: offset=%1 size=%2 fileSize=%3")
                    .arg(entry.offset).arg(entry.size & 0x3FFFFFFFu).arg(mFileSize));
                return fail();
            }
            folder.files.append(entry);
        }
        if (ds.status() != QDataStream::Ok)
            return fail();
        totalFileCount += folder.fileCount;
    }
    if (totalFileCount != fileCount || totalFolderNameLength != folderNamesLength) {
        LOG_DEBUG(QString("BSA table counts mismatch: files=%1 headerFiles=%2 folderNames=%3 headerFolderNames=%4")
            .arg(totalFileCount).arg(fileCount).arg(totalFolderNameLength).arg(folderNamesLength));
        return fail();
    }

    const quint64 fileNamesOffset = folderBlocksEnd;
    if (fileNamesOffset + fileNamesLength > mFileSize)
        return fail();
    for (const Folder& folder : folders) {
        for (const BsaFileEntry& file : folder.files) {
            if (file.offset < fileNamesOffset + fileNamesLength)
                return fail();
        }
    }
    if (!ds.device()->seek(static_cast<qint64>(fileNamesOffset)))
        return fail();
    qint64 fileNameBytes = 0;
    if (mFlags & FLAG_FILENAMES) {
        for (Folder& folder : folders) {
            for (BsaFileEntry& file : folder.files) {
                bool ok = true;
                file.fileName = readNullTerminated(ds, ok, &fileNameBytes);
                if (!ok)
                    return fail();
            }
        }
    }
    if (ds.status() != QDataStream::Ok || fileNameBytes != fileNamesLength) {
        LOG_DEBUG(QString("BSA file-name table mismatch: read=%1 header=%2 status=%3")
            .arg(fileNameBytes).arg(fileNamesLength).arg(static_cast<int>(ds.status())));
        return fail();
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

bool BsaArchive::readBtdxCompressed(quint32 index, QByteArray& out) const
{
    const BsaFileEntry& entry = mEntries[index];
    const quint32 packedSize = entry.packedSize;
    const quint32 unpackedSize = entry.size;
    if (packedSize == 0 || static_cast<qint64>(entry.offset) + packedSize > mFileSize) {
        LOG_ERROR(QString("BsaArchive: BTDX payload out of range for %1").arg(entry.fullPath));
        return false;
    }
    if (!mFile->seek(static_cast<qint64>(entry.offset)))
        return false;
    QByteArray packed(packedSize, '\0');
    if (mFile->read(packed.data(), packedSize) != packedSize)
        return false;

    if (unpackedSize == 0) {
        out.clear();
        return false;
    }
    out.resize(unpackedSize);

    if (mBtdxLz4) {
        // Raw LZ4 block, no frame header and no length prefix.
        const int written = lz4DecompressBlock(packed.constData(), out.data(), out.size());
        if (written != static_cast<int>(unpackedSize)) {
            LOG_ERROR(QString("BsaArchive: BTDX LZ4 decode wrote %1 of %2 bytes for %3")
                          .arg(written).arg(unpackedSize).arg(entry.fullPath));
            out.clear();
            return false;
        }
        return true;
    }

    uLongf destLen = unpackedSize;
    const int ret = uncompress(reinterpret_cast<Bytef*>(out.data()), &destLen,
                               reinterpret_cast<const Bytef*>(packed.constData()),
                               static_cast<uLong>(packedSize));
    if (ret != Z_OK || destLen != unpackedSize) {
        LOG_ERROR(QString("BsaArchive: BTDX zlib decode failed for %1 (%2)")
                      .arg(entry.fullPath).arg(ret));
        out.clear();
        return false;
    }
    return true;
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

    // Starfield: the entry carries the unpacked size, the packed size sits
    // alongside it, and the codec depends on the archive version (v2 zlib,
    // v3 a raw LZ4 block with no frame header).
    if (mBtdx)
        return readBtdxCompressed(index, out);

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

    QSaveFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("BsaArchive: cannot write to %1").arg(outputPath));
        return false;
    }
    const qint64 written = outFile.write(data);
    const bool committed = written == data.size() && outFile.commit();
    if (!committed) {
        LOG_ERROR(QString("BsaArchive: cannot commit output %1 (%2)")
                      .arg(outputPath, outFile.errorString()));
        return false;
    }
    return true;
}

namespace {

// Skyrim SE / FO4 BSA name hash, matching the algorithm Bethesda uses for
// the 64-bit folder/file name hashes in TES4-family archives. Implemented
// from the format reverse-engineering used by xEdit and OpenMW
// (components/bsa/compressedbsafile.cpp generateHash).
quint64 bsaGenerateHash(const QString& str, const QString& extension = QString())
{
    if (str.isEmpty())
        return 0;

    auto at = [&str](int i) -> quint32 {
        QChar c = str.at(i);
        return c == QLatin1Char('/') ? static_cast<quint32>('\\') : c.unicode();
    };

    const int len = str.size();
    quint64 result = at(len - 1);
    if (len >= 3)
        result |= static_cast<quint64>(at(len - 2)) << 8;
    result |= static_cast<quint64>(len) << 16;
    result |= static_cast<quint64>(at(0)) << 24;
    if (len >= 4)
    {
        quint32 hash = 0;
        for (int i = 1; i <= len - 3; ++i)
            hash = hash * 0x1003Fu + at(i);
        result += static_cast<quint64>(hash) << 32;
    }

    if (extension.isEmpty())
        return result;

    if (extension == QLatin1String(".kf"))
        result |= 0x80;
    else if (extension == QLatin1String(".nif"))
        result |= 0x8000;
    else if (extension == QLatin1String(".dds"))
        result |= 0x8080;
    else if (extension == QLatin1String(".wav"))
        result |= 0x80000000u;

    quint32 hash = 0;
    for (const QChar& c : extension)
        hash = hash * 0x1003Fu + c.unicode();
    result += static_cast<quint64>(hash) << 32;
    return result;
}

} // namespace

quint64 BsaArchive::hashName(const QString& stem, const QString& extension)
{
    return bsaGenerateHash(stem, extension);
}

bool BsaArchive::create(const QStringList& filePaths, const QString& outputPath, bool compress,
                        const QString& sourceRoot, quint32 version)
{
    if (filePaths.isEmpty()) {
        LOG_ERROR("BSA create: no files to archive");
        return false;
    }
    if (version != 0x67 && version != 0x68 && version != 0x69) {
        LOG_ERROR(QString("BSA create: unsupported target version 0x%1").arg(version, 8, 16, QChar('0')));
        return false;
    }
    const bool isSse = version == 0x69;

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("BSA create: cannot open output file: %1").arg(outputPath));
        return false;
    }

    QFileInfo outInfo(outputPath);
    const QString baseDir = sourceRoot.isEmpty()
        ? outInfo.absolutePath() : QFileInfo(sourceRoot).absoluteFilePath();

    struct InputFile {
        QString fullPath; // lowercased, backslash separators
        QByteArray data;
        QByteArray diskData;
        QByteArray encodedName;
        quint64 nameHash = 0;
        quint32 dataOffset = 0;
        bool compressed = false;
    };
    QVector<InputFile> inputs;

    for (const QString& filePath : filePaths) {
        QFileInfo fi(filePath);
        const QString absPath = fi.absoluteFilePath();
        QString relPath = QDir(baseDir).relativeFilePath(absPath);
        relPath = QDir::cleanPath(relPath).replace('\\', '/');
        if (relPath.isEmpty() || relPath == ".."
            || relPath.startsWith("../") || QDir::isAbsolutePath(relPath))
            relPath = fi.fileName();

        QFile inFile(filePath);
        if (!inFile.open(QIODevice::ReadOnly)) {
            LOG_WARNING(QString("BSA create: skipping unreadable file: %1").arg(filePath));
            continue;
        }
        InputFile input;
        input.fullPath = relPath.toLower().replace('/', '\\');
        input.data = inFile.readAll();
        inFile.close();
        if (compress && !input.data.isEmpty()) {
            QByteArray packed;
            if (isSse) {
                const QByteArray frame = lz4CompressFrame(input.data);
                appendLe32(packed, static_cast<quint32>(input.data.size()));
                packed.append(frame);
            } else {
                const QByteArray compressed = zlibCompress(input.data);
                if (compressed.isEmpty()) continue;
                appendLe32(packed, static_cast<quint32>(input.data.size()));
                packed.append(compressed);
            }
            if (packed.size() < input.data.size()) {
                input.diskData = packed;
                input.compressed = true;
            }
        }
        if (input.diskData.isEmpty())
            input.diskData = input.data;
        if (input.data.isEmpty()) {
            LOG_WARNING(QString("BSA create: skipping empty file: %1").arg(filePath));
            continue;
        }

        // Split into folder + stem + extension for the hash.
        const int slash = input.fullPath.lastIndexOf('\\');
        QString folder = slash >= 0 ? input.fullPath.left(slash) : QString();
        QString fileName = slash >= 0 ? input.fullPath.mid(slash + 1) : input.fullPath;
        const int dot = fileName.lastIndexOf('.');
        const QString stem = dot > 0 ? fileName.left(dot) : fileName;
        const QString ext = dot > 0 ? fileName.mid(dot) : QString();
        input.nameHash = bsaGenerateHash(stem, ext);
        input.encodedName = fileName.toUtf8();

        // Folder hash uses the folder path with a trailing backslash.
        inputs.append(input);
    }

    if (inputs.isEmpty()) {
        LOG_ERROR("BSA create: no valid files to archive");
        outFile.close();
        return false;
    }

    // Group files by folder, preserving order.
    struct FolderGroup {
        QString name;
        QByteArray encodedName;
        quint64 folderHash = 0;
        quint64 blockOffset = 0;
        QVector<InputFile> files;
    };
    QVector<FolderGroup> folders;
    for (const InputFile& input : inputs) {
        const int slash = input.fullPath.lastIndexOf('\\');
        const QString folderName = slash >= 0 ? input.fullPath.left(slash) : QString();
        bool found = false;
        for (FolderGroup& g : folders) {
            if (g.name == folderName) { g.files.append(input); found = true; break; }
        }
        if (!found) {
            FolderGroup g;
            g.name = folderName;
            g.files.append(input);
            folders.append(g);
        }
    }

    for (FolderGroup& g : folders) {
        g.encodedName = g.name.toUtf8();
        g.folderHash = bsaGenerateHash(g.name + QLatin1String("\\"));
    }
    std::sort(folders.begin(), folders.end(), [](const FolderGroup& lhs, const FolderGroup& rhs) {
        return lhs.folderHash < rhs.folderHash;
    });
    for (FolderGroup& g : folders) {
        std::sort(g.files.begin(), g.files.end(), [](const InputFile& lhs, const InputFile& rhs) {
            return lhs.nameHash < rhs.nameHash;
        });
    }

    for (const FolderGroup& g : folders) {
        if (g.encodedName.size() > 254) {
            LOG_ERROR("BSA create: folder name exceeds 255-byte length prefix");
            outFile.close();
            outFile.remove();
            return false;
        }
    }

    quint64 fileCount64 = 0;
    quint64 folderNamesLength64 = 0;
    quint64 fileNamesLength64 = 0;
    for (const FolderGroup& g : folders) {
        fileCount64 += static_cast<quint64>(g.files.size());
        folderNamesLength64 += g.encodedName.size() + 1;
        for (const InputFile& f : g.files)
            fileNamesLength64 += f.encodedName.size() + 1;
    }
    if (fileCount64 > std::numeric_limits<quint32>::max()
        || folderNamesLength64 > std::numeric_limits<quint32>::max()
        || fileNamesLength64 > std::numeric_limits<quint32>::max()) {
        LOG_ERROR("BSA create: archive tables exceed 32-bit limits");
        outFile.close();
        outFile.remove();
        return false;
    }

    const quint32 folderCount = static_cast<quint32>(folders.size());
    const quint32 fileCount = static_cast<quint32>(fileCount64);
    const quint32 folderNamesLength = static_cast<quint32>(folderNamesLength64);
    const quint32 fileNamesLength = static_cast<quint32>(fileNamesLength64);

    constexpr quint32 HEADER_SIZE = 36;
    const quint32 folderRecordSize = isSse ? 24u : 16u;
    constexpr quint32 FILE_RECORD = 16;
    const quint32 BSA_FLAGS = FLAG_PATHNAMES | FLAG_FILENAMES
        | (compress ? FLAG_COMPRESS : 0);

    quint64 blockCursor = HEADER_SIZE
        + static_cast<quint64>(folderCount) * folderRecordSize;
    for (FolderGroup& g : folders) {
        g.blockOffset = blockCursor;
        blockCursor += 2 + static_cast<quint64>(g.encodedName.size())
            + static_cast<quint64>(g.files.size()) * FILE_RECORD;
    }
    const quint64 dataStart = blockCursor + fileNamesLength;
    if (dataStart > std::numeric_limits<quint32>::max()) {
        LOG_ERROR("BSA create: data start exceeds 32-bit file offset range");
        outFile.close();
        outFile.remove();
        return false;
    }
    quint64 payloadCursor = dataStart;
    for (FolderGroup& g : folders) {
        for (InputFile& f : g.files) {
            if (f.diskData.size() > static_cast<int>(0x3FFFFFFFu)
                || payloadCursor > std::numeric_limits<quint32>::max()
                || static_cast<quint64>(f.diskData.size()) > std::numeric_limits<quint32>::max() - payloadCursor) {
                LOG_ERROR("BSA create: payload exceeds 32-bit file offset range");
                outFile.close();
                outFile.remove();
                return false;
            }
            f.dataOffset = static_cast<quint32>(payloadCursor);
            payloadCursor += static_cast<quint64>(f.diskData.size());
        }
    }

    const auto writeFailed = [&outFile]() {
        outFile.close();
        outFile.remove();
        return false;
    };

    if (outFile.write("BSA\x00", 4) != 4
        || !writeU32(outFile, version)
        || !writeU32(outFile, HEADER_SIZE)
        || !writeU32(outFile, BSA_FLAGS)
        || !writeU32(outFile, folderCount)
        || !writeU32(outFile, fileCount)
        || !writeU32(outFile, folderNamesLength)
        || !writeU32(outFile, fileNamesLength)
        || !writeU32(outFile, 0)) {
        return writeFailed();
    }

    for (const FolderGroup& g : folders) {
        const quint64 storedOffset = g.blockOffset + fileNamesLength;
        if (!writeU64(outFile, g.folderHash)
            || !writeU32(outFile, static_cast<quint32>(g.files.size())))
            return writeFailed();
        if (isSse)
        {
            if (!writeU32(outFile, 0)
                || !writeU32(outFile, static_cast<quint32>(storedOffset))
                || !writeU32(outFile, 0))
                return writeFailed();
        }
        else if (!writeU32(outFile, static_cast<quint32>(storedOffset)))
        {
            return writeFailed();
        }
    }

    for (const FolderGroup& g : folders) {
        if (!writeU8(outFile, static_cast<quint8>(g.encodedName.size() + 1))
            || outFile.write(g.encodedName.constData(), g.encodedName.size()) != g.encodedName.size()
            || !writeU8(outFile, 0)) {
            return writeFailed();
        }
        for (const InputFile& f : g.files) {
            const quint32 size = static_cast<quint32>(f.diskData.size())
                | (compress && !f.compressed ? FILE_SIZE_COMPRESS : 0);
            if (!writeU64(outFile, f.nameHash)
                || !writeU32(outFile, size)
                || !writeU32(outFile, f.dataOffset)) {
                return writeFailed();
            }
        }
    }

    for (const FolderGroup& g : folders) {
        for (const InputFile& f : g.files) {
            if (outFile.write(f.encodedName.constData(), f.encodedName.size()) != f.encodedName.size()
                || !writeU8(outFile, 0)) {
                return writeFailed();
            }
        }
    }

    for (const FolderGroup& g : folders) {
        for (const InputFile& f : g.files) {
            if (outFile.write(f.diskData.constData(), f.diskData.size()) != f.diskData.size())
                return writeFailed();
        }
    }

    outFile.close();
    LOG_INFO(QString("BSA create: wrote %1 files in %2 folders to %3")
                .arg(fileCount).arg(folderCount).arg(outputPath));
    return true;
}

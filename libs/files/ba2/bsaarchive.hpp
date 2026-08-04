#pragma once

#include <QString>
#include <QVector>
#include <QByteArray>

class QFile;

struct BsaFileEntry {
    QString fullPath;          // folder\file.fuz
    QString folderName;
    QString fileName;
    quint32 size = 0;          // on-disk size (bit 30 = compressed)
    quint32 offset = 0;        // absolute file offset of data
    quint64 nameHash = 0;
    bool compressed = false;

    quint32 rawSize() const { return size & 0x3FFFFFFFu; }
};

/// Skyrim SE / Fallout 3 / Skyrim LE / Oblivion BSA reader (magic "BSA\0"),
/// plus the older Morrowind (TES3) BSA (magic "\0\1\0\0").
/// TES4-family layout per the xEdit wbBSArchive implementation:
///   magic 'BSA\0' (4) + version (4, 0x69 = SSE, 0x68 = FO3/TES5, 0x67 = Oblivion)
///   28-byte header: FoldersOffset, Flags, FolderCount, FileCount,
///                   FolderNamesLength, FileNamesLength, FileFlags
///   at FoldersOffset: folder records (24 bytes SSE / 16 bytes older):
///     Hash u64, FileCount u32, [Unk u32], Offset (i64 SSE / u32 older)
///   then per folder: name (u8 len + bytes) + FileCount file records
///     (16 bytes each: Hash u64, Size u32, Offset u32)
///   then all file names (null-terminated).
/// TES3 layout: magic "\0\1\0\0" + header (HashOffset, FileCount) + per file
///   Size+Offset, name offsets, null-terminated names, 8-byte hashes; data
///   offsets are relative to the end of the table.
/// Compression: a file is compressed when
///   (archiveFlags & 0x0004) XOR (size & 0x40000000) is set.
///   SSE compressed data is an LZ4 frame with a 4-byte LE uncompressed-size
///   prefix; older games use zlib.
class BsaArchive {
public:
    BsaArchive();
    ~BsaArchive();

    BsaArchive(const BsaArchive&) = delete;
    BsaArchive& operator=(const BsaArchive&) = delete;

    // Open a BSA archive for reading. Returns true on success.
    bool open(const QString& path);

    const QVector<BsaFileEntry>& entries() const { return mEntries; }
    QString name() const { return mName; }
    int fileCount() const { return mEntries.size(); }
    quint32 archiveFlags() const { return mFlags; }
    int version() const { return mVersion; }

    // Extract a file by index to the given output path.
    bool extract(quint32 index, const QString& outputPath) const;

    // Read a file's data by index into a byte array.
    bool readData(quint32 index, QByteArray& out) const;

    // Create a new Skyrim SE (v0x69) BSA archive from a list of files.
    // Files are stored uncompressed with folder/file name tables, matching
    // the format Bethesda's archive tools and the game read. Returns true
    // on success.
    bool create(const QStringList& filePaths, const QString& outputPath);

    // Compute the 64-bit name hash Bethesda stores in TES4-family BSA file
    // records for a file's stem + extension. Exposed for validation.
    static quint64 hashName(const QString& stem, const QString& extension = QString());

private:
    bool readCompressed(quint32 index, QByteArray& out) const;
    bool readUncompressed(quint32 index, QByteArray& out) const;

    QString mName;
    QVector<BsaFileEntry> mEntries;
    QFile* mFile = nullptr;
    qint64 mFileSize = 0;
    quint32 mFlags = 0;
    quint32 mVersion = 0;
};

#pragma once

#include "../../esm/gameformat.hpp"

#include <QString>
#include <QVector>
#include <QByteArray>

class QFile;

/// One archive format a game accepts.
struct BsaArchiveTarget
{
    quint32 version = 0;        // on-disk version, e.g. 0x69
    bool lz4 = false;           // true for 0x69, false for the zlib versions
    QString label;              // human-readable, e.g. "Skyrim SE (0x69, LZ4)"
    bool isDefault = false;     // the one createForGame() uses
};

struct BsaFileEntry {
    QString fullPath;          // folder\file.fuz
    QString folderName;
    QString fileName;
    quint32 size = 0;          // uncompressed size
    quint32 packedSize = 0;    // compressed size (0 = stored); BTDX only
    quint64 offset = 0;        // absolute file offset of data
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
///     Hash u64, FileCount u32, Offset u32, with SSE padding fields around
///     the offset
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

    // Create a new BSA archive from a list of files.
    // Files use the folder/file name tables and can optionally be compressed.
    // `version` selects the on-disk format; prefer defaultVersionForGame() or
    // targetsForGame() over hard-coding one. Returns true on success.
    bool create(const QStringList& filePaths, const QString& outputPath,
                bool compress = false, const QString& sourceRoot = QString(),
                quint32 version = 0x69);

    // Create using the default target for a game, so callers do not have to
    // remember which version each game expects.
    bool createForGame(const QStringList& filePaths, const QString& outputPath,
                       bool compress, const QString& sourceRoot,
                       GameFormat::Game game);

    // The archive formats a game accepts, best first. Morrowind uses MWSA
    // rather than BSA and Starfield uses BA2, so both return an empty list and
    // a version of 0.
    //
    // Skyrim is deliberately ambiguous: the Game enum covers LE, SE and AE, but
    // LE writes 0x68 while SE and AE write 0x69. The default is 0x69 and 0x68 is
    // offered as an alternative, so an LE user's choice stays visible rather
    // than being silently wrong.
    static QVector<BsaArchiveTarget> targetsForGame(GameFormat::Game game);
    static quint32 defaultVersionForGame(GameFormat::Game game);

    // Compute the 64-bit name hash Bethesda stores in TES4-family BSA file
    // records for a file's stem + extension. Exposed for validation.
    static quint64 hashName(const QString& stem, const QString& extension = QString());

private:
    bool readCompressed(quint32 index, QByteArray& out) const;
    bool readUncompressed(quint32 index, QByteArray& out) const;
    bool readBtdxCompressed(quint32 index, QByteArray& out) const;

    QString mName;
    QVector<BsaFileEntry> mEntries;
    QFile* mFile = nullptr;
    qint64 mFileSize = 0;
    quint32 mFlags = 0;
    quint32 mVersion = 0;

    // 0 for the classic 'BSA\0' family, or the Starfield 'BTDX' version (2 for
    // zlib general archives, 3 for LZ4 texture archives). The two families use
    // different codecs, so the version selects the decompressor.
    bool mBtdx = false;
    bool mBtdxLz4 = false;

    bool readBtdx();
};

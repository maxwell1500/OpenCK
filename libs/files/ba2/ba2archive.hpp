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

    QString mName;
    QVector<Ba2FileEntry> mEntries;
    QFile* mFile = nullptr;
    uchar* mMappedData = nullptr;
    qint64 mFileSize = 0;
    quint32 mFileTableOffset = 0;
    quint32 mFileCount = 0;
};

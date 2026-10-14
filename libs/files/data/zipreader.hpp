#ifndef ZIPREADER_HPP
#define ZIPREADER_HPP

#include <QString>
#include <QByteArray>
#include <QHash>

class QFile;

// Reads single members out of an existing ZIP archive. Only what is needed to
// extract one entry by name: the central directory is parsed once on open and
// the entry's bytes are inflated on demand, so large archives cost no memory
// beyond the index. Names match case-insensitively, as asset paths do on
// Windows (a NIF names "paintedmetalpanel01_orange01.mat", the zip stores
// "PaintedMetalPanel01_Orange01.mat").
class ZipReader
{
public:
    // Opens the archive and indexes its central directory. hasEntry() reports
    // whether the archive was usable.
    explicit ZipReader(const QString& filePath);
    ~ZipReader();

    ZipReader(const ZipReader&) = delete;
    ZipReader& operator=(const ZipReader&) = delete;

    bool isValid() const { return m_valid; }

    bool hasEntry(const QString& nameInArchive) const
    { return m_entries.contains(nameInArchive.toLower()); }

    // Returns the decompressed bytes of one entry, or an empty array if the
    // archive has no such entry or decoding fails.
    QByteArray readEntry(const QString& nameInArchive);

private:
    struct Entry
    {
        quint32 method = 0;
        quint32 compSize = 0;
        quint32 uncompSize = 0;
        qint64 localOffset = 0;
    };

    QByteArray inflateMember(const Entry& e) const;

    QFile* m_file = nullptr;
    QHash<QString, Entry> m_entries;
    bool m_valid = false;
};

#endif // ZIPREADER_HPP

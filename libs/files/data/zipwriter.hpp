#ifndef ZIPWRITER_HPP
#define ZIPWRITER_HPP

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QVector>

class QFile;

// Minimal ZIP archive writer using zlib's deflate (raw, method 8). Produces
// archives readable by Windows Explorer and standard unzip tools. Entries use
// forward-slash names and DOS timestamps.
class ZipWriter
{
public:
    // Creates the archive at the given path. Returns false if the file
    // cannot be opened for writing.
    bool open(const QString& filePath);

    // Adds a file entry from memory.
    void addFile(const QString& nameInArchive, const QByteArray& data);

    // Adds a file entry by reading the contents of an existing file.
    // Returns false if the source file cannot be read.
    bool addFileFromDisk(const QString& sourcePath, const QString& nameInArchive);

    // Finalizes the central directory and closes the file.
    // Must be called before the archive is usable.
    void close();

private:
    QByteArray compress(const QByteArray& data);

    QFile* m_file = nullptr;
    QVector<QByteArray> m_centralDirectory;
};

#endif // ZIPWRITER_HPP

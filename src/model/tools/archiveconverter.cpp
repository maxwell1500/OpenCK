#include "archiveconverter.hpp"
#include "logger.hpp"

#include "../../../libs/files/ba2/ba2archive.hpp"
#include "../../../libs/files/ba2/bsaarchive.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryDir>

namespace {

// One payload staged on disk for the writer, keyed the way both archive
// families spell it (folder/file, case-insensitive).
struct ExtractedEntry
{
    QString key;   // "meshes/chair.nif" (lowercase)
    QString path;  // absolute path on disk
    qint64 size = 0;
};

QString keyOf(const QString& folder, const QString& file)
{
    return (QDir::fromNativeSeparators(folder) + "/" + file).toLower();
}

bool stage(const QString& path, const QByteArray& data)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
    {
        LOG_WARNING(QString("ArchiveConverter: mkpath failed for %1")
                        .arg(QFileInfo(path).absolutePath()));
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size())
    {
        LOG_WARNING(QString("ArchiveConverter: write failed for %1").arg(path));
        return false;
    }
    return true;
}

bool extractBsa(const QString& sourcePath, const QString& destDir,
                QVector<ExtractedEntry>& out)
{
    BsaArchive archive;
    if (!archive.open(sourcePath))
    {
        LOG_WARNING(QString("ArchiveConverter: cannot open BSA %1").arg(sourcePath));
        return false;
    }

    const QVector<BsaFileEntry> entries = archive.entries();
    out.reserve(entries.size());
    for (int i = 0; i < entries.size(); ++i)
    {
        const BsaFileEntry entry = entries.at(i);
        QByteArray data;
        if (!archive.readData(static_cast<quint32>(i), data))
        {
            LOG_WARNING(QString("ArchiveConverter: read failed for %1\\%2")
                            .arg(entry.folderName, entry.fileName));
            return false;
        }
        const QString path =
            QDir(destDir).absoluteFilePath(entry.folderName + "/" + entry.fileName);
        if (!stage(path, data))
        {
            return false;
        }
        out.append({ keyOf(entry.folderName, entry.fileName), path, data.size() });
    }
    return true;
}

bool extractBa2(const QString& sourcePath, const QString& destDir,
                QVector<ExtractedEntry>& out)
{
    Ba2Archive archive;
    if (!archive.open(sourcePath))
    {
        LOG_WARNING(QString("ArchiveConverter: cannot open BA2 %1").arg(sourcePath));
        return false;
    }

    const bool texture = archive.isTexture();
    const QVector<Ba2FileEntry> entries = archive.entries();
    out.reserve(entries.size());
    for (int i = 0; i < entries.size(); ++i)
    {
        const Ba2FileEntry entry = entries.at(i);
        QByteArray data;
        const bool ok = texture
            ? archive.extractTextureToBytes(static_cast<quint32>(i), data)
            : archive.extractToBytes(static_cast<quint32>(i), data);
        if (!ok)
        {
            LOG_WARNING(QString("ArchiveConverter: read failed for %1")
                            .arg(entry.relativePath));
            return false;
        }
        const QString path = QDir(destDir).absoluteFilePath(entry.relativePath);
        if (!stage(path, data))
        {
            return false;
        }
        const QString key = QDir::fromNativeSeparators(entry.relativePath).toLower();
        out.append({ key, path, data.size() });
    }
    return true;
}

// Re-reads the written container and compares every payload byte-for-byte
// against the staged source. A conversion nobody can verify is a failure.
ArchiveConversionReport verify(const QString& writtenPath,
                               const QVector<ExtractedEntry>& expected,
                               bool writtenIsBa2)
{
    ArchiveConversionReport report;

    QHash<QString, QByteArray> actual;
    if (writtenIsBa2)
    {
        Ba2Archive archive;
        if (!archive.open(writtenPath))
        {
            report.failures.append(QStringLiteral("written BA2 does not open"));
            return report;
        }
        const bool texture = archive.isTexture();
        const QVector<Ba2FileEntry> entries = archive.entries();
        for (int i = 0; i < entries.size(); ++i)
        {
            QByteArray data;
            const bool ok = texture
                ? archive.extractTextureToBytes(static_cast<quint32>(i), data)
                : archive.extractToBytes(static_cast<quint32>(i), data);
            if (!ok)
            {
                report.failures.append(entries.at(i).relativePath
                                       + QStringLiteral(": unreadable"));
                continue;
            }
            actual.insert(QDir::fromNativeSeparators(entries.at(i).relativePath).toLower(),
                          data);
            report.bytesOut += data.size();
        }
    }
    else
    {
        BsaArchive archive;
        if (!archive.open(writtenPath))
        {
            report.failures.append(QStringLiteral("written BSA does not open"));
            return report;
        }
        const QVector<BsaFileEntry> entries = archive.entries();
        for (int i = 0; i < entries.size(); ++i)
        {
            const BsaFileEntry entry = entries.at(i);
            QByteArray data;
            if (!archive.readData(static_cast<quint32>(i), data))
            {
                report.failures.append(entry.fullPath + QStringLiteral(": unreadable"));
                continue;
            }
            actual.insert(keyOf(entry.folderName, entry.fileName), data);
            report.bytesOut += data.size();
        }
    }

    // The source may hold duplicate-name entries (Oblivion's Misc.bsa does);
    // they stage to one path and the writer stores one record, so the
    // expectation is the unique set of keys, not the raw entry count.
    QSet<QString> wantedKeys;
    for (const ExtractedEntry& entry : expected)
    {
        wantedKeys.insert(entry.key);
    }

    for (const ExtractedEntry& entry : expected)
    {
        if (!wantedKeys.remove(entry.key))
        {
            continue; // already compared
        }
        QFile file(entry.path);
        if (!file.open(QIODevice::ReadOnly))
        {
            report.failures.append(entry.key + QStringLiteral(": staged file unreadable"));
            continue;
        }
        const QByteArray source = file.readAll();
        file.close();
        if (!actual.contains(entry.key))
        {
            report.failures.append(entry.key + QStringLiteral(": missing from output"));
            continue;
        }
        if (actual.value(entry.key) != source)
        {
            report.failures.append(entry.key + QStringLiteral(": payload mismatch"));
            continue;
        }
        ++report.fileCount;
    }
    return report;
}

// Duplicate-name entries in a source archive stage to the same path; handing
// that path to a writer twice makes it store two files under one name, which
// shifts the container's name-table pairing. Keep the first occurrence.
QStringList uniqueStagedFiles(const QVector<ExtractedEntry>& entries,
                              qint64* bytesIn = nullptr)
{
    QStringList files;
    QSet<QString> seen;
    qint64 total = 0;
    for (const ExtractedEntry& entry : entries)
    {
        if (seen.contains(entry.path))
        {
            continue;
        }
        seen.insert(entry.path);
        files.append(entry.path);
        total += entry.size;
    }
    if (bytesIn)
    {
        *bytesIn = total;
    }
    return files;
}

} // namespace

QString ArchiveConversionReport::summary() const
{
    return QStringLiteral("%1 file(s) verified, %2 bytes in, %3 bytes out, %4 failure(s)")
        .arg(fileCount).arg(bytesIn).arg(bytesOut).arg(failures.size());
}

ArchiveConversionReport ArchiveConverter::convertBsa(const QString& sourcePath,
                                                     GameFormat::Game targetGame,
                                                     const QString& outputPath,
                                                     bool compress)
{
    ArchiveConversionReport report;

    // Morrowind's MWSA and Starfield's BA2 are not classic BSA targets;
    // refuse rather than write a container the game will not read.
    if (BsaArchive::targetsForGame(targetGame).isEmpty())
    {
        report.failures.append(
            QStringLiteral("%1 does not use the classic BSA format")
                .arg(GameFormat::gameName(targetGame)));
        return report;
    }

    QTemporaryDir workDir;
    if (!workDir.isValid())
    {
        report.failures.append(QStringLiteral("no temporary workspace"));
        return report;
    }

    QVector<ExtractedEntry> entries;
    if (!extractBsa(sourcePath, workDir.path(), entries))
    {
        report.failures.append(QStringLiteral("extraction failed"));
        return report;
    }

    qint64 bytesIn = 0;
    const QStringList files = uniqueStagedFiles(entries, &bytesIn);

    BsaArchive writer;
    if (!writer.createForGame(files, outputPath, compress, workDir.path(), targetGame))
    {
        report.failures.append(QStringLiteral("write failed"));
        return report;
    }

    report = verify(outputPath, entries, false);
    report.bytesIn = bytesIn;
    return report;
}

ArchiveConversionReport ArchiveConverter::convertBsaToBa2(const QString& sourcePath,
                                                          const QString& outputPath,
                                                          bool compress,
                                                          const QString& archiveType)
{
    ArchiveConversionReport report;

    QTemporaryDir workDir;
    if (!workDir.isValid())
    {
        report.failures.append(QStringLiteral("no temporary workspace"));
        return report;
    }

    QVector<ExtractedEntry> entries;
    if (!extractBsa(sourcePath, workDir.path(), entries))
    {
        report.failures.append(QStringLiteral("extraction failed"));
        return report;
    }

    qint64 bytesIn = 0;
    const QStringList files = uniqueStagedFiles(entries, &bytesIn);

    Ba2Archive writer;
    if (!writer.create(files, outputPath, compress, archiveType, workDir.path()))
    {
        report.failures.append(QStringLiteral("write failed"));
        return report;
    }

    report = verify(outputPath, entries, true);
    report.bytesIn = bytesIn;
    return report;
}

ArchiveConversionReport ArchiveConverter::convertBa2ToBsa(const QString& sourcePath,
                                                          GameFormat::Game targetGame,
                                                          const QString& outputPath,
                                                          bool compress)
{
    ArchiveConversionReport report;

    if (BsaArchive::targetsForGame(targetGame).isEmpty())
    {
        report.failures.append(
            QStringLiteral("%1 does not use the classic BSA format")
                .arg(GameFormat::gameName(targetGame)));
        return report;
    }

    QTemporaryDir workDir;
    if (!workDir.isValid())
    {
        report.failures.append(QStringLiteral("no temporary workspace"));
        return report;
    }

    QVector<ExtractedEntry> entries;
    if (!extractBa2(sourcePath, workDir.path(), entries))
    {
        report.failures.append(QStringLiteral("extraction failed"));
        return report;
    }

    qint64 bytesIn = 0;
    const QStringList files = uniqueStagedFiles(entries, &bytesIn);

    BsaArchive writer;
    if (!writer.createForGame(files, outputPath, compress, workDir.path(), targetGame))
    {
        report.failures.append(QStringLiteral("write failed"));
        return report;
    }

    report = verify(outputPath, entries, false);
    report.bytesIn = bytesIn;
    return report;
}

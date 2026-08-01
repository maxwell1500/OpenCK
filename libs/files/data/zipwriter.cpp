#include "zipwriter.hpp"

#include <QFile>
#include <QDateTime>
#include <QDataStream>

#include "../log/logger.hpp"

#include <zlib.h>

namespace {

// zlib's compress2() gives us a raw DEFLATE stream; the ZIP format wraps it
// with a two-byte header and a four-byte adler32 trailer that the caller is
// responsible for supplying (the header type byte is left 0).
QByteArray rawDeflate(const QByteArray& input)
{
    uLongf destLen = compressBound(static_cast<uLong>(input.size()));
    QByteArray out(destLen, Qt::Uninitialized);
    const int res = compress2(
        reinterpret_cast<Bytef*>(out.data()), &destLen,
        reinterpret_cast<const Bytef*>(input.constData()),
        static_cast<uLong>(input.size()),
        Z_BEST_COMPRESSION);
    if (res != Z_OK) {
        return QByteArray();
    }
    out.truncate(static_cast<int>(destLen));
    return out;
}

// DOS date/time packing used by ZIP headers.
quint16 packDosTime(const QDateTime& dt)
{
    return static_cast<quint16>((dt.time().hour() << 11)
        | (dt.time().minute() << 5)
        | (dt.time().second() / 2));
}

quint16 packDosDate(const QDateTime& dt)
{
    return static_cast<quint16>(((dt.date().year() - 1980) << 9)
        | (dt.date().month() << 5)
        | dt.date().day());
}

} // namespace

bool ZipWriter::open(const QString& filePath)
{
    auto* file = new QFile(filePath);
    if (!file->open(QIODevice::WriteOnly)) {
        LOG_WARNING(QString("ZipWriter: cannot open %1").arg(filePath));
        delete file;
        return false;
    }
    m_file = file;
    m_centralDirectory.clear();
    return true;
}

void ZipWriter::addFile(const QString& nameInArchive, const QByteArray& data)
{
    if (!m_file) {
        return;
    }

    const QByteArray name = nameInArchive.toUtf8();
    const QByteArray compressed = rawDeflate(data);
    const bool store = compressed.isEmpty();

    const QDateTime now = QDateTime::currentDateTime();
    const quint16 dosTime = packDosTime(now);
    const quint16 dosDate = packDosDate(now);

    QDataStream out(m_file);
    out.setByteOrder(QDataStream::LittleEndian);

    // Local file header
    out << quint32(0x04034b50);
    out << quint16(20);            // version needed to extract
    out << quint16(0x0000);        // general purpose bit flag
    out << quint16(store ? 0 : 8); // compression method
    out << dosTime << dosDate;
    out << quint32(0);             // crc-32 (patched below)
    out << quint32(store ? static_cast<quint32>(data.size()) : static_cast<quint32>(compressed.size()));
    out << quint32(static_cast<quint32>(data.size()));
    out << quint16(static_cast<quint16>(name.size()));
    out << quint16(0);             // extra field length
    out.writeRawData(name.constData(), name.size());
    out.writeRawData(store ? data.constData() : compressed.constData(),
                     store ? data.size() : compressed.size());

    // The CRC is written after we know the full payload; rewind and patch.
    const quint32 crc = static_cast<quint32>(crc32(0, reinterpret_cast<const Bytef*>(data.constData()),
        static_cast<uInt>(data.size())));
    const qint64 headerSize = static_cast<qint64>(30 + name.size());
    const qint64 dataStart = m_file->pos() - static_cast<qint64>(store ? data.size() : compressed.size());
    m_file->seek(dataStart - headerSize + 14);
    QDataStream crcOut(m_file);
    crcOut.setByteOrder(QDataStream::LittleEndian);
    crcOut << crc;
    m_file->seek(m_file->size());

    // Central directory entry (recorded for close())
    QByteArray central;
    QDataStream cent(&central, QIODevice::WriteOnly);
    cent.setByteOrder(QDataStream::LittleEndian);
    cent << quint32(0x02014b50);
    cent << quint16(20);            // version made by
    cent << quint16(20);            // version needed to extract
    cent << quint16(0x0000);        // flags
    cent << quint16(store ? 0 : 8); // method
    cent << dosTime << dosDate;
    cent << crc;
    cent << quint32(store ? static_cast<quint32>(data.size()) : static_cast<quint32>(compressed.size()));
    cent << quint32(static_cast<quint32>(data.size()));
    cent << quint16(static_cast<quint16>(name.size()));
    cent << quint16(0);             // extra field length
    cent << quint16(0);             // comment length
    cent << quint16(0);             // disk number start
    cent << quint16(0);             // internal attributes
    cent << quint32(0);             // external attributes
    cent << quint32(dataStart);     // local header offset
    cent.writeRawData(name.constData(), name.size());
    m_centralDirectory.append(central);
}

bool ZipWriter::addFileFromDisk(const QString& sourcePath, const QString& nameInArchive)
{
    QFile f(sourcePath);
    if (!f.open(QIODevice::ReadOnly)) {
        LOG_WARNING(QString("ZipWriter: cannot read %1").arg(sourcePath));
        return false;
    }
    const QByteArray data = f.readAll();
    f.close();
    addFile(nameInArchive, data);
    return true;
}

void ZipWriter::close()
{
    if (!m_file) {
        return;
    }

    const quint32 centralStart = static_cast<quint32>(m_file->size());
    for (const QByteArray& entry : m_centralDirectory) {
        m_file->write(entry);
    }
    const quint32 centralSize = static_cast<quint32>(m_file->size()) - centralStart;

    QDataStream out(m_file);
    out.setByteOrder(QDataStream::LittleEndian);
    out << quint32(0x06054b50);   // end of central directory
    out << quint16(0);            // disk number
    out << quint16(0);            // disk with central directory
    out << quint16(static_cast<quint16>(m_centralDirectory.size()));
    out << quint16(static_cast<quint16>(m_centralDirectory.size()));
    out << centralSize;
    out << centralStart;
    out << quint16(0);            // comment length

    m_file->flush();
    m_file->close();
    delete m_file;
    m_file = nullptr;
}

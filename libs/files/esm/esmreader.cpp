#include "esmreader.hpp"
#include "tes4codes.hpp"
#include "../log/logger.hpp"

#include <QBuffer>
#include <zlib.h>
#include <sstream>

ESMReader::ESMReader(const QString& path)
    : esm(path)
{
}

ESMReader::ESMReader(const QString& path, const FilePaths& filePaths)
    : esm(path, filePaths)
{
}

ESMReader::~ESMReader()
{
}

void ESMReader::startStream()
{
}

void ESMReader::open()
{
    esm.file.close();
    stream.setDevice(&esm.file);
    stream.setByteOrder(QDataStream::LittleEndian);

    if (!esm.file.open(QIODevice::ReadOnly))
    {
        std::ostringstream oss;
        oss << "Error: cannot open data file \""
            << esm.file.fileName().toStdString()
            << "\".";
        throw std::runtime_error(oss.str());
    }

    if (readName() != 'TES4')
    {
        notifyFailure("not a valid Skyrim file!");
    }

    header.load(*this);
    esm.recCount = header.numRecords;

    if (header.flags.test(FileFlag::Localized))
    {
        esm.localised = true;
    }

    qint64 posAfterHeader = esm.file.pos();
    qint64 recLeft = esm.recLeft;
    qint64 subLeft = esm.subLeft;
    qDebug() << "ESMReader::open: posAfterHeader=" << posAfterHeader
             << "recLeft=" << recLeft
             << "subLeft=" << subLeft
             << "fileSize=" << esm.size;
}

NAME ESMReader::readName()
{
    NAME name = 0;
    buf.resize(sizeof(NAME));
    qint64 bytesRead = stream.readRawData(buf.data(), sizeof(NAME));
    esm.forward(sizeof(NAME));
    if (bytesRead != sizeof(NAME))
    {
        return 0;
    }
    memcpy(&name, buf.data(), sizeof(NAME));
    return swapName(name);
}

bool ESMReader::isNextName(NAME name)
{
    NAME cmp;
    buf.resize(sizeof(NAME));
    esm.file.peek(buf.data(), sizeof(NAME));
    memcpy(&cmp, buf.data(), sizeof(NAME));
    return swapName(cmp) == name;
}

void ESMReader::skipGrupHeader()
{
    quint32 grupSize = readType<quint32>(true);    // size
    mGrupEnd = esm.file.pos() + grupSize;
    readType<quint32>(true);    // label
    readType<quint32>(true);    // group type
    readType<quint8>(true);        // vc day
    readType<quint8>(true);        // vc month
    readType<quint8>(true);        // recent user
    readType<quint8>(true);        // current user
    readType<quint32>(true);    // unknown
}

RecHeader ESMReader::readHeader()
{
    // If we're inside a decompressed record buffer, restore the file stream
    // so the next record is read from the file. Any leftover recLeft in the
    // compressed buffer is silently dropped — the caller's readNSubHeader
    // should have drained it.
    if (compressedBuffer && compressedBuffer->isOpen())
    {
        restoreStreamFromCompression();
    }

    RecHeader header;
    header.size = readType<quint32>(true);
    esm.recLeft = header.size;
    esm.subLeft = 0;

    header.flags.val = readType<quint32>(true);
    header.id = readType<quint32>(true);
    mCurrentFormId = header.id;
    header.vcDay = readType<quint8>(true);
    header.vcMonth = readType<quint8>(true);
    header.vcLastUser = readType<quint8>(true);
    header.vcCurrUser = readType<quint8>(true);
    header.version = readType<quint16>(true);
    header.unknown = readType<quint16>(true);

    // Detect compressed records (Starfield/Skyrim/FO4 use zlib).
    // Flag 0x00040000 = record data is zlib-compressed.
    constexpr quint32 FLAG_COMPRESSED = 0x00040000;
    if (header.flags.val & FLAG_COMPRESSED)
    {
        decompressCurrentRecord(static_cast<int>(header.size));
    }

    return header;
}

void ESMReader::decompressCurrentRecord(int compressedSize)
{
    // Compressed record layout (Bethesda):
    //   4 bytes: uncompressed size (LE uint32)
    //   N bytes: zlib-compressed data (0x78 0x9C + deflate + 4-byte adler32)
    // The `compressedSize` parameter is the size from the record header,
    // which equals 4 + N.

    if (compressedSize <= 4)
    {
        throw std::runtime_error("Compressed record too small");
    }

    // Read the entire compressed payload (4-byte size header + zlib stream).
    int payloadSize = compressedSize;
    QByteArray zlibData(payloadSize, '\0');
    qint64 bytesRead = esm.file.read(zlibData.data(), payloadSize);
    if (bytesRead != payloadSize)
    {
        throw std::runtime_error("Failed to read compressed record data");
    }

    // Update left/recLeft/subLeft counters to reflect the file bytes consumed.
    esm.left -= payloadSize;
    esm.recLeft -= payloadSize;

    // Read the 4-byte uncompressed size from the start of the zlib data.
    quint32 uncompressedSize = static_cast<quint8>(zlibData[0])
        | (static_cast<quint8>(zlibData[1]) << 8)
        | (static_cast<quint8>(zlibData[2]) << 16)
        | (static_cast<quint8>(zlibData[3]) << 24);

    // Decompress using zlib (skipping the 4-byte LE size header).
    QByteArray zlibStream = zlibData.mid(4);
    QByteArray decompressed(static_cast<int>(uncompressedSize), '\0');

    z_stream zs = {};
    zs.next_in = reinterpret_cast<Bytef*>(zlibStream.data());
    zs.avail_in = static_cast<uInt>(zlibStream.size());
    zs.next_out = reinterpret_cast<Bytef*>(decompressed.data());
    zs.avail_out = static_cast<uInt>(decompressed.size());

    if (inflateInit(&zs) != Z_OK)
    {
        throw std::runtime_error("Failed to initialize zlib decompression");
    }

    // Use Z_NO_FLUSH loop until all input is consumed.
    int res = Z_OK;
    while (res == Z_OK && zs.avail_in > 0)
    {
        res = inflate(&zs, Z_NO_FLUSH);
    }
    inflateEnd(&zs);

    if (res != Z_STREAM_END && zs.total_out != uncompressedSize)
    {
        throw std::runtime_error(
            QString("Failed to decompress record (compressed=%1 bytes, expected uncompressed=%2, zlib res=%3, got %4)")
                .arg(zlibStream.size())
                .arg(uncompressedSize)
                .arg(res)
                .arg(zs.total_out)
                .toStdString());
    }

    QByteArray actual = decompressed.left(static_cast<int>(zs.total_out));

    // Set recLeft to the actual decompressed size so the load loop drains correctly.
    esm.recLeft = actual.size();
    esm.subLeft = 0;

    // Switch the stream to read from the decompressed buffer.
    compressedData = actual;
    compressedBuffer.reset(new QBuffer(&compressedData));
    compressedBuffer->open(QIODevice::ReadOnly);
    stream.setDevice(compressedBuffer.get());

    LOG_DEBUG(QString("ESMReader: decompressed record: %1 bytes -> %2 bytes")
        .arg(compressedSize)
        .arg(actual.size()));
}

void ESMReader::restoreStreamFromCompression()
{
    if (!compressedBuffer)
    {
        return;
    }
    compressedBuffer->close();
    compressedBuffer.reset();
    compressedData.clear();
    stream.setDevice(&esm.file);
    // recLeft should be 0 (the entire decompressed buffer was consumed) at
    // this point. If it isn't, the caller didn't fully drain the record,
    // and we've already lost those bytes from the file cursor — the next
    // read will be misaligned. This is logged as a warning.
    if (esm.recLeft != 0)
    {
        LOG_WARNING(QString("ESMReader: restoreStreamFromCompression with recLeft=%1, possible misalignment")
            .arg(esm.recLeft));
    }
}

NAME ESMReader::readNSubHeader()
{
    if (esm.subLeft > 0)
    {
        skip(static_cast<int>(esm.subLeft));
    }

    if (esm.recLeft < 6)
    {
        if (esm.recLeft > 0)
        {
            skip(static_cast<int>(esm.recLeft));
        }
        return 0;
    }

    NAME name{ readName() };
    name = Tes4Codes::fromTes4(name);
    quint16 sz = readType<quint16>();
    esm.subLeft = sz;

    return name;
}

quint16 ESMReader::readSubHeader()
{
    quint16 sz = readType<quint16>();
    esm.subLeft = sz;

    return sz;
}

QString ESMReader::readZString()
{
    const quint16 sz = static_cast<quint16>(esm.subLeft);
    buf.resize(sz);
    stream.readRawData(buf.data(), sz);
    esm.forward(sz);
    return QString(QByteArray(buf));
}

QString ESMReader::readSubZString(NAME expectedName)
{
    NAME actualName = readNSubHeader();

    if (actualName != expectedName)
    {
        throw std::runtime_error("Error process subrecord - unexpected name.");
    }

    return readZString();
}

bool ESMReader::isLeft()
{
    return esm.left > 0;
}

bool ESMReader::isRecLeft()
{
    return esm.recLeft > 0;
}

bool ESMReader::isSubLeft()
{
    return esm.subLeft > 0;
}

int ESMReader::recordCount()
{
    return esm.recCount;
}

void ESMReader::skipRecord()
{
    readHeader();
    skip(esm.recLeft);
}

void ESMReader::skipRemainingRecord()
{
    if (esm.recLeft > 0)
    {
        skip(static_cast<int>(esm.recLeft));
    }
}

void ESMReader::skipSub()
{
    readSubHeader();
    skip(esm.subLeft);
}

void ESMReader::skip(int bytes)
{
    // When reading from a decompressed buffer, use the buffer's position.
    // Otherwise use the file's position.
    if (compressedBuffer && compressedBuffer->isOpen())
    {
        qint64 currentPos = compressedBuffer->pos();
        compressedBuffer->seek(currentPos + bytes);
    }
    else
    {
        esm.file.seek(esm.file.pos() + bytes);
    }
    esm.forward(bytes);
}

void ESMReader::skipToGrupEnd()
{
    if (mGrupEnd > 0)
    {
        qint64 remaining = mGrupEnd - esm.file.pos();
        if (remaining > 0)
        {
            skip(static_cast<int>(remaining));
        }
        mGrupEnd = 0;
    }
}

void ESMReader::notifyFailure(const QString& msg)
{
    std::ostringstream oss;
    oss << "Error: " << msg.toStdString()
        << "\nFile: " << esm.file.fileName().toStdString()
        << "\nOffset: "
        << QString("0x%1").arg(esm.size - esm.left, 0, 16).toUpper().toStdString();
    throw std::runtime_error(oss.str());
}

const Header& ESMReader::getHeader() const
{
    return header;
}

Header ESMReader::getHeader()
{
    return header;
}

const Strings& ESMReader::getStrings() const
{
    return esm.strings;
}

bool ESMReader::localised() const
{
    return esm.localised;
}

qint64 ESMReader::filePos() const
{
    return esm.file.pos();
}

void ESMReader::seekTo(qint64 pos)
{
    esm.file.seek(pos);
}

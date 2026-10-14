#include "zipreader.hpp"

#include <QFile>
#include <QVector>

#include <zlib.h>
#include <cstring>

namespace {

quint16 le16(const char* p)
{
    return quint16(quint8(p[0])) | (quint16(quint8(p[1])) << 8);
}

quint32 le32(const char* p)
{
    return quint32(le16(p)) | (quint32(le16(p + 2)) << 16);
}

// Walks back from EOF to the end-of-central-directory record. Its comment
// length is the only clue to where it starts, so scan the tail.
bool findEocd(QFile& f, qint64& outOffset, quint32& outCount)
{
    const qint64 fileSize = f.size();
    if (fileSize < 22)
        return false;

    const qint64 scan = qMin(fileSize, static_cast<qint64>(66000));
    if (!f.seek(fileSize - scan))
        return false;

    const QByteArray buf = f.read(scan);
    if (buf.size() < 22)
        return false;

    for (int pos = buf.size() - 22; pos >= 0; --pos) {
        if (std::memcmp(buf.constData() + pos, "PK\x05\x06", 4) != 0)
            continue;

        const quint32 n = le16(buf.constData() + pos + 10);
        const quint32 cdSize = le32(buf.constData() + pos + 12);
        // The record's own offset field; prefer it, but a streamed writer can
        // report a bogus one, in which case the archive ends just before EOCD.
        const qint64 eocd = fileSize - scan + pos;
        qint64 off = static_cast<qint64>(le32(buf.constData() + pos + 16));
        if (off < 0 || off + static_cast<qint64>(cdSize) > eocd)
            off = eocd - static_cast<qint64>(cdSize);
        if (off < 0)
            continue;

        outOffset = off;
        outCount = n;
        return true;
    }
    return false;
}

} // namespace

ZipReader::ZipReader(const QString& filePath)
    : m_file(new QFile(filePath))
{
    if (!m_file->open(QIODevice::ReadOnly)) {
        delete m_file;
        m_file = nullptr;
        return;
    }

    qint64 cdOffset = 0;
    quint32 count = 0;
    if (!findEocd(*m_file, cdOffset, count) || cdOffset <= 0 || !m_file->seek(cdOffset)) {
        delete m_file;
        m_file = nullptr;
        return;
    }

    const QByteArray cd = m_file->read(static_cast<qint64>(1048576) * 300);
    if (cd.isEmpty()) {
        delete m_file;
        m_file = nullptr;
        return;
    }

    // Central directory entries: 46-byte fixed record, then name, extra and
    // comment. The member body is found later at the local header.
    int pos = 0;
    for (quint32 i = 0; i < count; ++i) {
        if (pos + 46 > cd.size() || std::memcmp(cd.constData() + pos, "PK\x01\x02", 4) != 0)
            break;

        Entry e;
        e.method = le16(cd.constData() + pos + 10);
        e.compSize = le32(cd.constData() + pos + 20);
        e.uncompSize = le32(cd.constData() + pos + 24);
        const quint32 nlen = le16(cd.constData() + pos + 28);
        const quint32 elen = le16(cd.constData() + pos + 30);
        const quint32 clen = le16(cd.constData() + pos + 32);

        if (pos + 46 + nlen > cd.size())
            break;

        e.localOffset = static_cast<qint64>(le32(cd.constData() + pos + 42));
        m_entries.insert(QString::fromUtf8(cd.constData() + pos + 46, static_cast<int>(nlen)).toLower(), e);
        pos += 46 + nlen + elen + clen;
    }

    m_valid = !m_entries.isEmpty();
}

ZipReader::~ZipReader()
{
    delete m_file;
}

QByteArray ZipReader::inflateMember(const Entry& e) const
{
    const QByteArray comp = m_file->read(static_cast<qint64>(e.compSize));
    if (comp.size() != static_cast<qint64>(e.compSize))
        return QByteArray();

    if (e.method == 0) // stored
        return comp;

    // One spare byte lets zlib consume the stream terminator; with the output
    // buffer sized exactly it stops at Z_OK even though every byte is out.
    QByteArray out(static_cast<int>(e.uncompSize) + 1, Qt::Uninitialized);

    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    // A negative window size asks for a raw DEFLATE stream, which is what a
    // ZIP member body is: no zlib or gzip header.
    if (inflateInit2(&strm, -15) != Z_OK)
        return QByteArray();

    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(comp.constData()));
    strm.avail_in = static_cast<uInt>(comp.size());
    strm.next_out = reinterpret_cast<Bytef*>(out.data());
    strm.avail_out = static_cast<uInt>(out.size());

    const int res = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);
    out.resize(static_cast<int>(strm.total_out));
    return (res == Z_STREAM_END) ? out : QByteArray();
}

QByteArray ZipReader::readEntry(const QString& nameInArchive)
{
    if (!m_file || !m_valid)
        return QByteArray();

    auto it = m_entries.constFind(nameInArchive.toLower());
    if (it == m_entries.constEnd())
        return QByteArray();

    if (!m_file->seek(it->localOffset))
        return QByteArray();

    // The local header's own name/extra lengths can differ from the central
    // directory's, so re-read them to find where the member body starts.
    const QByteArray hdr = m_file->read(30);
    if (hdr.size() < 30 || std::memcmp(hdr.constData(), "PK\x03\x04", 4) != 0)
        return QByteArray();

    const qint64 body = it->localOffset + 30 + le16(hdr.constData() + 26) + le16(hdr.constData() + 28);
    if (!m_file->seek(body))
        return QByteArray();

    return inflateMember(it.value());
}

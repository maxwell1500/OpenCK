#include "opallist.hpp"

#include <QDataStream>
#include <QFile>

#include <cstring>

#include "../../files/log/logger.hpp"

namespace {

// The shipped format is little-endian; read explicit widths rather than
// relying on QDataStream's float/version defaults.
quint32 readU32(const QByteArray& data, int& offset)
{
    if (offset + 4 > data.size())
        return 0;
    const uchar* p = reinterpret_cast<const uchar*>(data.constData()) + offset;
    offset += 4;
    return static_cast<quint32>(p[0])
        | (static_cast<quint32>(p[1]) << 8)
        | (static_cast<quint32>(p[2]) << 16)
        | (static_cast<quint32>(p[3]) << 24);
}

quint64 readU64(const QByteArray& data, int& offset)
{
    const quint32 lo = readU32(data, offset);
    const quint32 hi = readU32(data, offset);
    return static_cast<quint64>(lo) | (static_cast<quint64>(hi) << 32);
}

void writeU32(QByteArray& out, quint32 value)
{
    out.append(static_cast<char>(value & 0xFF));
    out.append(static_cast<char>((value >> 8) & 0xFF));
    out.append(static_cast<char>((value >> 16) & 0xFF));
    out.append(static_cast<char>((value >> 24) & 0xFF));
}

void writeU64(QByteArray& out, quint64 value)
{
    writeU32(out, static_cast<quint32>(value & 0xFFFFFFFFu));
    writeU32(out, static_cast<quint32>(value >> 32));
}

} // namespace

QVector<float> OpalPlacement::transform() const
{
    QVector<float> values;
    if (!hasTransform())
        return values;
    values.resize(6);
    for (int i = 0; i < 6; ++i)
    {
        const uchar* p = reinterpret_cast<const uchar*>(payload.constData()) + i * 4;
        const quint32 bits = static_cast<quint32>(p[0])
            | (static_cast<quint32>(p[1]) << 8)
            | (static_cast<quint32>(p[2]) << 16)
            | (static_cast<quint32>(p[3]) << 24);
        float f = 0.0f;
        static_assert(sizeof(float) == 4, "float must be 32-bit");
        std::memcpy(&f, &bits, sizeof(f));
        values[i] = f;
    }
    return values;
}

bool OpalList::parse(const QByteArray& data, OpalList& out)
{
    out = OpalList();

    // Header is version + count; an empty or truncated header is not a list.
    if (data.size() < 8)
        return false;

    int offset = 0;
    out.version = readU32(data, offset);
    const quint32 count = readU32(data, offset);

    out.placements.reserve(static_cast<int>(count));
    for (quint32 i = 0; i < count; ++i)
    {
        OpalPlacement placement;
        const quint32 nameLen = readU32(data, offset);
        if (offset + static_cast<int>(nameLen) + 1 > data.size())
            return false;
        placement.name = QString::fromLatin1(data.constData() + offset,
                                             static_cast<int>(nameLen));
        offset += static_cast<int>(nameLen);
        offset += 1;   // NUL terminator (always present on shipped data)

        const quint32 payloadLen = readU32(data, offset);
        if (offset + static_cast<int>(payloadLen) + 8 > data.size())
            return false;
        placement.payload = data.mid(offset, static_cast<int>(payloadLen));
        offset += static_cast<int>(payloadLen);

        placement.trailer = readU64(data, offset);
        out.placements.append(placement);
    }

    // Every shipped file consumes exactly its own size; a leftover tail means
    // the layout assumption is wrong, so reject rather than silently accept.
    if (offset != data.size())
        return false;

    return true;
}

bool OpalList::loadFile(const QString& path, OpalList& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("OpalList::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    if (!parse(data, out))
    {
        LOG_WARNING(QString("OpalList::loadFile: malformed .opl %1").arg(path));
        return false;
    }
    LOG_DEBUG(QString("OpalList: parsed %1 placements (version %2) from %3")
        .arg(out.rowCount()).arg(out.version).arg(path));
    return true;
}

QByteArray OpalList::serialize() const
{
    QByteArray out;
    writeU32(out, version);
    writeU32(out, static_cast<quint32>(placements.size()));

    for (const OpalPlacement& placement : placements)
    {
        const QByteArray name = placement.name.toLatin1();
        writeU32(out, static_cast<quint32>(name.size()));
        out.append(name);
        out.append('\0');
        writeU32(out, static_cast<quint32>(placement.payload.size()));
        out.append(placement.payload);
        writeU64(out, placement.trailer);
    }
    return out;
}

bool OpalList::saveFile(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        LOG_WARNING(QString("OpalList::saveFile: cannot write %1").arg(path));
        return false;
    }
    const QByteArray data = serialize();
    const bool ok = file.write(data) == data.size();
    file.close();
    return ok;
}

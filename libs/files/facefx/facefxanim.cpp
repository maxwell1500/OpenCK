#include "facefxanim.hpp"

#include <QFile>
#include <QtEndian>

#include <cstring>

namespace FaceFx {

namespace {

constexpr char kMagic[6] = { '_', '_', 'f', 'f', 'x', '\0' };

quint32 readU32(const QByteArray& d, int off)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(d.constData() + off));
}

quint16 readU16(const QByteArray& d, int off)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(d.constData() + off));
}

quint32 readFloatBits(const QByteArray& d, int off)
{
    return readU32(d, off);
}

} // namespace

void FaceFxAnim::setError(const QString& msg)
{
    m_error = msg;
}

bool FaceFxAnim::load(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        setError(QStringLiteral("cannot open %1").arg(path));
        return false;
    }
    return parse(f.readAll());
}

bool FaceFxAnim::load(const QByteArray& data)
{
    return parse(data);
}

// File layout (as observed on Starfield - FaceAnimation01.ba2):
//   offset 0: "__ffx\0" (6 bytes)
//   offset 6: u16 format (observed 0x0300 = w/ version byte 3; treated as raw field)
//   offset 8: u32 total size, must match the file length
//   offset 12: 20-byte entry identifier
//   offset 32: u32 record count
//   offset 36: count * 12-byte records
//
// Each 12-byte record: f32 value (channel sample), u16 field0..field3.
bool FaceFxAnim::parse(const QByteArray& data)
{
    if (data.size() < 36 || data.left(6) != QByteArray(kMagic, 6)) {
        setError(QStringLiteral("bad magic"));
        return false;
    }
    m_header.format = readU16(data, 6);
    m_header.size = readU32(data, 8);
    if (m_header.size != static_cast<quint32>(data.size())) {
        setError(QStringLiteral("size mismatch: field says %1, buffer has %2")
                     .arg(m_header.size)
                     .arg(data.size()));
        return false;
    }
    m_header.id = data.mid(12, 20);
    m_header.count = readU32(data, 32);
    const quint64 walk = 36u + static_cast<quint64>(m_header.count) * 12u;
    if (walk != static_cast<quint64>(m_header.size)) {
        setError(QStringLiteral("record count drift: expect size 36 + %1*12 = %2, got %3")
                     .arg(m_header.count)
                     .arg(walk)
                     .arg(m_header.size));
        return false;
    }
    m_records.clear();
    m_records.reserve(static_cast<int>(m_header.count));
    for (quint32 i = 0; i < m_header.count; ++i) {
        const int off = 36 + static_cast<int>(i) * 12;
        AnimRecord r;
        quint32 bits = readU32(data, off);
        std::memcpy(&r.value, &bits, sizeof(r.value));
        r.field0 = readU16(data, off + 4);
        r.field1 = readU16(data, off + 6);
        r.field2 = readU16(data, off + 8);
        r.field3 = readU16(data, off + 10);
        m_records.append(r);
    }
    return true;
}

} // namespace FaceFx

#include "facefxactor.hpp"

#include <QFile>
#include <QtEndian>

namespace FaceFx {

namespace {

quint32 readU32(const QByteArray& d, int off)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(d.constData() + off));
}

quint16 readU16(const QByteArray& d, int off)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(d.constData() + off));
}

bool readStr0(const QByteArray& d, int& off, QString& out)
{
    if (off + 4 > d.size()) return false;
    const quint32 len = readU32(d, off);
    off += 4;
    if (len == 0 || len > 1024 || off + static_cast<int>(len) > d.size()) return false;
    out = QString::fromLatin1(d.constData() + off, static_cast<int>(len) - 1);
    off += static_cast<int>(len);
    return true;
}

bool isFxTypeName(const QString& s)
{
    if (s.isEmpty()) return false;
    for (const QChar c : s) {
        if (!c.unicode()) return false;
    }
    return s.startsWith(QLatin1String("Fx"));
}

} // namespace

void FaceFxActor::setError(const QString& msg)
{
    m_error = msg;
}

bool FaceFxActor::load(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        setError(QStringLiteral("cannot open %1").arg(path));
        return false;
    }
    return load(f.readAll());
}

bool FaceFxActor::load(const QByteArray& data)
{
    m_header = ActorHeader();
    m_types.clear();
    m_names.clear();
    m_error.clear();
    return parse(data);
}

bool FaceFxActor::parse(const QByteArray& d)
{
    if (d.size() < 12) {
        setError(QStringLiteral("file too small"));
        return false;
    }
    if (d.mid(0, 4) != QByteArrayLiteral("FACE")) {
        setError(QStringLiteral("bad magic"));
        return false;
    }

    int off = 4;
    m_header.version = readU32(d, off); off += 4;
    m_header.opaque = readU32(d, off); off += 4;

    if (!readStr0(d, off, m_header.publisher)) {
        setError(QStringLiteral("bad publisher string"));
        return false;
    }
    if (!readStr0(d, off, m_header.project)) {
        setError(QStringLiteral("bad project string"));
        return false;
    }

    if (off + 4 + 2 + 16 > d.size()) {
        setError(QStringLiteral("truncated header"));
        return false;
    }
    m_header.constant1000 = readU32(d, off); off += 4;
    m_header.constant2 = readU16(d, off); off += 2;
    m_header.typeCount = readU32(d, off); off += 4;
    m_header.fieldB = readU32(d, off); off += 4;
    m_header.fieldC = readU32(d, off); off += 4;
    m_header.fieldD = readU32(d, off); off += 4;

    const quint32 typeCount = m_header.typeCount;
    if (typeCount == 0 || typeCount > 256) {
        setError(QStringLiteral("implausible type count %1").arg(typeCount));
        return false;
    }

    // Type table: A entries of [u32 id][u32 len][name][variable payload].
    // Payload size is not fixed (observed 14/28/30 across files), so we scan
    // forward for each expected sequential id with an Fx-prefixed name.
    for (quint32 expectId = 0; expectId < typeCount; ++expectId) {
        bool found = false;
        const int scanLimit = d.size() - 8;
        for (int p = off; p <= scanLimit; ++p) {
            const quint32 id = readU32(d, p);
            if (id != expectId) continue;
            const quint32 len = readU32(d, p + 4);
            if (len == 0 || len > 512) continue;
            if (p + 8 + static_cast<int>(len) > d.size()) continue;
            const QString name = QString::fromLatin1(d.constData() + p + 8, static_cast<int>(len));
            if (!isFxTypeName(name)) continue;
            m_types.append(TypeEntry{ id, name });
            off = p + 8 + static_cast<int>(len);
            found = true;
            break;
        }
        if (!found) {
            setError(QStringLiteral("type entry %1 not found").arg(expectId));
            return false;
        }
    }

    // Name table: [u32 id][u32 len][name][12B payload] until id == 0xFFFFFFFF.
    // After the last type entry we sit at its payload start; the table begins
    // somewhere after that payload (size varies), so scan for a clean run.
    int nameStart = -1;
    const int scanEnd = d.size() - 8;
    for (int p = off; p <= scanEnd; ++p) {
        int q = p;
        int count = 0;
        bool ok = true;
        while (q + 8 <= d.size()) {
            const quint32 id = readU32(d, q);
            if (id == 0xFFFFFFFFu) break;
            const quint32 len = readU32(d, q + 4);
            if (len == 0 || len > 512 || q + 8 + static_cast<int>(len) + 12 > d.size()) {
                ok = false;
                break;
            }
            const char* nm = d.constData() + q + 8;
            for (quint32 i = 0; i < len; ++i) {
                const uchar c = static_cast<uchar>(nm[i]);
                if (c < 0x20 || c >= 0x7F) { ok = false; break; }
            }
            if (!ok) break;
            q = q + 8 + static_cast<int>(len) + 12;
            if (++count > 4096) { ok = false; break; }
        }
        if (ok && count >= 1 && q + 4 <= d.size() && readU32(d, q) == 0xFFFFFFFFu) {
            nameStart = p;
            break;
        }
    }
    if (nameStart < 0) {
        setError(QStringLiteral("name table not found"));
        return false;
    }

    off = nameStart;
    while (off + 8 <= d.size()) {
        const quint32 id = readU32(d, off);
        if (id == 0xFFFFFFFFu) break;
        const quint32 len = readU32(d, off + 4);
        if (len == 0 || len > 512 || off + 8 + static_cast<int>(len) + 12 > d.size()) {
            setError(QStringLiteral("bad name entry at %1").arg(off));
            return false;
        }
        const QString name = QString::fromLatin1(d.constData() + off + 8, static_cast<int>(len));
        const int payloadOff = off + 8 + static_cast<int>(len);
        NameEntry ne;
        ne.id = id;
        ne.name = name;
        ne.payload[0] = readU32(d, payloadOff);
        ne.payload[1] = readU32(d, payloadOff + 4);
        ne.payload[2] = readU32(d, payloadOff + 8);
        m_names.append(ne);
        off = payloadOff + 12;
    }

    if (m_names.isEmpty()) {
        setError(QStringLiteral("no name entries"));
        return false;
    }
    return true;
}

} // namespace FaceFx

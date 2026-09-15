#include "reflectstream.hpp"

#include <QSet>

namespace {

// Primitive/serialization type names that appear in the embedded schema. Any
// NUL-terminated token matching one of these is classified as a type rather
// than a field, alongside tokens containing "::".
bool isKnownTypeName(const QString& s)
{
    static const QSet<QString> known = {
        QStringLiteral("XMFLOAT2"), QStringLiteral("XMFLOAT3"),
        QStringLiteral("XMFLOAT4"), QStringLiteral("XMFLOAT4X4"),
        QStringLiteral("XMFLOAT3X3"), QStringLiteral("XMFLOAT4X3"),
        QStringLiteral("float"), QStringLiteral("double"),
        QStringLiteral("int"), QStringLiteral("int8_t"),
        QStringLiteral("int16_t"), QStringLiteral("int32_t"), QStringLiteral("int64_t"),
        QStringLiteral("uint8_t"), QStringLiteral("uint16_t"),
        QStringLiteral("uint32_t"), QStringLiteral("uint64_t"),
        QStringLiteral("bool"), QStringLiteral("char"), QStringLiteral("void"),
        QStringLiteral("REFL"), QStringLiteral("BETH"), QStringLiteral("STRT"),
    };
    return known.contains(s);
}

bool isPrintableAscii(char c)
{
    const unsigned char u = static_cast<unsigned char>(c);
    return u >= 0x20 && u < 0x7F;
}

} // namespace

bool ReflectionStream::hasField(const QString& name) const
{
    return fieldNames.contains(name);
}

ReflectionStream parseReflectionStream(const QByteArray& data)
{
    ReflectionStream stream;
    stream.raw = data;

    if (data.size() < 8 || data.left(4) != QByteArrayLiteral("BETH"))
        return stream;

    stream.valid = true;

    const uchar* p = reinterpret_cast<const uchar*>(data.constData());
    stream.version = static_cast<quint32>(p[4])
        | (static_cast<quint32>(p[5]) << 8)
        | (static_cast<quint32>(p[6]) << 16)
        | (static_cast<quint32>(p[7]) << 24);

    // Walk every NUL-terminated printable-ASCII run of a plausible length.
    int i = 4;
    while (i < data.size())
    {
        if (!isPrintableAscii(data.at(i)))
        {
            ++i;
            continue;
        }
        const int start = i;
        while (i < data.size() && isPrintableAscii(data.at(i)))
            ++i;
        const int len = i - start;
        const bool terminated = (i < data.size() && data.at(i) == '\0');
        if (!terminated || len < 2 || len > 127)
            continue;

        const QString s = QString::fromLatin1(data.constData() + start, len);
        if (s.contains(QStringLiteral("::")) || isKnownTypeName(s))
        {
            stream.typeNames.append(s);
            if (stream.rootType.isEmpty() && s.contains(QStringLiteral("::")))
                stream.rootType = s;
        }
        else if (s != QStringLiteral("BETH"))
        {
            stream.fieldNames.append(s);
        }
    }

    return stream;
}

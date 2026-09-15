#ifndef REFLECTSTREAM_HPP
#define REFLECTSTREAM_HPP

#include <QByteArray>
#include <QString>
#include <QStringList>

// Starfield "reflection" data streams (REFL/RDIF/PCCC/PTCL/XNSE/PSDF) — the
// serialized type-schema + value blobs xEdit's wbReflection marks as not fully
// decoded. They are not raw noise: every stream starts with a "BETH" magic and
// carries an embedded schema of NUL-terminated type and field names, e.g.
// "BSGalaxy::BGSSunPresetForm" with fields "pParent", "XMFLOST4"/"XMFLOAT4",
// "x", "y", "z", "w", "SunColor", "SunIlluminance", …
//
// This parser extracts that schema (root type, field names, referenced type
// names) so the stream is no longer an opaque blob, while keeping the exact
// bytes for byte-for-byte round-trip. Field *values* are still not mapped to
// their offsets — doing so needs the per-type layouts that neither OpenCK nor
// xEdit currently has.
struct ReflectionStream
{
    bool valid = false;            // "BETH" magic present
    quint32 version = 0;           // first header word after the magic
    QString rootType;              // first qualified name, e.g. "BSGalaxy::BGS..."
    QStringList typeNames;         // qualified + primitive type names, in order
    QStringList fieldNames;        // unqualified field identifiers, in order
    QByteArray raw;                // exact bytes (preserved for round-trip)

    // True when the named field appears in the embedded schema.
    bool hasField(const QString& name) const;
};

// Parses a reflection stream. Returns a stream with valid == false (and raw
// set) when the magic is absent.
ReflectionStream parseReflectionStream(const QByteArray& data);

#endif // REFLECTSTREAM_HPP

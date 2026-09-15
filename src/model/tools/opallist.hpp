#ifndef OPALLIST_HPP
#define OPALLIST_HPP

#include <QByteArray>
#include <QString>
#include <QVector>

// Starfield OPAL procedural-placement list (.opl) — the real binary format,
// decoded from the ten shipped lists under Content/OPAL and verified
// 2026-09-14 (all 3,311 entries parse with zero trailing bytes):
//
//   uint32 version           (= 3 in every shipped file)
//   uint32 count
//   repeat count times:
//     uint32 nameLen         (excludes the terminator)
//     char[nameLen] name     (editor id of the placed form, e.g. "Bar_Bowl02")
//     uint8  NUL             (always present)
//     uint32 payloadLen      (0 or 24 in every shipped entry)
//     byte[payloadLen]       (24 = 6 floats: position xyz, rotation xyz)
//     uint64 trailer         (high 32 bits always 0; low 32 is the FormID)
//
// Note: the earlier CSV/header implementation was a guess; the shipped files
// are binary and carry no header row. The payload is kept as raw bytes so
// unknown payload shapes still round-trip exactly.
struct OpalPlacement
{
    QString name;
    QByteArray payload;      // opaque; 24 bytes == 6 floats on shipped data
    quint64 trailer = 0;     // FormID in the low 32 bits

    bool hasTransform() const { return payload.size() == 24; }
    // Six floats (pos xyz, rot xyz) when hasTransform(), else empty.
    QVector<float> transform() const;
    quint32 formId() const { return static_cast<quint32>(trailer & 0xFFFFFFFFu); }
};

struct OpalList
{
    quint32 version = 3;
    QVector<OpalPlacement> placements;

    // Parses the binary .opl payload. Returns false on a truncated or
    // malformed buffer (nothing partially parsed is left behind).
    static bool parse(const QByteArray& data, OpalList& out);

    // Loads and parses the given .opl file.
    static bool loadFile(const QString& path, OpalList& out);

    // Serializes back to the on-disk layout.
    QByteArray serialize() const;

    // Writes serialize() to the given path.
    bool saveFile(const QString& path) const;

    int rowCount() const { return placements.size(); }
};

#endif // OPALLIST_HPP

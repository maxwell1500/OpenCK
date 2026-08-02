#ifndef ImgsRECORD_H
#define ImgsRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
#include <QByteArray>
#include <cstring>
class ESMReader;
class ESMWriter;

struct ImgsRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    // Typed view of the IMGS DATA subrecord: 48 image-space modifier floats
    // (HDR bloom/luminance/saturation etc.) followed by six RGBA zone colors.
    // Unpacked on load when a DATA subrecord is present; repacked on save.
    // Data larger than the standard 72 bytes is preserved in trailingBytes.
    struct Data
    {
        bool present = false;

        // The 48 documented modifier floats. The first twelve carry the
        // well-known HDR parameters; the rest are preserved verbatim.
        float values[48] = {};

        // Six zone colors (RGBA), each present when the payload reaches that
        // offset (48 + i*4). Byte count is validated defensively.
        quint8 color[6][4] = {};

        QByteArray trailingBytes;  // any payload beyond 72 bytes
    } data;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const ImgsRecord& l, const ImgsRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.data.present == r.data.present
        && memcmp(l.data.values, r.data.values, sizeof(l.data.values)) == 0
        && memcmp(l.data.color, r.data.color, sizeof(l.data.color)) == 0
        && l.data.trailingBytes == r.data.trailingBytes
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}

inline bool operator!=(const ImgsRecord& l, const ImgsRecord& r)
{
    return !(l == r);
}
#endif

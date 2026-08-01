#ifndef EfshRECORD_H
#define EfshRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
class ESMReader;
class ESMWriter;

struct EfshRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    openck::FormComponents components;

    // Typed view of the DATA subrecord (Skyrim EFSH): fill/rim/base colors
    // (RGBA bytes) and their intensity scales. Unpacked on load when the
    // DATA subrecord is present; repacked on save.
    struct Data
    {
        bool present = false;
        quint32 shaderFlags = 0;
        quint8 fillR = 255, fillG = 255, fillB = 255, fillA = 255;
        quint8 rimR = 255, rimG = 255, rimB = 255, rimA = 255;
        quint8 baseR = 255, baseG = 255, baseB = 255, baseA = 255;
        float fillScale = 1.0f;
        float rimScale = 1.0f;
        float baseScale = 1.0f;
        quint32 unk1 = 0;
        quint32 unk2 = 0;
    } data;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const EfshRecord& l, const EfshRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.data.present == r.data.present && l.data.shaderFlags == r.data.shaderFlags
        && l.data.fillR == r.data.fillR && l.data.fillG == r.data.fillG
        && l.data.fillB == r.data.fillB && l.data.fillA == r.data.fillA
        && l.data.rimR == r.data.rimR && l.data.rimG == r.data.rimG
        && l.data.rimB == r.data.rimB && l.data.rimA == r.data.rimA
        && l.data.baseR == r.data.baseR && l.data.baseG == r.data.baseG
        && l.data.baseB == r.data.baseB && l.data.baseA == r.data.baseA
        && l.data.fillScale == r.data.fillScale && l.data.rimScale == r.data.rimScale
        && l.data.baseScale == r.data.baseScale
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const EfshRecord& l, const EfshRecord& r) { return !(l == r); }
#endif

#ifndef LighRECORD_H
#define LighRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include "../../components/tesfullname.hpp"
#include "../../components/tier1_components.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct LighRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString fullName;
    QString iconPath;
    QString modelPath;
    qint32 time = 0;
    quint32 radius = 0;
    quint32 color = 0;
    quint32 lightFlags = 0;
    float falloff = 0.0f;
    float fov = 0.0f;
    float fade = 0.0f;
    quint32 value = 0;
    float weight = 0.0f;
    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const LighRecord& l, const LighRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.fullName == r.fullName && l.iconPath == r.iconPath
        && l.modelPath == r.modelPath && l.time == r.time
        && l.radius == r.radius && l.color == r.color
        && l.lightFlags == r.lightFlags && l.falloff == r.falloff
        && l.fov == r.fov && l.fade == r.fade
        && l.value == r.value && l.weight == r.weight
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}

inline bool operator!=(const LighRecord& l, const LighRecord& r)
{
    return !(l == r);
}
#endif

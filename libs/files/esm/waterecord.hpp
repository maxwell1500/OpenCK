#ifndef WateRECORD_H
#define WateRECORD_H
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
struct WateRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString fullName;
    QString iconPath;
    quint32 waterFlags = 0;
    qint32 color = 0;
    float windVel = 0.0f;
    float waveHeight = 0.0f;
    float damage = 0.0f;
    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const WateRecord& l, const WateRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.fullName == r.fullName && l.iconPath == r.iconPath
        && l.waterFlags == r.waterFlags && l.color == r.color
        && l.windVel == r.windVel && l.waveHeight == r.waveHeight
        && l.damage == r.damage
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}

inline bool operator!=(const WateRecord& l, const WateRecord& r)
{
    return !(l == r);
}
#endif

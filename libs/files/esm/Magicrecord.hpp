#ifndef MagicRECORD_H
#define MagicRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct MagicRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    quint32 schools = 0;
    quint32 damageType = 0;
    quint32 castingSound = 0;
    QString iconPath;
    QString modelPath;
    QVector<quint32> effects;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const MagicRecord& l, const MagicRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.schools == r.schools && l.damageType == r.damageType
        && l.castingSound == r.castingSound && l.iconPath == r.iconPath
        && l.modelPath == r.modelPath && l.effects == r.effects
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const MagicRecord& l, const MagicRecord& r)
{
    return !(l == r);
}
#endif

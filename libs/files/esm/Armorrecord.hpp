#ifndef ArmorRECORD_H
#define ArmorRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct ArmorRecord {
    openck::FormComponents components;
    QString editorId;
    QString fullName;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    quint32 armorRating = 0;
    float weight = 0.0f;
    quint32 value = 0;
    QString iconPath;
    QString modelPath;
    float health = 0.0f;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const ArmorRecord& l, const ArmorRecord& r)
{
    return l.editorId == r.editorId && l.fullName == r.fullName && l.formId == r.formId && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords && l.armorRating == r.armorRating
        && l.weight == r.weight && l.value == r.value && l.iconPath == r.iconPath
        && l.modelPath == r.modelPath && l.health == r.health;
}

inline bool operator!=(const ArmorRecord& l, const ArmorRecord& r)
{
    return !(l == r);
}
#endif

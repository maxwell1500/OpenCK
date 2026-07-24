#ifndef PerkRECORD_H
#define PerkRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct PerkRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString description;
    QString requirements;
    QString iconPath;
    QVector<quint32> conditions;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const PerkRecord& l, const PerkRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.description == r.description && l.requirements == r.requirements
        && l.iconPath == r.iconPath && l.conditions == r.conditions
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const PerkRecord& l, const PerkRecord& r)
{
    return !(l == r);
}
#endif

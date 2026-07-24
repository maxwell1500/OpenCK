#ifndef StatRECORD_H
#define StatRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct StatRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    // Item-specific fields
    QString iconPath;
    QString modelPath;
    QString lodModelPath;
    quint32 lodFlags = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const StatRecord& l, const StatRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.iconPath == r.iconPath && l.modelPath == r.modelPath
        && l.lodModelPath == r.lodModelPath && l.lodFlags == r.lodFlags
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const StatRecord& l, const StatRecord& r)
{
    return !(l == r);
}
#endif

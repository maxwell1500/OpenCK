#ifndef ActiRECORD_H
#define ActiRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct ActiRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    // Item-specific fields
    QString iconPath;
    QString modelPath;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const ActiRecord& l, const ActiRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.iconPath == r.iconPath && l.modelPath == r.modelPath
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const ActiRecord& l, const ActiRecord& r)
{
    return !(l == r);
}
#endif

#ifndef ContRECORD_H
#define ContRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct ContRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    // Item-specific fields
    QString iconPath;
    QString modelPath;
    quint32 contents = 0;
    quint32 inventoryControl = 0;
    float weight = 0.0f;
    quint32 value = 0;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const ContRecord& l, const ContRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords && l.iconPath == r.iconPath
        && l.modelPath == r.modelPath && l.contents == r.contents
        && l.inventoryControl == r.inventoryControl && l.weight == r.weight
        && l.value == r.value;
}

inline bool operator!=(const ContRecord& l, const ContRecord& r)
{
    return !(l == r);
}
#endif

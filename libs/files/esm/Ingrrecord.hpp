#ifndef IngrRECORD_H
#define IngrRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct IngrRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString iconPath;
    QString modelPath;
    float weight = 0.0f;
    quint32 value = 0;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const IngrRecord& l, const IngrRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.iconPath == r.iconPath && l.modelPath == r.modelPath
        && l.weight == r.weight && l.value == r.value
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const IngrRecord& l, const IngrRecord& r)
{
    return !(l == r);
}
#endif

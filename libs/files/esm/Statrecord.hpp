#ifndef StatRECORD_H
#define StatRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct StatRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString iconPath;
    QString smallIconPath;
    QString modelPath;
    QString lodModelPath;
    quint32 lodFlags = 0;
    QVector<RawSubRecord> rawSubRecords;
    // Subrecord order seen at load, so an untouched save re-emits the
    // record payload-identical (the snapshot gate compares positionally).
    QVector<NAME> loadOrder;
    bool hasFlags = false;
    quint32 flagsSpelling = NAME('FNAM');
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const StatRecord& l, const StatRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.iconPath == r.iconPath && l.smallIconPath == r.smallIconPath
        && l.modelPath == r.modelPath
        && l.lodModelPath == r.lodModelPath && l.lodFlags == r.lodFlags
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const StatRecord& l, const StatRecord& r)
{
    return !(l == r);
}
#endif

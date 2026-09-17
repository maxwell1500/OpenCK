#ifndef ActiRECORD_H
#define ActiRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct ActiRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString iconPath;
    QString modelPath;
    QVector<RawSubRecord> rawSubRecords;
    QVector<NAME> loadOrder;
    bool hasFlags = false;
    NAME flagsSpelling = NAME('FNAM');
    quint8 flagsWidth = 4;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
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

#ifndef ClassRECORD_H
#define ClassRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct ClassRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString className;
    QString description;
    quint32 serviceFlags = 0;
    QString iconPath;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const ClassRecord& l, const ClassRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.className == r.className && l.description == r.description
        && l.serviceFlags == r.serviceFlags && l.iconPath == r.iconPath
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const ClassRecord& l, const ClassRecord& r)
{
    return !(l == r);
}
#endif

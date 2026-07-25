#ifndef FactRECORD_H
#define FactRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct FactRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString factionName;
    QString description;
    QString iconPath;
    QVector<QString> ranks;
    QVector<quint32> relations;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const FactRecord& l, const FactRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.factionName == r.factionName && l.description == r.description
        && l.iconPath == r.iconPath && l.ranks == r.ranks && l.relations == r.relations
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const FactRecord& l, const FactRecord& r)
{
    return !(l == r);
}
#endif

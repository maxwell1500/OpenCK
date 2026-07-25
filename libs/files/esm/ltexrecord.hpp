#ifndef LTEXRECORD_HPP
#define LTEXRECORD_HPP

#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

struct LtexRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString iconPath;
    quint32 havokMaterial = 0;
    QVector<quint32> grassFormIds;
    QVector<RawSubRecord> rawSubRecords;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const LtexRecord& l, const LtexRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.flags == r.flags && l.iconPath == r.iconPath
        && l.havokMaterial == r.havokMaterial
        && l.grassFormIds == r.grassFormIds
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const LtexRecord& l, const LtexRecord& r)
{
    return !(l == r);
}

#endif // LTEXRECORD_HPP

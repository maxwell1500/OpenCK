#ifndef PackageRECORD_H
#define PackageRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include "../../components/tier3_components.hpp"
#include "conditionrecord.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct PackageRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    quint32 packageType = 0;
    quint32 targetType = 0;
    QVector<quint32> targetIds;
    QVector<quint32> parameters;
    QVector<CtdaCondition> conditions;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const PackageRecord& l, const PackageRecord& r)
{
    return l.components == r.components && l.editorId == r.editorId
        && l.formId == r.formId && l.flags == r.flags
        && l.packageType == r.packageType && l.targetType == r.targetType
        && l.targetIds == r.targetIds && l.parameters == r.parameters
        && l.conditions == r.conditions
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const PackageRecord& l, const PackageRecord& r)
{
    return !(l == r);
}
#endif

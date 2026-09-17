#ifndef SCENRECORD_H
#define SCENRECORD_H

#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include "conditionrecord.hpp"

#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

// Scene (SCEN) record. The Editor ID is a typed field and CTDA conditions
// are parsed/round-tripped as typed conditions; the remaining scene
// definition subrecords (VNAM flags, PHDA phases, HNAM/SNAM action lists,
// DATA) round-trip losslessly through rawSubRecords.
struct ScenRecord
{
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<CtdaCondition> conditions;
    QVector<RawSubRecord> rawSubRecords;
    // Subrecord names in on-disk order so save can re-emit them positionally
    // (CTDA is parsed out of rawSubRecords and would otherwise be hoisted).
    QVector<NAME> loadOrder;
    // Per CTDA occurrence: index into rawSubRecords for an unparsed condition
    // or -1 when it lives in `conditions` (consumed in order).
    QVector<int> conditionOrder;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const ScenRecord& l, const ScenRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.conditions == r.conditions
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const ScenRecord& l, const ScenRecord& r) { return !(l == r); }

#endif // SCENRECORD_H

#ifndef SCENRECORD_H
#define SCENRECORD_H

#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"

#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

// Scene (SCEN) record. Only the Editor ID is parsed as a typed field;
// the scene definition subrecords (VNAM flags, CTDA conditions, PHDA
// phases, HNAM/SNAM action lists, DATA) round-trip losslessly through
// rawSubRecords until a proper scene editor lands.
struct ScenRecord
{
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const ScenRecord& l, const ScenRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const ScenRecord& l, const ScenRecord& r) { return !(l == r); }

#endif // SCENRECORD_H

#ifndef LvlcRECORD_H
#define LvlcRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include "lvlistrecord.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;

struct LvlcRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    quint8 chanceNone = 0;
    quint8 listFlags = 0;
    QVector<LvloEntry> entries;
    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const LvlcRecord& l, const LvlcRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.chanceNone == r.chanceNone && l.listFlags == r.listFlags
        && l.entries == r.entries
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}

inline bool operator!=(const LvlcRecord& l, const LvlcRecord& r)
{
    return !(l == r);
}
#endif

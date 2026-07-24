#ifndef LvliRECORD_H
#define LvliRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;

#pragma pack(push, 1)
struct LvloEntry {
    qint16 level = 0;
    quint32 formId = 0;
    qint16 count = 0;

    inline bool operator==(const LvloEntry& o) const {
        return level == o.level && formId == o.formId && count == o.count;
    }
    inline bool operator!=(const LvloEntry& o) const { return !(*this == o); }
};
#pragma pack(pop)

struct LvliRecord {
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

inline bool operator==(const LvliRecord& l, const LvliRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.chanceNone == r.chanceNone && l.listFlags == r.listFlags
        && l.entries == r.entries
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}

inline bool operator!=(const LvliRecord& l, const LvliRecord& r)
{
    return !(l == r);
}
#endif

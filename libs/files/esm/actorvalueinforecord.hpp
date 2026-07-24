#ifndef AvifRECORD_H
#define AvifRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
class ESMReader;
class ESMWriter;

struct ActorValueInfoRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    openck::FormComponents components;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const ActorValueInfoRecord& l, const ActorValueInfoRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}
inline bool operator!=(const ActorValueInfoRecord& l, const ActorValueInfoRecord& r) { return !(l == r); }
#endif

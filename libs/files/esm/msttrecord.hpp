#ifndef MsttRECORD_H
#define MsttRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include "../../components/tier1_components.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct MsttRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString modelPath;
    quint32 msttFlags = 0;
    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const MsttRecord& l, const MsttRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.modelPath == r.modelPath && l.msttFlags == r.msttFlags
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}

inline bool operator!=(const MsttRecord& l, const MsttRecord& r)
{
    return !(l == r);
}
#endif

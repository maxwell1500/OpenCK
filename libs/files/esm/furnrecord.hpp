#ifndef FurnRECORD_H
#define FurnRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/component.hpp"
#include "../../components/formcomponents.hpp"
#include "../../components/tesfullname.hpp"
#include "../../components/tier1_components.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct FurnRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString fullName;
    QString modelPath;
    quint32 markerCount = 0;
    QVector<RawSubRecord> rawSubRecords;

    openck::FormComponents components;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const FurnRecord& l, const FurnRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.fullName == r.fullName && l.modelPath == r.modelPath
        && l.markerCount == r.markerCount
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}

inline bool operator!=(const FurnRecord& l, const FurnRecord& r)
{
    return !(l == r);
}
#endif

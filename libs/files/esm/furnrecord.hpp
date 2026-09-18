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

#include <memory>
class ESMReader;
class ESMWriter;
struct FurnRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString fullName;
    QString modelPath;
    QVector<RawSubRecord> rawSubRecords;
    QVector<NAME> loadOrder;
    bool hasFlags = false;
    NAME flagsSpelling = NAME('FNAM');
    quint8 flagsWidth = 4;

    openck::FormComponents components;

    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<FurnRecord> verbatimSnapshot;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const FurnRecord& l, const FurnRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.fullName == r.fullName         && l.modelPath == r.modelPath
        && l.rawSubRecords == r.rawSubRecords && l.components == r.components;
}

inline bool operator!=(const FurnRecord& l, const FurnRecord& r)
{
    return !(l == r);
}
#endif

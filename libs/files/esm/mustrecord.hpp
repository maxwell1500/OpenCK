#ifndef MustRECORD_H
#define MustRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

#include <memory>
class ESMReader;
class ESMWriter;
struct MustRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QString musicFile;
    quint32 flags = 0;
    QVector<RawSubRecord> rawSubRecords;
    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<MustRecord> verbatimSnapshot;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const MustRecord& l, const MustRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.musicFile == r.musicFile && l.flags == r.flags
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const MustRecord& l, const MustRecord& r)
{
    return !(l == r);
}
#endif

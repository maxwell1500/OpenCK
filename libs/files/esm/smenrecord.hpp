#ifndef SmenRecordRECORD_H
#define SmenRecordRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

#include <memory>
class ESMReader;
class ESMWriter;
struct SmenRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QVector<RawSubRecord> rawSubRecords;
    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<SmenRecord> verbatimSnapshot;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const SmenRecord& l, const SmenRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const SmenRecord& l, const SmenRecord& r)
{
    return !(l == r);
}
#endif
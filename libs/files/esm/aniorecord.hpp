#ifndef AnioRECORD_H
#define AnioRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

#include <memory>
class ESMReader;
class ESMWriter;
struct AnioRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QString modelPath;
    QVector<RawSubRecord> rawSubRecords;
    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<AnioRecord> verbatimSnapshot;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const AnioRecord& l, const AnioRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.modelPath == r.modelPath && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const AnioRecord& l, const AnioRecord& r)
{
    return !(l == r);
}
#endif

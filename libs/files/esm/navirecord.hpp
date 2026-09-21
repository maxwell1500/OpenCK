#ifndef NaviRECORD_H
#define NaviRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

#include <memory>
class ESMReader;
class ESMWriter;
struct NaviRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QVector<RawSubRecord> rawSubRecords;
    QVector<NAME> loadOrder;
    bool hasEdid = false;
    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<NaviRecord> verbatimSnapshot;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const NaviRecord& l, const NaviRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.rawSubRecords == r.rawSubRecords
        && l.components == r.components;
}

inline bool operator!=(const NaviRecord& l, const NaviRecord& r)
{
    return !(l == r);
}
#endif

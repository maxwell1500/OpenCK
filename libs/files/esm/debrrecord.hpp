#ifndef DebrRECORD_H
#define DebrRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;

struct DebrisEntry {
    QString modelPath;
    quint32 count = 0;
    quint16 scale = 100;
    quint16 flags = 0;

    inline bool operator==(const DebrisEntry& o) const {
        return modelPath == o.modelPath && count == o.count
            && scale == o.scale && flags == o.flags;
    }
    inline bool operator!=(const DebrisEntry& o) const { return !(*this == o); }
};

struct DebrRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    QVector<DebrisEntry> debris;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const DebrRecord& l, const DebrRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.debris == r.debris && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const DebrRecord& l, const DebrRecord& r)
{
    return !(l == r);
}
#endif

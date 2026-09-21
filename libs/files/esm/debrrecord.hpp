#ifndef DebrRECORD_H
#define DebrRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

#include <memory>
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
    // Starfield DEBR DATA is not the u32-count/256-byte-model struct the
    // parser assumed; the payload is preserved raw and re-emitted verbatim
    // (debris stays a best-effort display parse).
    bool hasEdid = false;
    QVector<NAME> loadOrder;
    // Preserved DATA payload (not part of rawSubRecords, so an assembled
    // record and a loaded one compare equal). DATA repeats; dataRaw mirrors
    // the last, dataRaws holds every occurrence for positional replay.
    QByteArray dataRaw;
    QVector<QByteArray> dataRaws;
    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<DebrRecord> verbatimSnapshot;
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

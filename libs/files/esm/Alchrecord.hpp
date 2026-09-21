#ifndef AlchRECORD_H
#define AlchRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>

#include <memory>
class ESMReader;
class ESMWriter;
struct AlchRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString iconPath;
    QString modelPath;
    float weight = 0.0f;
    quint32 value = 0;
    QVector<RawSubRecord> rawSubRecords;
    // Width preservation: Starfield writes weight-only 4-byte DATA (legacy
    // records carry weight + value = 8). dataFields counts the u32 slots
    // present at load; the save re-emits exactly that width so an
    // unconditional 8-byte write neither over-reads at load nor appends a
    // spurious subrecord.
    int dataFields = 2;
    bool hasData = false;
    bool hasFlags = false;
    // Positional replay (see ArmorRecord): loadOrder + loadIsRaw, EDID gate.
    QVector<NAME> loadOrder;
    QVector<quint8> loadIsRaw;
    bool hasEdid = false;
    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<AlchRecord> verbatimSnapshot;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const AlchRecord& l, const AlchRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.iconPath == r.iconPath && l.modelPath == r.modelPath
        && l.weight == r.weight && l.value == r.value
        && l.components == r.components
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const AlchRecord& l, const AlchRecord& r)
{
    return !(l == r);
}
#endif

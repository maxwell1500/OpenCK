#ifndef RaceRECORD_H
#define RaceRECORD_H
#include "records.hpp"
#include "variant.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct RaceRecord {
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    quint32 raceFlags = 0;
    QVector<quint32> npcVariables;
    QVector<quint32> faceData;
    QVector<quint32> headData;
    QVector<RawSubRecord> rawSubRecords;
    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const RaceRecord& l, const RaceRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.flags == r.flags
        && l.raceFlags == r.raceFlags && l.npcVariables == r.npcVariables
        && l.faceData == r.faceData && l.headData == r.headData
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const RaceRecord& l, const RaceRecord& r)
{
    return !(l == r);
}
#endif

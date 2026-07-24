#ifndef REFRRECORD_H
#define REFRRECORD_H

#include "common.hpp"
#include "records.hpp"

#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

struct RefrRecord
{
    QString editorId;
    quint32 formId;
    quint32 baseId;
    float posX, posY, posZ;
    float rotX, rotY, rotZ;
    float scale;
    quint32 owner;
    quint32 lockLevel;
    bool initiallyDisabled;
    QVector<quint32> scriptIds;
    QVector<RawSubRecord> rawSubRecords;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const RefrRecord& l, const RefrRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.baseId == r.baseId
        && l.posX == r.posX && l.posY == r.posY && l.posZ == r.posZ
        && l.rotX == r.rotX && l.rotY == r.rotY && l.rotZ == r.rotZ
        && l.scale == r.scale && l.owner == r.owner && l.lockLevel == r.lockLevel
        && l.initiallyDisabled == r.initiallyDisabled && l.scriptIds == r.scriptIds
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const RefrRecord& l, const RefrRecord& r)
{
    return !(l == r);
}

#endif // REFRRECORD_H
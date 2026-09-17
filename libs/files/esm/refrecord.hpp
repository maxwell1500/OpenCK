#ifndef REFRRECORD_H
#define REFRRECORD_H

#include "common.hpp"
#include "records.hpp"
#include "../../components/formcomponents.hpp"

#include <QString>
#include <QVector>

#include <memory>

class ESMReader;
class ESMWriter;

struct RefrRecord
{
    openck::FormComponents components;
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
    // Subrecord names in on-disk order, recorded at load so save() can
    // replay them positionally (untouched round-trips stay identical).
    QVector<NAME> loadOrder;
    // True when an EDID subrecord was present at load (possibly empty).
    bool hasEdid = false;
    // Verbatim round-trip (see src/model/world/verbatimrecord.hpp).
    QByteArray verbatimBody;
    quint32 verbatimFlags = 0;
    std::shared_ptr<RefrRecord> verbatimSnapshot;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();

    // Applies a viewport transform (position/rotation in radians/scale) to the
    // reference. Shared by the viewport commit path and its tests.
    void applyTransform(float px, float py, float pz,
                        float rx, float ry, float rz, float scaleValue)
    {
        posX = px;
        posY = py;
        posZ = pz;
        rotX = rx;
        rotY = ry;
        rotZ = rz;
        scale = scaleValue;
    }
};

inline bool operator==(const RefrRecord& l, const RefrRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId && l.baseId == r.baseId
        && l.posX == r.posX && l.posY == r.posY && l.posZ == r.posZ
        && l.rotX == r.rotX && l.rotY == r.rotY && l.rotZ == r.rotZ
        && l.scale == r.scale && l.owner == r.owner && l.lockLevel == r.lockLevel
        && l.initiallyDisabled == r.initiallyDisabled && l.scriptIds == r.scriptIds
        && l.components == r.components
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const RefrRecord& l, const RefrRecord& r)
{
    return !(l == r);
}

#endif // REFRRECORD_H
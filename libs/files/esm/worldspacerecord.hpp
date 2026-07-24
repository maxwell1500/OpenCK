#ifndef WORLSPACE_H
#define WORLSPACE_H

#include "common.hpp"
#include "records.hpp"

#include <QString>
#include <QVector>

class ESMReader;
class ESMWriter;

struct WorldspaceRecord
{
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString name;
    QString iconPath;
    quint32 waterType = 0;
    quint32 climateId = 0;
    quint32 lightingId = 0;
    quint32 mapSize = 0;
    quint32 templ = 0;
    quint32 terrain = 0;
    QString mapImage;
    QString lodNoise;
    QString billboardTexture;
    quint32 music = 0;
    quint32 dnam = 0;
    qint32 dataMinX = 0;
    qint32 dataMinY = 0;
    QVector<quint32> cellIds;
    QVector<quint32> navPointIds;
    QVector<RawSubRecord> rawSubRecords;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
};

inline bool operator==(const WorldspaceRecord& l, const WorldspaceRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.flags == r.flags
        && l.name == r.name && l.iconPath == r.iconPath
        && l.waterType == r.waterType && l.climateId == r.climateId
        && l.lightingId == r.lightingId && l.mapSize == r.mapSize
        && l.templ == r.templ && l.terrain == r.terrain
        && l.mapImage == r.mapImage && l.lodNoise == r.lodNoise
        && l.billboardTexture == r.billboardTexture
        && l.music == r.music && l.dnam == r.dnam
        && l.dataMinX == r.dataMinX && l.dataMinY == r.dataMinY
        && l.cellIds == r.cellIds && l.navPointIds == r.navPointIds
        && l.rawSubRecords == r.rawSubRecords;
}

inline bool operator!=(const WorldspaceRecord& l, const WorldspaceRecord& r)
{
    return !(l == r);
}

#endif // WORLSPACE_H

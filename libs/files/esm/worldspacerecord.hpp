#ifndef WORLSPACE_H
#define WORLSPACE_H

#include "common.hpp"
#include "records.hpp"
#include "../../components/formcomponents.hpp"

#include <QString>
#include <QByteArray>
#include <QVector>

class ESMReader;
class ESMWriter;

// Worldspace (WRLD) record. Load/save follows the Skyrim SE layout (verified
// against Skyrim.esm): EDID, FULL (name), XNAM water, CNAM climate, ZNAM
// lighting, NAM2/NAM3 map dimensions, MNAM map cell bounds, ONAM map scale,
// NAMA LOD bias, DATA flags, DNAM bounds; every other subrecord (incl. the
// BFCB/BFCE custom-data blocks) is preserved verbatim in rawSubRecords so the
// round-trip is byte-exact. Subrecord order is kept in mOrder.
struct WorldspaceRecord
{
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;
    QString name;
    QString iconPath;
    quint32 waterType = 0;    // XNAM
    quint32 climateId = 0;    // CNAM
    quint32 lightingId = 0;   // ZNAM
    quint32 mapWidth = 0;     // NAM2
    quint32 mapHeight = 0;    // NAM3
    qint32 mapNwX = 0;        // MNAM (map cell bounds)
    qint32 mapNwY = 0;
    qint32 mapSeX = 0;
    qint32 mapSeY = 0;
    float mapLodBias = 1.0f;  // NAMA
    QByteArray onamData;      // ONAM (map scale float + padding)
    QByteArray dataFlags;     // DATA (raw; 1 byte in Skyrim)
    QByteArray dnamData;      // DNAM (raw bounds)
    quint32 templ = 0;        // TNAM (TES4 parent); unused for Skyrim
    quint32 terrain = 0;      // WNAM; unused for Skyrim
    QString mapImage;         // legacy; unused for Skyrim
    QString lodNoise;         // legacy; unused for Skyrim
    QString billboardTexture; // legacy; unused for Skyrim
    quint32 music = 0;        // RNAM; unused for Skyrim
    quint32 dnam = 0;         // legacy mirror; unused for Skyrim
    qint32 dataMinX = 0;      // == mapNwX (kept for cell-grid consumers)
    qint32 dataMinY = 0;      // == mapNwY
    quint32 mapSize = 0;      // == mapWidth (kept for exporters)
    QVector<quint32> cellIds;
    QVector<quint32> navPointIds;
    QVector<RawSubRecord> rawSubRecords;
    QVector<quint32> mOrder;  // subrecord emission order

    float mapScale() const;
    void setMapScale(float scale);

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const WorldspaceRecord& l, const WorldspaceRecord& r)
{
    return l.editorId == r.editorId && l.formId == r.formId
        && l.flags == r.flags
        && l.name == r.name && l.iconPath == r.iconPath
        && l.waterType == r.waterType && l.climateId == r.climateId
        && l.lightingId == r.lightingId
        && l.mapWidth == r.mapWidth && l.mapHeight == r.mapHeight
        && l.mapNwX == r.mapNwX && l.mapNwY == r.mapNwY
        && l.mapSeX == r.mapSeX && l.mapSeY == r.mapSeY
        && l.mapLodBias == r.mapLodBias
        && l.onamData == r.onamData
        && l.dataFlags == r.dataFlags && l.dnamData == r.dnamData
        && l.templ == r.templ && l.terrain == r.terrain
        && l.mapImage == r.mapImage && l.lodNoise == r.lodNoise
        && l.billboardTexture == r.billboardTexture
        && l.music == r.music && l.dnam == r.dnam
        && l.dataMinX == r.dataMinX && l.dataMinY == r.dataMinY
        && l.mapSize == r.mapSize
        && l.cellIds == r.cellIds && l.navPointIds == r.navPointIds
        && l.rawSubRecords == r.rawSubRecords
        && l.mOrder == r.mOrder
        && l.components == r.components;
}

inline bool operator!=(const WorldspaceRecord& l, const WorldspaceRecord& r)
{
    return !(l == r);
}

#endif // WORLSPACE_H

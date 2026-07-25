#ifndef LandRECORD_H
#define LandRECORD_H
#include "records.hpp"
#include "../../components/formcomponents.hpp"
#include <QString>
#include <QVector>
class ESMReader;
class ESMWriter;
struct LandRecord {
    openck::FormComponents components;
    QString editorId;
    quint32 formId = 0;
    quint32 flags = 0;

    qint32 cellX = 0;
    qint32 cellY = 0;

    struct Normal {
        qint8 nx = 0, ny = 0, nz = 0;
    };

    struct Color {
        quint8 r = 0, g = 0, b = 0, a = 0;
    };

    struct TextureLayer {
        quint32 textureFormId = 0;
        quint8 opacity = 0;
    };

    float baseHeight = 0.0f;

    bool hasHeightData = false;
    qint8 heightData[33][33] = {};

    bool hasNormalData = false;
    Normal normalData[33][33] = {};

    bool hasColorData = false;
    Color colorData[33][33] = {};

    TextureLayer textureLayers[4] = {};
    int numTextureLayers = 0;

    QVector<RawSubRecord> rawSubRecords;

    void load(ESMReader& esm, bool base);
    void save(ESMWriter& esm) const;
    void blank();
    void initComponents();
};

inline bool operator==(const LandRecord& l, const LandRecord& r)
{
    if (l.editorId != r.editorId || l.formId != r.formId || l.flags != r.flags
        || l.cellX != r.cellX || l.cellY != r.cellY
        || l.baseHeight != r.baseHeight
        || l.hasHeightData != r.hasHeightData || l.hasNormalData != r.hasNormalData
        || l.hasColorData != r.hasColorData || l.numTextureLayers != r.numTextureLayers
        || l.rawSubRecords != r.rawSubRecords || l.components != r.components)
        return false;

    if (l.hasHeightData && memcmp(l.heightData, r.heightData, sizeof(l.heightData)) != 0)
        return false;
    if (l.hasNormalData && memcmp(l.normalData, r.normalData, sizeof(l.normalData)) != 0)
        return false;
    if (l.hasColorData && memcmp(l.colorData, r.colorData, sizeof(l.colorData)) != 0)
        return false;
    for (int i = 0; i < l.numTextureLayers; ++i) {
        if (l.textureLayers[i].textureFormId != r.textureLayers[i].textureFormId
            || l.textureLayers[i].opacity != r.textureLayers[i].opacity)
            return false;
    }
    return true;
}

inline bool operator!=(const LandRecord& l, const LandRecord& r)
{
    return !(l == r);
}
#endif

#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

// Reads Starfield / Fallout 76 "Bethesda Terrain Data" (.btd, magic "BTDB")
// files: the worldspace height map and land-texture data. Layout follows
// fo76utils btdfile.cpp (Starfield variant: cell bounds are zero in the file
// and derived from the resolution; no ground-cover or vertex-color sections;
// each LOD0 zlib block holds a 128x128 int16 height map + 128x128 int16 land
// texture alphas).
struct BtdTerrain
{
    bool ok = false;
    quint32 version = 0;
    float worldHeightMin = 0.0f;
    float worldHeightMax = 0.0f;
    quint32 resX = 0;                 // total heightmap resolution
    quint32 resY = 0;
    qint32 cellMinX = 0, cellMinY = 0;
    qint32 cellMaxX = 0, cellMaxY = 0;
    quint32 nCellsX = 0, nCellsY = 0; // cells along each axis
    quint32 ltexCount = 0;
    QVector<quint32> ltexFormIds;     // LTEX record form IDs
    QVector<quint16> heightMapLOD4;      // nCellsY*8 * nCellsX*8
    QVector<quint16> landTexturesLOD4;   // nCellsY*8 * nCellsX*8
    QVector<quint16> cellHeightMap;      // 128*128 (cell 0, LOD0)
    QVector<quint16> cellLandTextures;   // 128*128

    static BtdTerrain read(const QByteArray& bytes);
};

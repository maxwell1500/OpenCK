#ifndef TERRAINLODGENERATOR_HPP
#define TERRAINLODGENERATOR_HPP

#include <QString>
#include <QImage>
#include <QVector>

#include "nifparser.hpp"
#include "../../libs/files/esm/landrecord.hpp"

class Data;

class TerrainLodGenerator
{
public:
    struct TerrainLodOptions
    {
        int lodLevel = 1;
        bool generateTextures = true;
        int textureResolution = 256;
    };

    struct TerrainLodResult
    {
        bool success = false;
        QString error;
        int verticesGenerated = 0;
        int trianglesGenerated = 0;
    };

    static TerrainLodResult generateTerrainMesh(const LandRecord& land,
                                                 const QString& outputPath,
                                                 const TerrainLodOptions& options);

    static TerrainLodResult generateWorldLod(Data& data,
                                              quint32 worldspaceFormId,
                                              const QString& outputDir,
                                              const TerrainLodOptions& options);

    static void heightmapToMesh(const qint8 heightData[33][33],
                                 float baseHeight,
                                 float heightScale,
                                 QVector<Nif::Vector3>& vertices,
                                 QVector<unsigned int>& indices);

    static QImage generateSplatMap(const LandRecord::Color colorData[33][33],
                                    int resolution);

private:
    static const float CELL_SIZE;
    static const float HEIGHT_SCALE;
    static const float BASE_HEIGHT;

    static void computeNormals(const QVector<Nif::Vector3>& vertices,
                                const QVector<unsigned int>& indices,
                                QVector<Nif::Vector3>& normals);

    static void computeUVs(const QVector<Nif::Vector3>& vertices,
                            float cellMinX, float cellMinY,
                            float cellWidth, float cellHeight,
                            QVector<Nif::Vector2>& uvs);
};

#endif // TERRAINLODGENERATOR_HPP

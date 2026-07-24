#include "terrainlodgenerator.hpp"
#include "../../libs/files/esm/refrecord.hpp"
#include "../world/data.hpp"
#include "logger.hpp"

#include <QDir>
#include <QFileInfo>
#include <cmath>

const float TerrainLodGenerator::CELL_SIZE = 4096.0f;
const float TerrainLodGenerator::HEIGHT_SCALE = 8.0f;
const float TerrainLodGenerator::BASE_HEIGHT = -4096.0f;

void TerrainLodGenerator::heightmapToMesh(const qint8 heightData[33][33],
                                           float baseHeight,
                                           float heightScale,
                                           QVector<Nif::Vector3>& vertices,
                                           QVector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    const int rows = 33;
    const int cols = 33;

    float cellStepX = CELL_SIZE / static_cast<float>(cols - 1);
    float cellStepY = CELL_SIZE / static_cast<float>(rows - 1);

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            float x = static_cast<float>(col) * cellStepX;
            float y = static_cast<float>(row) * cellStepY;
            float h = static_cast<float>(heightData[row][col]) * heightScale + baseHeight;

            vertices.append({x, y, h});
        }
    }

    for (int row = 0; row < rows - 1; ++row)
    {
        for (int col = 0; col < cols - 1; ++col)
        {
            unsigned int topLeft = static_cast<unsigned int>(row * cols + col);
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = static_cast<unsigned int>((row + 1) * cols + col);
            unsigned int bottomRight = bottomLeft + 1;

            indices.append(topLeft);
            indices.append(bottomLeft);
            indices.append(topRight);

            indices.append(topRight);
            indices.append(bottomLeft);
            indices.append(bottomRight);
        }
    }
}

void TerrainLodGenerator::computeNormals(const QVector<Nif::Vector3>& vertices,
                                           const QVector<unsigned int>& indices,
                                           QVector<Nif::Vector3>& normals)
{
    normals.fill({0.0f, 0.0f, 0.0f}, vertices.size());

    for (int i = 0; i < indices.size(); i += 3)
    {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        if (i0 >= static_cast<unsigned int>(vertices.size()) ||
            i1 >= static_cast<unsigned int>(vertices.size()) ||
            i2 >= static_cast<unsigned int>(vertices.size()))
            continue;

        const Nif::Vector3& v0 = vertices[i0];
        const Nif::Vector3& v1 = vertices[i1];
        const Nif::Vector3& v2 = vertices[i2];

        float ex1 = v1.x - v0.x, ey1 = v1.y - v0.y, ez1 = v1.z - v0.z;
        float ex2 = v2.x - v0.x, ey2 = v2.y - v0.y, ez2 = v2.z - v0.z;
        float nx = ey1 * ez2 - ez1 * ey2;
        float ny = ez1 * ex2 - ex1 * ez2;
        float nz = ex1 * ey2 - ey1 * ex2;

        normals[i0].x += nx; normals[i0].y += ny; normals[i0].z += nz;
        normals[i1].x += nx; normals[i1].y += ny; normals[i1].z += nz;
        normals[i2].x += nx; normals[i2].y += ny; normals[i2].z += nz;
    }

    for (auto& n : normals)
    {
        float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len > 1e-8f)
        {
            n.x /= len;
            n.y /= len;
            n.z /= len;
        }
        else
        {
            n = {0.0f, 0.0f, 1.0f};
        }
    }
}

void TerrainLodGenerator::computeUVs(const QVector<Nif::Vector3>& vertices,
                                       float cellMinX, float cellMinY,
                                       float cellWidth, float cellHeight,
                                       QVector<Nif::Vector2>& uvs)
{
    uvs.resize(vertices.size());
    for (int i = 0; i < vertices.size(); ++i)
    {
        float u = (vertices[i].x - cellMinX) / cellWidth;
        float v = (vertices[i].y - cellMinY) / cellHeight;
        uvs[i] = {u, v};
    }
}

QImage TerrainLodGenerator::generateSplatMap(const LandRecord::Color colorData[33][33],
                                               int resolution)
{
    QImage splatMap(resolution, resolution, QImage::Format_RGB32);
    splatMap.fill(Qt::black);

    for (int y = 0; y < resolution; ++y)
    {
        for (int x = 0; x < resolution; ++x)
        {
            float srcX = static_cast<float>(x) / static_cast<float>(resolution) * 32.0f;
            float srcY = static_cast<float>(y) / static_cast<float>(resolution) * 32.0f;

            int ix = qBound(0, static_cast<int>(srcX), 32);
            int iy = qBound(0, static_cast<int>(srcY), 32);

            const LandRecord::Color& c = colorData[iy][ix];
            splatMap.setPixelColor(x, y, QColor(c.r, c.g, c.b, c.a));
        }
    }

    return splatMap;
}

TerrainLodGenerator::TerrainLodResult TerrainLodGenerator::generateTerrainMesh(
    const LandRecord& land,
    const QString& outputPath,
    const TerrainLodOptions& options)
{
    TerrainLodResult result;

    if (!land.hasHeightData)
    {
        result.error = QString("LAND record at (%1,%2) has no height data")
                           .arg(land.cellX).arg(land.cellY);
        return result;
    }

    QVector<Nif::Vector3> vertices;
    QVector<unsigned int> indices;

    float adjustedBaseHeight = BASE_HEIGHT;
    float adjustedHeightScale = HEIGHT_SCALE;

    if (options.lodLevel > 1)
    {
        float lodScale = 1.0f / static_cast<float>(1 << (options.lodLevel - 1));
        adjustedHeightScale *= lodScale;
    }

    heightmapToMesh(land.heightData, adjustedBaseHeight, adjustedHeightScale,
                     vertices, indices);

    float cellMinX = static_cast<float>(land.cellX) * CELL_SIZE;
    float cellMinY = static_cast<float>(land.cellY) * CELL_SIZE;

    QVector<Nif::Vector2> uvs;
    computeUVs(vertices, cellMinX, cellMinY, CELL_SIZE, CELL_SIZE, uvs);

    QVector<Nif::Color4> colors;
    if (land.hasColorData)
    {
        colors.resize(vertices.size());
        const int cols = 33;
        for (int i = 0; i < vertices.size(); ++i)
        {
            int row = i / cols;
            int col = i % cols;
            row = qBound(0, row, 32);
            col = qBound(0, col, 32);
            const LandRecord::Color& c = land.colorData[row][col];
            colors[i] = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
        }
    }
    else
    {
        colors.fill({1.0f, 1.0f, 1.0f, 1.0f}, vertices.size());
    }

    Nif::NifParser nif;
    Nif::Node* root = new Nif::Node();
    root->name = QString("Terrain_%1_%2").arg(land.cellX).arg(land.cellY);
    root->position = {0.0f, 0.0f, 0.0f};
    nif.setRoot(root);

    Nif::TriShape terrainShape;
    terrainShape.name = "TerrainMesh";
    terrainShape.vertices = vertices;
    terrainShape.uvs = uvs;
    terrainShape.colors = colors;
    terrainShape.indices = indices;
    terrainShape.recalculateNormals();

    root->shapes.append(terrainShape);

    if (options.generateTextures && land.hasColorData)
    {
        QImage splatMap = generateSplatMap(land.colorData, options.textureResolution);
        QFileInfo fi(outputPath);
        QString splatPath = fi.path() + "/" + fi.completeBaseName() + "_splat.png";
        splatMap.save(splatPath);
    }

    if (nif.save(outputPath))
    {
        result.success = true;
        result.verticesGenerated = vertices.size();
        result.trianglesGenerated = indices.size() / 3;

        LOG_INFO(QString("Terrain LOD generated: %1 (%2 verts, %3 tris)")
                     .arg(outputPath)
                     .arg(result.verticesGenerated)
                     .arg(result.trianglesGenerated));
    }
    else
    {
        result.error = QString("Failed to save NIF: %1").arg(outputPath);
        LOG_ERROR(result.error);
    }

    return result;
}

TerrainLodGenerator::TerrainLodResult TerrainLodGenerator::generateWorldLod(
    Data& data,
    quint32 worldspaceFormId,
    const QString& outputDir,
    const TerrainLodOptions& options)
{
    TerrainLodResult result;

    QDir dir(outputDir);
    if (!dir.exists())
    {
        if (!dir.mkpath(".")) {
            result.error = "Failed to create output directory: " + outputDir;
            return result;
        }
    }

    const auto& landCollection = data.getLandCollection();
    const auto& refrCollection = data.getRefrCollection();

    int totalVertices = 0;
    int totalTriangles = 0;
    int cellsProcessed = 0;

    const auto& landRecords = landCollection.getRecords();
    for (int i = 0; i < landRecords.size(); ++i)
    {
        if (landRecords[i].isErased() || landRecords[i].isDeleted())
            continue;

        const LandRecord& land = landRecords[i].get();
        if (!land.hasHeightData)
            continue;

        QString cellFileName = QString("%1/cell_%2_%3_lod%4.nif")
                                   .arg(outputDir)
                                   .arg(land.cellX)
                                   .arg(land.cellY)
                                   .arg(options.lodLevel);

        TerrainLodResult cellResult = generateTerrainMesh(land, cellFileName, options);
        if (cellResult.success)
        {
            totalVertices += cellResult.verticesGenerated;
            totalTriangles += cellResult.trianglesGenerated;
            cellsProcessed++;
        }
    }

    const auto& refrRecords = refrCollection.getRecords();
    for (int i = 0; i < refrRecords.size(); ++i)
    {
        if (refrRecords[i].isErased() || refrRecords[i].isDeleted())
            continue;

        const RefrRecord& refr = refrRecords[i].get();

        Nif::NifParser refNif;
        Nif::Node* refRoot = new Nif::Node();
        refRoot->name = QString("REFR_%1").arg(refr.formId, 0, 16);
        refRoot->position = {refr.posX, refr.posY, refr.posZ};
        refRoot->rotation = {refr.rotX, refr.rotY, refr.rotZ};
        refNif.setRoot(refRoot);

        Nif::TriShape refShape;
        refShape.name = "ObjectLOD";
        refShape.vertices = {{-32.0f, -32.0f, 0.0f},
                             {32.0f, -32.0f, 0.0f},
                             {32.0f, 32.0f, 0.0f},
                             {-32.0f, 32.0f, 0.0f}};
        refShape.uvs = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
        refShape.normals = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
                            {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
        refShape.colors = {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
                           {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}};
        refShape.indices = {0, 1, 2, 0, 2, 3};
        refRoot->shapes.append(refShape);

        float scaleFactor = refr.scale > 0.0f ? refr.scale : 1.0f;
        for (auto& v : refShape.vertices)
        {
            v.x *= scaleFactor;
            v.y *= scaleFactor;
            v.z *= scaleFactor;
        }
        refShape.recalculateNormals();

        QString refPath = QString("%1/ref_%2_lod%3.nif")
                              .arg(outputDir)
                              .arg(refr.formId, 0, 16)
                              .arg(options.lodLevel);

        refNif.save(refPath);
    }

    result.success = cellsProcessed > 0;
    result.verticesGenerated = totalVertices;
    result.trianglesGenerated = totalTriangles;

    if (cellsProcessed == 0)
    {
        result.error = "No LAND records with valid height data found";
    }
    else
    {
        LOG_INFO(QString("World LOD generated: %1 cells, %2 vertices, %3 triangles")
                     .arg(cellsProcessed)
                     .arg(totalVertices)
                     .arg(totalTriangles));
    }

    return result;
}

#include "treelodgenerator.hpp"
#include "textureatlasgenerator.hpp"
#include "logger.hpp"
#include "nifparser.hpp"
#include "../world/data.hpp"

#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <algorithm>
#include <cmath>

QImage TreeLodGenerator::generateBillboard(const QString& treeNifPath, int resolution)
{
    Nif::NifParser parser;
    if (!parser.load(treeNifPath))
    {
        LOG_WARNING(QString("TreeLOD: Failed to load NIF: %1").arg(treeNifPath));
        return QImage();
    }

    Nif::Node* root = parser.getRoot();
    if (!root)
    {
        LOG_WARNING(QString("TreeLOD: No root node in: %1").arg(treeNifPath));
        return QImage();
    }

    QVector<Nif::TriShape*> allShapes;
    std::function<void(Nif::Node*)> collectShapes = [&](Nif::Node* node) {
        if (!node) return;
        for (auto& shape : node->shapes)
            allShapes.append(&shape);
        for (auto* child : node->children)
            collectShapes(child);
    };
    collectShapes(root);

    if (allShapes.isEmpty())
    {
        LOG_WARNING(QString("TreeLOD: No shapes in: %1").arg(treeNifPath));
        return QImage();
    }

    Nif::Vector3 boundsMin = {1e30f, 1e30f, 1e30f};
    Nif::Vector3 boundsMax = {-1e30f, -1e30f, -1e30f};

    for (const auto* shapePtr : allShapes)
    {
        for (const auto& v : shapePtr->vertices)
        {
            boundsMin.x = std::min(boundsMin.x, v.x);
            boundsMin.y = std::min(boundsMin.y, v.y);
            boundsMin.z = std::min(boundsMin.z, v.z);
            boundsMax.x = std::max(boundsMax.x, v.x);
            boundsMax.y = std::max(boundsMax.y, v.y);
            boundsMax.z = std::max(boundsMax.z, v.z);
        }
    }

    float sizeX = boundsMax.x - boundsMin.x;
    float sizeZ = boundsMax.z - boundsMin.z;
    float sizeY = boundsMax.y - boundsMin.y;
    float maxSize = std::max({sizeX, sizeY, sizeZ});
    if (maxSize < 1e-6f)
    {
        LOG_WARNING(QString("TreeLOD: Degenerate bounds in: %1").arg(treeNifPath));
        return QImage();
    }

    QImage billboard(resolution, resolution, QImage::Format_ARGB32);
    billboard.fill(Qt::transparent);

    QPainter painter(&billboard);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    float margin = maxSize * 0.05f;
    float worldSize = maxSize + margin * 2.0f;
    float scale = static_cast<float>(resolution) / worldSize;

    float centerX = (boundsMin.x + boundsMax.x) * 0.5f;
    float centerZ = (boundsMin.z + boundsMax.z) * 0.5f;
    float bottomY = boundsMin.y;

    QTransform transform;
    transform.scale(1.0f, -1.0f);
    transform.translate(-static_cast<float>(resolution) * 0.5f,
                        -static_cast<float>(resolution) * 0.5f);

    QTransform scaleTransform;
    scaleTransform.scale(scale, scale);
    scaleTransform.translate(-centerX, -bottomY);

    QPainterPath silhouettePath;

    for (const auto* shapePtr : allShapes)
    {
        const Nif::TriShape& shape = *shapePtr;
        for (int i = 0; i < shape.indices.size(); i += 3)
        {
            int i0 = static_cast<int>(shape.indices[i]);
            int i1 = static_cast<int>(shape.indices[i + 1]);
            int i2 = static_cast<int>(shape.indices[i + 2]);

            if (i0 >= shape.vertices.size() || i1 >= shape.vertices.size() || i2 >= shape.vertices.size())
                continue;

            const Nif::Vector3& v0 = shape.vertices[i0];
            const Nif::Vector3& v1 = shape.vertices[i1];
            const Nif::Vector3& v2 = shape.vertices[i2];

            QPolygonF tri;
            tri.append(QPointF(v0.x, v0.z));
            tri.append(QPointF(v1.x, v1.z));
            tri.append(QPointF(v2.x, v2.z));

            QPolygonF projectedTri;
            for (const QPointF& p : tri)
            {
                QPointF proj = scaleTransform.map(p);
                projectedTri.append(transform.map(proj));
            }

            silhouettePath.moveTo(projectedTri[0]);
            silhouettePath.lineTo(projectedTri[1]);
            silhouettePath.lineTo(projectedTri[2]);
            silhouettePath.closeSubpath();
        }
    }

    painter.setPen(Qt::NoPen);

    QLinearGradient gradient(0, 0, 0, resolution);
    gradient.setColorAt(0.0f, QColor(50, 120, 30, 200));
    gradient.setColorAt(0.5f, QColor(30, 90, 20, 220));
    gradient.setColorAt(1.0f, QColor(20, 60, 15, 180));
    painter.setBrush(gradient);

    painter.drawPath(silhouettePath);

    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    painter.setBrush(QColor(255, 255, 255, 200));
    painter.drawRect(billboard.rect());
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter.end();

    return billboard;
}

TreeLodGenerator::TreeLodResult TreeLodGenerator::generateTreeLodAtlas(
    const Data& data,
    const QString& dataDir,
    const QString& outputDir,
    int atlasResolution)
{
    TreeLodResult result;
    QDir dir(dataDir);
    QDir outDir(outputDir);

    if (!outDir.exists())
    {
        if (!outDir.mkpath("."))
        {
            result.error = QString("Cannot create output directory: %1").arg(outputDir);
            return result;
        }
    }

    const auto& treeCollection = data.getTreeCollection();
    QVector<QString> treeIds;

    for (int row = 0; row < treeCollection.size(); ++row)
    {
        const TreeRecord& record = treeCollection.getRecord(row).get();
        treeIds.append(record.editorId);
    }

    if (treeIds.isEmpty())
    {
        result.error = "No tree records found";
        return result;
    }

    LOG_INFO(QString("TreeLOD: Processing %1 trees").arg(treeIds.size()));

    QVector<QImage> billboardImages;

    for (const QString& treeId : treeIds)
    {
        int treeIndex = -1;
        for (int row = 0; row < treeCollection.size(); ++row)
        {
            const TreeRecord& record = treeCollection.getRecord(row).get();
            if (record.editorId == treeId)
            {
                treeIndex = row;
                break;
            }
        }
        if (treeIndex < 0) continue;

        const TreeRecord& record = treeCollection.getRecord(treeIndex).get();
        QString nifPath = record.modelPath;

        if (!nifPath.isEmpty() && !nifPath.startsWith("/"))
        {
            nifPath = dataDir + "/" + nifPath;
        }

        QImage billboard = generateBillboard(nifPath, 256);
        if (billboard.isNull())
        {
            QImage placeholder(256, 256, QImage::Format_ARGB32);
            placeholder.fill(QColor(100, 150, 80, 128));
            billboardImages.append(placeholder);
        }
        else
        {
            billboardImages.append(billboard);
        }

        result.treesProcessed++;
    }

    TextureAtlasGenerator::AtlasResult atlasResult =
        TextureAtlasGenerator::generateAtlasFromImages(billboardImages, atlasResolution, 2);

    if (!atlasResult.success)
    {
        result.error = QString("Atlas generation failed: %1").arg(atlasResult.error);
        return result;
    }

    result.atlasImage = atlasResult.atlasImage;
    result.treeIds = treeIds;
    result.uvRects.clear();
    for (const auto& entry : atlasResult.entries)
    {
        result.uvRects.append(entry.uvRect);
    }

    QString atlasPath = outDir.filePath("treelod_atlas.png");
    result.atlasImage.save(atlasPath, "PNG");

    LOG_INFO(QString("TreeLOD: Atlas saved to %1 (%2x%3)")
             .arg(atlasPath).arg(atlasResult.atlasWidth).arg(atlasResult.atlasHeight));

    result.success = true;
    return result;
}

bool TreeLodGenerator::createBillboardNif(const QString& treeNifPath,
                                           const QString& billboardTexturePath,
                                           const QString& outputPath)
{
    Nif::NifParser parser;
    Nif::Node* root = new Nif::Node();
    root->name = "BillboardRoot";
    root->position = {0, 0, 0};
    root->rotation = {0, 0, 0};
    root->isBillboardNode = true;

    Nif::TriShape quad;
    quad.name = "BillboardQuad";
    quad.texture = billboardTexturePath;
    quad.alphaMode = Nif::TriShape::AlphaMode::Blend;

    float halfSize = 64.0f;
    {
        QVector<Nif::TriShape> treeShapes;
        Nif::Vector3 treeBoundsMin, treeBoundsMax;
        if (loadTreeModel(treeNifPath, treeShapes, treeBoundsMin, treeBoundsMax)) {
            float sizeX = treeBoundsMax.x - treeBoundsMin.x;
            float sizeZ = treeBoundsMax.z - treeBoundsMin.z;
            float sizeY = treeBoundsMax.y - treeBoundsMin.y;
            float treeMaxSize = std::max({sizeX, sizeY, sizeZ});
            if (treeMaxSize > 1e-6f) {
                halfSize = treeMaxSize * 0.5f;
            }
        }
    }

    quad.vertices.append({-halfSize, 0.0f, 0.0f});
    quad.vertices.append({halfSize, 0.0f, 0.0f});
    quad.vertices.append({halfSize, 0.0f, halfSize * 2.0f});
    quad.vertices.append({-halfSize, 0.0f, halfSize * 2.0f});

    quad.uvs.append({0.0f, 1.0f});
    quad.uvs.append({1.0f, 1.0f});
    quad.uvs.append({1.0f, 0.0f});
    quad.uvs.append({0.0f, 0.0f});

    quad.normals.append({0.0f, -1.0f, 0.0f});
    quad.normals.append({0.0f, -1.0f, 0.0f});
    quad.normals.append({0.0f, -1.0f, 0.0f});
    quad.normals.append({0.0f, -1.0f, 0.0f});

    quad.colors.append({1.0f, 1.0f, 1.0f, 1.0f});
    quad.colors.append({1.0f, 1.0f, 1.0f, 1.0f});
    quad.colors.append({1.0f, 1.0f, 1.0f, 1.0f});
    quad.colors.append({1.0f, 1.0f, 1.0f, 1.0f});

    quad.indices = {0, 1, 2, 0, 2, 3};

    root->shapes.append(quad);
    parser.setRoot(root);

    bool saved = parser.save(outputPath);
    if (!saved)
    {
        LOG_WARNING(QString("TreeLOD: Failed to save billboard NIF: %1").arg(outputPath));
    }

    return saved;
}

bool TreeLodGenerator::loadTreeModel(const QString& nifPath,
                                      QVector<Nif::TriShape>& shapes,
                                      Nif::Vector3& boundsMin,
                                      Nif::Vector3& boundsMax)
{
    Nif::NifParser parser;
    if (!parser.load(nifPath))
        return false;

    Nif::Node* root = parser.getRoot();
    if (!root)
        return false;

    boundsMin = {1e30f, 1e30f, 1e30f};
    boundsMax = {-1e30f, -1e30f, -1e30f};

    std::function<void(Nif::Node*)> collect = [&](Nif::Node* node) {
        if (!node) return;
        for (auto& shape : node->shapes)
        {
            shapes.append(shape);
            for (const auto& v : shape.vertices)
            {
                boundsMin.x = std::min(boundsMin.x, v.x);
                boundsMin.y = std::min(boundsMin.y, v.y);
                boundsMin.z = std::min(boundsMin.z, v.z);
                boundsMax.x = std::max(boundsMax.x, v.x);
                boundsMax.y = std::max(boundsMax.y, v.y);
                boundsMax.z = std::max(boundsMax.z, v.z);
            }
        }
        for (auto* child : node->children)
            collect(child);
    };

    collect(root);
    return !shapes.isEmpty();
}

QImage TreeLodGenerator::renderTreeToImage(const QVector<Nif::TriShape>& shapes,
                                            const Nif::Vector3& boundsMin,
                                            const Nif::Vector3& boundsMax,
                                            int resolution)
{
    QImage image(resolution, resolution, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    float sizeX = boundsMax.x - boundsMin.x;
    float sizeZ = boundsMax.z - boundsMin.z;
    float sizeY = boundsMax.y - boundsMin.y;
    float maxSize = std::max({sizeX, sizeY, sizeZ});

    if (maxSize < 1e-6f)
        return image;

    float margin = maxSize * 0.05f;
    float worldSize = maxSize + margin * 2.0f;
    float scale = static_cast<float>(resolution) / worldSize;

    float centerX = (boundsMin.x + boundsMax.x) * 0.5f;
    float centerZ = (boundsMin.z + boundsMax.z) * 0.5f;
    float bottomY = boundsMin.y;

    QTransform transform;
    transform.scale(1.0f, -1.0f);
    transform.translate(-static_cast<float>(resolution) * 0.5f,
                        -static_cast<float>(resolution) * 0.5f);

    QTransform scaleTransform;
    scaleTransform.scale(scale, scale);
    scaleTransform.translate(-centerX, -bottomY);

    for (const auto& shape : shapes)
    {
        for (int i = 0; i < shape.indices.size(); i += 3)
        {
            int i0 = static_cast<int>(shape.indices[i]);
            int i1 = static_cast<int>(shape.indices[i + 1]);
            int i2 = static_cast<int>(shape.indices[i + 2]);

            if (i0 >= shape.vertices.size() || i1 >= shape.vertices.size() || i2 >= shape.vertices.size())
                continue;

            const Nif::Vector3& v0 = shape.vertices[i0];
            const Nif::Vector3& v1 = shape.vertices[i1];
            const Nif::Vector3& v2 = shape.vertices[i2];

            QPolygonF tri;
            tri.append(QPointF(v0.x, v0.z));
            tri.append(QPointF(v1.x, v1.z));
            tri.append(QPointF(v2.x, v2.z));

            QPolygonF projectedTri;
            for (const QPointF& p : tri)
            {
                QPointF proj = scaleTransform.map(p);
                projectedTri.append(transform.map(proj));
            }

            float normalizedY0 = (v0.y - bottomY) / maxSize;
            float normalizedY1 = (v1.y - bottomY) / maxSize;
            float normalizedY2 = (v2.y - bottomY) / maxSize;
            float avgY = (normalizedY0 + normalizedY1 + normalizedY2) / 3.0f;

            int r = static_cast<int>(30 + avgY * 40);
            int g = static_cast<int>(80 + avgY * 60);
            int b = static_cast<int>(20 + avgY * 30);
            r = std::clamp(r, 0, 255);
            g = std::clamp(g, 0, 255);
            b = std::clamp(b, 0, 255);

            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(r, g, b, 200));
            painter.drawPolygon(projectedTri);
        }
    }

    painter.end();
    return image;
}

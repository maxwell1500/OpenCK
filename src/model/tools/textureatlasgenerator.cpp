#include "textureatlasgenerator.hpp"
#include "logger.hpp"

#include <QPainter>
#include <QDir>
#include <algorithm>

TextureAtlasGenerator::AtlasResult TextureAtlasGenerator::generateAtlas(
    const QVector<QString>& texturePaths,
    int maxAtlasSize,
    int padding)
{
    QVector<QImage> images;
    QVector<QString> paths;

    for (const QString& path : texturePaths)
    {
        QImage img(path);
        if (img.isNull())
        {
            LOG_WARNING(QString("TextureAtlas: Failed to load texture: %1").arg(path));
            continue;
        }
        images.append(img);
        paths.append(path);
    }

    if (images.isEmpty())
    {
        AtlasResult result;
        result.error = "No valid textures loaded";
        return result;
    }

    return generateAtlasFromImages(images, maxAtlasSize, padding);
}

TextureAtlasGenerator::AtlasResult TextureAtlasGenerator::generateAtlasFromImages(
    const QVector<QImage>& images,
    int maxAtlasSize,
    int padding)
{
    AtlasResult result;

    if (images.isEmpty())
    {
        result.error = "No images provided";
        return result;
    }

    int count = images.size();

    QVector<QPair<int, int>> sortedIndices;
    for (int i = 0; i < count; ++i)
    {
        sortedIndices.append({i, images[i].height()});
    }

    std::sort(sortedIndices.begin(), sortedIndices.end(),
              [](const QPair<int, int>& a, const QPair<int, int>& b) {
                  return a.second > b.second;
              });

    QVector<int> sortedOriginalIndices;
    for (const auto& p : sortedIndices)
    {
        sortedOriginalIndices.append(p.first);
    }

    QVector<Shelf> shelves;
    int currentX = 0;
    int currentY = 0;
    int currentShelfHeight = 0;
    int atlasWidth = 0;
    int atlasHeight = 0;

    for (int i = 0; i < count; ++i)
    {
        int originalIndex = sortedOriginalIndices[i];
        const QImage& img = images[originalIndex];

        int w = img.width();
        int h = img.height();

        if (w > maxAtlasSize || h > maxAtlasSize)
        {
            LOG_WARNING(QString("TextureAtlas: Image too large, skipping"));
            continue;
        }

        if (currentX + w > maxAtlasSize)
        {
            currentY += currentShelfHeight + padding;
            currentX = 0;
            currentShelfHeight = 0;

            if (currentY + h > maxAtlasSize)
            {
                LOG_INFO(QString("TextureAtlas: Atlas full at %1x%2")
                         .arg(atlasWidth).arg(atlasHeight));
                break;
            }
        }

        ShelfEntry entry;
        entry.x = currentX;
        entry.y = currentY;
        entry.width = w;
        entry.height = h;
        entry.index = originalIndex;

        bool foundShelf = false;
        for (auto& shelf : shelves)
        {
            if (shelf.y == currentY)
            {
                shelf.entries.append(entry);
                foundShelf = true;
                break;
            }
        }

        if (!foundShelf)
        {
            Shelf newShelf;
            newShelf.y = currentY;
            newShelf.height = h;
            newShelf.entries.append(entry);
            shelves.append(newShelf);
        }

        currentShelfHeight = std::max(currentShelfHeight, h);
        currentX += w + padding;
        atlasWidth = std::max(atlasWidth, currentX);
    }

    atlasHeight = currentY + currentShelfHeight;

    if (atlasWidth == 0 || atlasHeight == 0)
    {
        result.error = "No images placed in atlas";
        return result;
    }

    int powerOfTwoWidth = 1;
    int powerOfTwoHeight = 1;
    while (powerOfTwoWidth < atlasWidth) powerOfTwoWidth <<= 1;
    while (powerOfTwoHeight < atlasHeight) powerOfTwoHeight <<= 1;
    atlasWidth = powerOfTwoWidth;
    atlasHeight = powerOfTwoHeight;

    QImage atlas(atlasWidth, atlasHeight, QImage::Format_ARGB32);
    atlas.fill(Qt::transparent);

    QPainter painter(&atlas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QVector<AtlasEntry> entries;

    for (const auto& shelf : shelves)
    {
        for (const auto& entry : shelf.entries)
        {
            const QImage& srcImg = images[entry.index];
            QRect destRect(entry.x, entry.y, entry.width, entry.height);
            painter.drawImage(destRect, srcImg);

            AtlasEntry atlasEntry;
            atlasEntry.image = srcImg;
            atlasEntry.uvRect = QRectF(
                static_cast<float>(entry.x) / atlasWidth,
                static_cast<float>(entry.y) / atlasHeight,
                static_cast<float>(entry.width) / atlasWidth,
                static_cast<float>(entry.height) / atlasHeight
            );

            entries.append(atlasEntry);
        }
    }

    painter.end();

    result.atlasImage = atlas;
    result.entries = entries;
    result.atlasWidth = atlasWidth;
    result.atlasHeight = atlasHeight;
    result.success = true;

    LOG_INFO(QString("TextureAtlas: Generated %1x%2 atlas with %3 entries")
             .arg(atlasWidth).arg(atlasHeight).arg(entries.size()));

    return result;
}

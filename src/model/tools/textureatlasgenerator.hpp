#ifndef TEXTUREATLASGENERATOR_HPP
#define TEXTUREATLASGENERATOR_HPP

#include <QString>
#include <QImage>
#include <QVector>
#include <QRectF>

class TextureAtlasGenerator
{
public:
    struct AtlasEntry
    {
        QString sourceTexture;
        QRectF uvRect;
        QImage image;
    };

    struct AtlasResult
    {
        QImage atlasImage;
        QVector<AtlasEntry> entries;
        int atlasWidth = 0;
        int atlasHeight = 0;
        bool success = false;
        QString error;
    };

    static AtlasResult generateAtlas(const QVector<QString>& texturePaths,
                                      int maxAtlasSize = 4096,
                                      int padding = 2);

    static AtlasResult generateAtlasFromImages(const QVector<QImage>& images,
                                                int maxAtlasSize = 4096,
                                                int padding = 2);

private:
    struct ShelfEntry
    {
        int x;
        int y;
        int width;
        int height;
        int index;
    };

    struct Shelf
    {
        int y;
        int height;
        QVector<ShelfEntry> entries;
    };
};

#endif // TEXTUREATLASGENERATOR_HPP

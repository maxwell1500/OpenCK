#ifndef TERRAINBLOCK_H
#define TERRAINBLOCK_H

#include <QVector>
#include <QRect>
#include <QString>

// TerrainBlock implements landscape cutting: it extracts a rectangular
// sub-block from a heightmap ("cut") and pastes it back at a target origin
// ("insert"), optionally blending the block's edge into the surrounding
// terrain so the seams are smooth. This is the "terrain blocks + landscape
// cutting" workflow the real landscape editor exposes.
class TerrainBlock
{
public:
    // A cut-out rectangular region of a heightmap.
    struct Block
    {
        QRect rect;             // source rectangle (in grid coordinates)
        int sourceSize = 0;     // side length of the source heightmap
        QVector<float> heights; // row-major [y * width + x]
        int width() const { return rect.width(); }
        int height() const { return rect.height(); }
    };

    // Extracts the rectangle 'rect' (clamped to the grid) from 'heightmap'
    // (mapSize x mapSize). Returns false if the rectangle is empty.
    static bool cut(const QVector<float>& heightmap, int mapSize,
                    const QRect& rect, Block& out);

    // Pastes 'block' into 'heightmap' at 'origin' (top-left), clamping to the
    // grid. If blendWidth > 0 the outer ring of the pasted block is blended
    // toward the existing heights so the seam is smooth.
    static void insert(QVector<float>& heightmap, int mapSize,
                       const Block& block, QPoint origin, int blendWidth = 0);

    // Rounds a rectangle down to the given grid multiple (landscape blocks
    // are usually aligned to a fixed grid like 8 or 16 cells).
    static QRect alignToGrid(const QRect& rect, int grid);
};

#endif // TERRAINBLOCK_H

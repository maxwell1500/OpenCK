#include "terrainblock.hpp"

#include <algorithm>
#include <cmath>

bool TerrainBlock::cut(const QVector<float>& heightmap, int mapSize,
                       const QRect& rect, Block& out)
{
    if (heightmap.isEmpty() || mapSize <= 0)
        return false;

    QRect clamped = rect.intersected(QRect(0, 0, mapSize, mapSize));
    if (clamped.width() <= 0 || clamped.height() <= 0)
        return false;

    out.rect = clamped;
    out.sourceSize = mapSize;
    out.heights.resize(clamped.width() * clamped.height());

    for (int y = clamped.top(); y <= clamped.bottom(); ++y)
    {
        for (int x = clamped.left(); x <= clamped.right(); ++x)
        {
            const int srcIndex = y * mapSize + x;
            const int dstIndex = (y - clamped.top()) * clamped.width()
                + (x - clamped.left());
            out.heights[dstIndex] = heightmap.at(srcIndex);
        }
    }
    return true;
}

void TerrainBlock::insert(QVector<float>& heightmap, int mapSize,
                          const Block& block, QPoint origin, int blendWidth)
{
    if (heightmap.isEmpty() || mapSize <= 0 || block.heights.size() !=
            block.rect.width() * block.rect.height())
        return;

    const int bw = qMax(0, blendWidth);
    for (int y = 0; y < block.height(); ++y)
    {
        for (int x = 0; x < block.width(); ++x)
        {
            const int gx = origin.x() + x;
            const int gy = origin.y() + y;
            if (gx < 0 || gy < 0 || gx >= mapSize || gy >= mapSize)
                continue;

            const int dstIndex = gy * mapSize + gx;
            const int srcIndex = y * block.width() + x;
            const float blockHeight = block.heights.at(srcIndex);

            if (bw <= 0)
            {
                heightmap[dstIndex] = blockHeight;
                continue;
            }

            // Blend factor: 1 in the interior, fading to 0 at the block edge.
            const int dxEdge = qMin(qMin(x, block.width() - 1 - x), bw - 1);
            const int dyEdge = qMin(qMin(y, block.height() - 1 - y), bw - 1);
            const int edgeDist = qMin(dxEdge, dyEdge);
            const float blend = (bw > 0)
                ? qBound(0.0f, static_cast<float>(edgeDist + 1) / bw, 1.0f)
                : 1.0f;

            const float existing = heightmap.at(dstIndex);
            heightmap[dstIndex] = existing + (blockHeight - existing) * blend;
        }
    }
}

QRect TerrainBlock::alignToGrid(const QRect& rect, int grid)
{
    if (grid <= 1)
        return rect;
    QRect out;
    out.setLeft((rect.left() / grid) * grid);
    out.setTop((rect.top() / grid) * grid);
    out.setWidth(((rect.width() + grid - 1) / grid) * grid);
    out.setHeight(((rect.height() + grid - 1) / grid) * grid);
    return out;
}

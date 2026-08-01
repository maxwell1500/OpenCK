#include "autopainter.hpp"

#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

float AutoPainter::slopeAt(const QVector<float>& heightmap, int mapSize,
                           int x, int y, float heightScale)
{
    if (mapSize <= 1)
        return 0.0f;

    const int minX = qMax(0, x - 1);
    const int maxX = qMin(mapSize - 1, x + 1);
    const int minY = qMax(0, y - 1);
    const int maxY = qMin(mapSize - 1, y + 1);

    const float h = heightmap[y * mapSize + x];
    float maxDelta = 0.0f;
    for (int ny = minY; ny <= maxY; ++ny)
    {
        for (int nx = minX; nx <= maxX; ++nx)
        {
            if (nx == x && ny == y)
                continue;
            const float dh = (heightmap[ny * mapSize + nx] - h) * heightScale;
            maxDelta = qMax(maxDelta, std::fabs(dh));
        }
    }

    // A neighbor is one terrain unit away; slope = atan(rise / run).
    return std::atan(maxDelta) * 180.0f / kPi;
}

QVector<int> AutoPainter::paint(const QVector<float>& heightmap,
                                const QVector<AutoPaintLayer>& layers,
                                const Options& options)
{
    QVector<int> result(heightmap.size(), -1);
    if (layers.isEmpty() || heightmap.isEmpty() || options.mapSize <= 0)
        return result;

    // Sort layers by priority (stable, highest last so the final pass wins).
    QVector<int> order(layers.size());
    for (int i = 0; i < order.size(); ++i)
        order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return layers[a].priority < layers[b].priority;
    });

    for (int y = 0; y < options.mapSize; ++y)
    {
        for (int x = 0; x < options.mapSize; ++x)
        {
            const int index = y * options.mapSize + x;
            const float height = heightmap.at(index);
            const float slope = options.useSlope
                ? slopeAt(heightmap, options.mapSize, x, y, options.heightScale)
                : 0.0f;

            int best = -1;
            for (int li : order)
            {
                const AutoPaintLayer& layer = layers[li];
                if (options.useHeight && (height < layer.minHeight || height > layer.maxHeight))
                    continue;
                if (options.useSlope && (slope < layer.minSlope || slope > layer.maxSlope))
                    continue;
                best = li; // later (higher priority) layers override
            }
            result[index] = best;
        }
    }
    return result;
}

QVector<AutoPaintLayer> AutoPainter::defaultLayers()
{
    QVector<AutoPaintLayer> layers;

    AutoPaintLayer low;
    low.texturePath = QStringLiteral("Textures\\Landscape\\Lowland.dds");
    low.minHeight = -100000.0f;
    low.maxHeight = 100.0f;
    low.priority = 0;
    layers.append(low);

    AutoPaintLayer grass;
    grass.texturePath = QStringLiteral("Textures\\Landscape\\Grass.dds");
    grass.minHeight = -50.0f;
    grass.maxHeight = 500.0f;
    grass.maxSlope = 35.0f;
    grass.priority = 1;
    layers.append(grass);

    AutoPaintLayer rock;
    rock.texturePath = QStringLiteral("Textures\\Landscape\\Rock.dds");
    rock.minSlope = 30.0f;
    rock.maxSlope = 90.0f;
    rock.priority = 2;
    layers.append(rock);

    AutoPaintLayer snow;
    snow.texturePath = QStringLiteral("Textures\\Landscape\\Snow.dds");
    snow.minHeight = 1500.0f;
    snow.maxHeight = 100000.0f;
    snow.priority = 3;
    layers.append(snow);

    return layers;
}

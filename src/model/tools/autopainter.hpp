#ifndef AUTOPAINTER_H
#define AUTOPAINTER_H

#include <QVector>
#include <QString>

// AutoPainter assigns texture layers to terrain cells automatically by
// height and slope bands. Given a heightmap, a set of texture layers, and
// per-layer rules (height min/max, slope min/max), it produces a per-cell
// layer index (or -1 when no layer matches) that the editor can apply as
// a paint pass — the "autopaint" behavior the real Creation Kit offers in
// its landscape editor.
struct AutoPaintLayer
{
    QString texturePath;
    float minHeight = -100000.0f;
    float maxHeight = 100000.0f;
    float minSlope = 0.0f;   // degrees
    float maxSlope = 90.0f;  // degrees
    float opacity = 1.0f;
    int priority = 0;        // higher wins when several layers match
};

class AutoPainter
{
public:
    // Options controlling the autopaint pass.
    struct Options
    {
        bool useSlope = true;
        bool useHeight = true;
        int mapSize = 0;            // heightmap side length (square)
        float heightScale = 1.0f;   // heightmap units -> game units
    };

    // Computes the per-cell layer assignment for a square heightmap.
    // heightmap is indexed [y * mapSize + x]; the result has the same size
    // and holds the layer index into 'layers', or -1 when nothing matches.
    static QVector<int> paint(const QVector<float>& heightmap,
                              const QVector<AutoPaintLayer>& layers,
                              const Options& options);

    // Returns the slope in degrees at a cell given its 3x3 neighborhood.
    static float slopeAt(const QVector<float>& heightmap, int mapSize,
                         int x, int y, float heightScale = 1.0f);

    // Returns a default layer set (low/grass/rock/snow by height+slope).
    static QVector<AutoPaintLayer> defaultLayers();
};

#endif // AUTOPAINTER_H

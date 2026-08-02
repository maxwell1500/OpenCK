#ifndef TERRAINOVERLAYMASK_H
#define TERRAINOVERLAYMASK_H

#include <QImage>
#include <QString>
#include <QPoint>

// TerrainOverlayMask loads a terrain overlay mask (the 463 .tif/.dds masks
// the game ships under Source\TGATextures\Terrain\OverlayMasks\) and applies
// it to a landscape texture layer. Overlay masks are single-channel alpha
// stencils that select where a biome/settlement material paints, exactly
// like brush alphas select brush shape — but stretched over the whole
// landscape grid instead of a brush footprint.
class TerrainOverlayMask
{
public:
    TerrainOverlayMask() = default;

    // Loads a DDS (via DdsDecoder) or QImageReader-supported image. The
    // alpha channel (or luminance when absent) becomes the mask.
    bool load(const QString& path);

    // Loads from a QImage directly (tests / built-in masks).
    void setImage(const QImage& image);

    bool isValid() const { return mValid; }
    QString filePath() const { return mPath; }
    int width() const { return mAlpha.width(); }
    int height() const { return mAlpha.height(); }

    // Samples the mask at normalized landscape coordinates (0..1 along each
    // axis). Returns 0.0 when no mask is loaded.
    float sample(float u, float v) const;

    // Resamples the mask to a target grid size (for building a per-cell
    // texture-alpha layer). Returns a [size*size] array of 0..1 values.
    QVector<float> resample(int size) const;

    void clear();

private:
    float sampleBilinear(float u, float v) const;

    QImage mAlpha;
    bool mValid = false;
    QString mPath;
};

#endif // TERRAINOVERLAYMASK_H

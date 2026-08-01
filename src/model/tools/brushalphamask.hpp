#ifndef BRUSHALPHAMASK_H
#define BRUSHALPHAMASK_H

#include <QImage>
#include <QString>

// BrushAlphaMask loads a DDS alpha texture (as shipped in the game's
// Data\Textures\BrushAlphas\) and applies it as a multiplicative stencil
// over a landscape brush footprint. The alpha channel is stretched over
// the brush circle: fully opaque pixels keep the brush strength, fully
// transparent pixels suppress it, and the in-between values taper it.
class BrushAlphaMask
{
public:
    BrushAlphaMask() = default;

    // Loads a DDS/PNG/BMP alpha texture. The alpha channel (or luminance
    // when the texture has no alpha) becomes the mask. Returns false and
    // clears the mask on failure.
    bool load(const QString& path);

    // Loads from a QImage directly (useful for tests / built-in masks).
    void setImage(const QImage& image);

    // True when a mask is loaded.
    bool isValid() const { return mValid; }

    // The file this mask was loaded from (empty for built-in masks).
    QString filePath() const { return mPath; }

    int width() const { return mAlpha.width(); }
    int height() const { return mAlpha.height(); }

    // Samples the mask at normalized brush coordinates (-1..1 along each
    // axis, centered on the brush). Returns 1.0 when no mask is loaded so
    // painting without a mask is unaffected.
    float valueAt(float nx, float ny) const;

    // Clears the mask back to the identity (full strength everywhere).
    void clear();

private:
    float sampleBilinear(float u, float v) const;

    QImage mAlpha;
    bool mValid = false;
    QString mPath;
};

#endif // BRUSHALPHAMASK_H

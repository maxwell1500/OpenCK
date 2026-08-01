#ifndef TEXTURECONVERSIONRULES_H
#define TEXTURECONVERSIONRULES_H

#include <QString>
#include <QVector>
#include <QJsonObject>

// TextureConversionRules parses the xtexconv conversion rule files the real
// Creation Kit ships as `Textures_Settings*.json` (and the in-game
// `texturesettings.json`). Each rule maps a path wildcard to a target pixel
// format, optional mipmap / gamma / distance-field behavior, and an output
// channel layout. OpenCK uses these rules to drive its DDS batch conversion.
struct TextureConversionRule
{
    enum class Format
    {
        BC7,           // DDS DXGI_FORMAT_BC7_UNORM
        BC5,           // DDS DXGI_FORMAT_BC5_UNORM (RG, normal maps)
        BC4,           // DDS DXGI_FORMAT_BC4_UNORM (R, alpha/height)
        R8,            // raw 8-bit single channel
        R8G8B8A8,      // raw RGBA
        R8G8B8A8_UNORM_SRGB, // raw RGBA sRGB
        Unknown
    };

    static QString formatToString(Format f);
    static Format stringToFormat(const QString& text);

    QString pathPattern;      // e.g. "textures/actors/character/*"
    Format format = Format::BC7;
    bool generateMipmaps = true;
    bool srgb = false;                 // gamma-corrected (sRGB) encoding
    bool distanceField = false;        // signed distance field output
    bool physicallyBasedMipmaps = false;
    bool compressUberFallback = false; // force single-format fallback
    int maxTextureSize = 0;            // 0 = no limit

    static TextureConversionRule fromJson(const QJsonObject& obj);
};

class TextureConversionRules
{
public:
    // Loads a conversion rules file. Accepts either a top-level array of
    // rules or an object with "rules" / "textures" keys. Returns false if
    // the file could not be parsed.
    static bool loadFile(const QString& path, QVector<TextureConversionRule>& out);

    // Parses a JSON array/object string.
    static bool parse(const QString& json, QVector<TextureConversionRule>& out);

    // Returns the rule that best matches the given texture path, or nullptr.
    // The first rule whose pattern matches is returned (rules are ordered).
    static const TextureConversionRule* findRule(
        const QVector<TextureConversionRule>& rules, const QString& texturePath);

    // Returns the default built-in rules when no settings file is available
    // (standard Fallout 4 / Starfield conventions).
    static QVector<TextureConversionRule> builtin();
};

#endif // TEXTURECONVERSIONRULES_H

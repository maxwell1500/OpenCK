#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

class AssetConverter
{
public:
    struct ConversionResult
    {
        bool success = false;
        QString error;
        int filesConverted = 0;
    };

    // NIF -> OBJ export
    static ConversionResult nifToObj(const QString& nifPath, const QString& objPath);

    // OBJ -> NIF import (basic)
    static ConversionResult objToNif(const QString& objPath, const QString& nifPath);

    // Texture batch conversion (DDS, TGA, PNG)
    // targetFormat: "dds", "tga", "png"
    static ConversionResult convertTextures(const QStringList& inputPaths,
                                           const QString& outputDir,
                                           const QString& targetFormat);

    // Rule-aware texture conversion: converts every input to DDS, choosing
    // the block format (DXT1/DXT5) from the matching xtexconv-style rule.
    // If rulesPath is empty the built-in rules are used.
    static ConversionResult convertTexturesByRules(const QStringList& inputPaths,
                                                   const QString& outputDir,
                                                   const QString& rulesPath = QString());

    // Sound batch conversion (WAV -> OGG)
    static ConversionResult convertSounds(const QStringList& inputPaths,
                                         const QString& outputDir);
};

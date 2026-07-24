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

    // Sound batch conversion (WAV -> OGG)
    static ConversionResult convertSounds(const QStringList& inputPaths,
                                         const QString& outputDir);
};

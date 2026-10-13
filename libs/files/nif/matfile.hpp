#pragma once

#include <QMap>
#include <QString>

// Readers for Starfield `.mat` material files. A .mat is a JSON dump of the
// material's component database sub-graph: objects named "<Family>_Material1",
// "<Family>_TextureSet1" and so on, where the TextureSet object is the one that
// lists the actual texture file names.
//
// Two sources exist. A LOOSE .mat (any file a mod ships under materials\) is
// self-contained, so its texture set is readable directly. Base-game materials
// are not shipped loose either, but the Creation Kit redistributes them as
// JSON in Tools\ContentResources.zip, so set the path with
// setContentResources() and the same reader covers the base game too. Anything
// found in neither place is reported as absent, which keeps the caller's
// proxy-geometry fallback honest.
class MatFile
{
public:
    // Slot index -> texture file path (as written in the .mat, e.g.
    // "Data\\Textures\\...\\foo_color.dds"). Returns an empty map when the file
    // cannot be read or carries no textures. Slot meanings come from the
    // Creation Kit's EditorFiles/RuleTemplates/ShaderModels: 0 colour,
    // 1 normal, 3 roughness, 4 metalness, 5 ambient occlusion.
    static QMap<int, QString> textureSlots(const QString& absPath);

    // Finds the loose file backing a material path taken from a NIF (e.g.
    // "Materials\\Common\\Metal\\MetalAluminum01.mat"). Tries the path as given
    // under dataDir, then any file with the same base name under dataDir's
    // materials\ tree, and finally the Creation Kit's ContentResources.zip set
    // with setContentResources(). Returns an empty string when no source
    // holds it.
    static QString findLoose(const QString& materialPath, const QString& dataDir);

    // Points the reader at a Creation Kit install's Tools\ContentResources.zip.
    // Opened lazily on first use, so passing a path that does not exist is
    // fine. Pass an empty path to forget a previously set one.
    static void setContentResources(const QString& zipPath);

    // Convenience: colour-map path for a material referenced by a NIF, or an
    // empty string.
    static QString colorTexture(const QString& materialPath, const QString& dataDir);
};

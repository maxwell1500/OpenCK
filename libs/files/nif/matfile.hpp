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
// What a texture-set slot holds. The Creation Kit's own channel vocabulary is
// the semantic names in EditorFiles/RuleTemplates/ShaderModels ("Albedo",
// "Normal", "Roughness", ...); the mapping to the numeric slot was derived by
// histogramming the channel suffix of all 216,208 texture-file entries in the
// Creation Kit's ContentResources.zip, so it is measured rather than assumed:
//
//   0 Albedo           41,033   (n=53,496; the rest are numbered albedo
//                               variants such as "_1", "_2" and "_white")
//   1 Normal           46,245
//   2 Opacity          25,009
//   3 Roughness        41,947
//   4 Metalness         7,984
//   5 AmbientOcclusion 10,382
//   6 Height              996
//   7 Emissive          3,854   and 879 albedo maps: unlit/emissive-only
//                               materials carry their colour here instead of
//                               in slot 0
//   8 Transmissive     18,949
//   9 Curvature           517
//  12 DepthOffset          11
//  15 OverlayRoughness    74    (13/14/15 are the overlay and scattering group)
//  17 ScatteringDirections 42
//  18 ScatteringForwardAlpha 42
//  19 SecondaryOpacity    823
//  20 HairID              12
//
// Colour is only slot 0. Falling back to "whichever slot is populated" would
// be wrong for the 2,352 texture sets whose only entry is a normal (slot 1),
// opacity (2) or roughness (3) map: they would be drawn in the colour slot.
// The emissive materials above need the shader model to tell them apart, and
// that lives only in the component database, so they stay untextured.
enum MatChannel
{
    MatChannel_Albedo = 0,
    MatChannel_Normal = 1,
    MatChannel_Opacity = 2,
    MatChannel_Roughness = 3,
    MatChannel_Metalness = 4,
    MatChannel_AmbientOcclusion = 5,
    MatChannel_Height = 6,
    MatChannel_Emissive = 7,
    MatChannel_Transmissive = 8,
};

class MatFile
{
public:
    // Slot index -> texture file path (as written in the .mat, e.g.
    // "Data\\Textures\\...\\foo_color.dds"). Returns an empty map when the file
    // cannot be read or carries no textures. See MatChannel for slot meanings;
    // colour is slot 0 (MatChannel_Albedo).
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

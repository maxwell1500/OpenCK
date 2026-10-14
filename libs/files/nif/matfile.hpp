#pragma once

#include <QMap>
#include <QString>
#include <QColor>

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
// Colour is slot 0. Falling back to "whichever slot is populated" would
// be wrong for the 2,352 texture sets whose only entry is a normal (slot 1),
// opacity (2) or roughness (3) map: they would be drawn in the colour slot.
// The one exception is slot 7 when its file is itself a "_color" map, which is
// how unlit effect materials such as GlowDecal_* carry their colour; a slot 7
// "_emissive" file is a glow mask and is not colour.
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

    // Points the reader at a compiled materialsbeta.cdb (extracted from
    // Starfield - Materials.ba2). The base game's material objects live there
    // rather than in loose files, so this is what covers materials that ship in
    // neither the data dir nor the Creation Kit's ContentResources.zip. Opened
    // and indexed lazily on first use.
    static void setMaterialDatabase(const QString& cdbPath);

    // Convenience: colour-map path for a material referenced by a NIF, or an
    // empty string. Follows the material's layer graph (see MatFile::baseTint
    // for the tint half of it).
    static QString colorTexture(const QString& materialPath, const QString& dataDir);

    // Colour multiplier for a material's albedo (or its flat colour when it has
    // no albedo), or an invalid colour when the material is untinted. Painted
    // base-game materials carry their paint colour only here: their base layer
    // has a tint and no colour map.
    static QColor baseTint(const QString& materialPath, const QString& dataDir);

    // Base layer's alpha mask (slot 2, Format "MatChannel_Opacity"), or an empty
    // string. Alpha-only decals have this and no colour map, so without it they
    // would be drawn as opaque blocks over the surface they stamp onto.
    static QString opacityTexture(const QString& materialPath, const QString& dataDir);
};

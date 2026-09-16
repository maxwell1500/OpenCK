#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QByteArray>
#include <QVector>
#include <QtGlobal>

#include "../nif/nifparser.hpp"

class QIODevice;

struct NiPoint3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    static NiPoint3 read(QIODevice& device);
    void write(QIODevice& device) const;
};

struct NiPoint2 {
    float u = 0.0f, v = 0.0f;
    static NiPoint2 read(QIODevice& device);
    void write(QIODevice& device) const;
};

struct NiPoint4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    static NiPoint4 read(QIODevice& device);
    void write(QIODevice& device) const;
};

struct NiColorRGBA {
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
    static NiColorRGBA read(QIODevice& device);
    void write(QIODevice& device) const;
};

struct NiQuat {
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f;
    static NiQuat read(QIODevice& device);
    void write(QIODevice& device) const;
};

enum class AlphaType : quint32 {
    NoAlpha = 0,
    AlphaTest = 1,
    AlphaBlend = 2,
    AlphaAdditive = 3,
    AlphaMultiply = 4,
    AlphaMask = 5,
};

class NifObject {
public:
    QString className;
    quint32 dataRef = 0;
    QByteArray padding;

    static quint32 readRef(QIODevice& device);
    void writeRef(QIODevice& device, quint32 ref) const;

    virtual void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader);
    virtual void write(QIODevice& device, quint32 version) const;
    virtual ~NifObject() = default;
};

class NifAVObject : public NifObject {
public:
    QString name;
    quint32 refParent = 0;
    quint32 refChildren = 0;
    quint32 refExtraData = 0;
    quint32 refControllers = 0;
    QList<quint32> children;
    QList<quint32> controllers;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
    void parseChildren(QIODevice& device, quint32 version);
    void writeChildren(QIODevice& device, quint32 version) const;
};

class NifNode : public NifAVObject {
public:
    bool keepHierarchy = true;
    bool hasRotations = true;
    bool hasTranslations = true;
    bool hasScales = true;
    bool hasScaleKeys = false;
    bool hasBSPNode = false;
    bool hasBSPGeometry = true;
    bool hasBoundingSphere = true;
    float boundingSphereRadius = 0.0f;
    NiPoint3 boundingSphereCenter;
    bool hasBoundingCylinder = false;
    float boundingCylinderRadius = 0.0f;
    float boundingCylinderLength = 0.0f;
    NiPoint3 boundingCylinderAxis;
    float boundingCylinderHeight = 0.0f;
    float collisionLevel = 0.0f;
    // Gamebryo-only: BSFaceGenNiNode roots carry 2 trailing bytes of
    // unknown purpose (0x0001 in all samples). Never written by the
    // dialect save path.
    quint16 faceGenTail = 0;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifTriBasedGeom : public NifAVObject {
public:
    QString geometryDataName;
    quint32 refGeometryData = 0;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifTriShape : public NifTriBasedGeom {
public:
    bool hasAlpha = true;
    bool hasMaterial = true;
    bool hasTexture = true;
    bool hasNormals = true;
    bool hasUVs = true;
    bool hasTangents = true;
    bool hasVertexColors = false;
    // External .mesh streams referenced by Starfield BSGeometry slots
    // (meshes live in BA2s). Filled by the Gamebryo reader; resolved to
    // vertex data by Nif::MeshArchiveResolver.
    QStringList externalMeshPaths;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifTriShapeData : public NifObject {
public:
    QList<NiPoint3> vertices;
    QList<NiPoint2> uvs;
    QList<NiPoint3> normals;
    QList<NiPoint4> tangents;
    QList<NiColorRGBA> vertexColors;
    QList<quint32> indices;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
    void parseVertices(QIODevice& device, quint32 version);
    void parseUVs(QIODevice& device, quint32 version);
    void parseNormals(QIODevice& device, quint32 version);
    void parseTangents(QIODevice& device, quint32 version);
    void parseVertexColors(QIODevice& device, quint32 version);
    void parseIndices(QIODevice& device, quint32 version);
};

class NifTexture : public NifObject {
public:
    QString textureName;
    quint32 refTextureData = 0;
    QString textureFile;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifMaterial : public NifObject {
public:
    NiColorRGBA ambientColor;
    NiColorRGBA diffuseColor;
    NiColorRGBA specularColor;
    float specularExponent = 0.0f;
    float emission = 0.0f;
    float reflectivity = 0.0f;
    float opacity = 1.0f;
    bool hasDiffuseTexture = false;
    bool hasSpecularTexture = false;
    bool hasBumpTexture = false;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifAlphaProperty : public NifObject {
public:
    AlphaType alphaType = AlphaType::NoAlpha;
    float alphaThreshold = 0.0f;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifBinaryExtraData : public NifObject {
public:
    QString extraDataName;
    QByteArray data;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifStringExtraData : public NifObject {
public:
    QString extraDataName;
    QString data;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifKeyframeController : public NifObject {
public:
    quint32 targetNode = 0;
    quint16 flags = 0;
    float frequency = 1.0f;
    float phase = 0.0f;
    float startTime = 0.0f;
    float stopTime = 0.0f;
    quint32 nextController = 0;
    quint32 keyframeDataRef = 0;
    QString clipName;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifControllerManager : public NifObject {
public:
    QList<quint32> controllerRefs;
    QList<QString> clipNames;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifKeyframeData : public NifObject {
public:
    quint32 numKeys = 0;
    quint32 interpolationType = 0;
    QVector<Nif::TransformKeyframe> keyframes;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifTransformData : public NifObject {
public:
    quint32 numTranslationKeys = 0;
    QVector<Nif::Vector3Keyframe> translateKeys;
    quint32 numRotationKeys = 0;
    QVector<Nif::QuaternionKeyframe> rotateKeys;
    quint32 numScaleKeys = 0;
    QVector<Nif::Vector3Keyframe> scaleKeys;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

// Per-vertex skinning blocks (REMAINING.md §8.1). Dialect-native layout
// (NOT the Gamebryo binary encoding): the instance names the skinned shape
// block plus the bone node blocks and the data block; the data block stores
// one bind-pose matrix and a sparse weight list per bone. NiSkinPartition
// data is intentionally not stored — the CPU blend reads SkinData only.
class NifSkinInstance : public NifObject {
public:
    quint32 refTargetShape = 0;   // dataRef of the skinned NiTriShape block
    quint32 refSkeletonRoot = 0;  // dataRef of the skeleton root NiNode (0 = none)
    QList<quint32> bones;         // dataRefs of the bone NiNode blocks
    quint32 refSkinData = 0;      // dataRef of the NiSkinData block

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifSkinData : public NifObject {
public:
    struct BoneWeights {
        float bindPose[16];  // row-major rest world matrix of the bone
        struct Influence {
            quint32 vertex = 0;  // shape-local vertex index
            float weight = 0.0f;
        };
        QList<Influence> weights;
    };
    QList<BoneWeights> bones;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

// Starfield BSSkin blocks (real Gamebryo binary encoding, FaceGen era).
// Grammar validated structurally across shipped face NIFs (15 triplets in
// 2 files: counts/strides consume exactly, refs resolve, attach name
// counts match instance bone counts):
//   SkinAttach: u32 unk(4), u32 nameCount, SizedStrings (bone names).
//   BSSkin::Instance: u32 target, u32 boneDataRef, u32 boneCount,
//     i32 headerFlag(-1), then 16 raw bytes per bone.
//   BSSkin::BoneData: u32 boneCount, then per bone a 4x4 float matrix +
//     float scale (~1.0).
// The Instance per-bone 16B payload semantics are still unknown (only 5
// distinct quads across 76 bones: three permutations of (0,1,1,*),
// all -1, and one (-1,-1,-1,76)) -> preserved raw, never interpreted.
// No parse()/write() overrides: these exist only in Gamebryo binaries and
// are decoded by the Gamebryo reader in nifparser.cpp.
class NifBSSkinInstance : public NifObject {
public:
    quint32 refTarget = 0;
    quint32 refBoneData = 0;
    qint32 headerFlag = -1;
    QVector<QByteArray> bonePayloads;   // 16 raw bytes per bone
};

class NifBSSkinBoneData : public NifObject {
public:
    struct Bone {
        float matrix[16];       // inverse-bind matrix (row-major presumed)
        float scale = 1.0f;     // ~1.0 in all samples
    };
    QVector<Bone> bones;
};

class NifSkinAttach : public NifObject {
public:
    quint32 unknown = 0;        // 4 in all samples
    QStringList boneNames;
};

struct ParticleSystemSettings {
    quint32 numParticles = 0;
    float numVisibleParticles = 0.0f;
    quint32 emitterType = 0;
    quint32 rendererType = 0;
    float emissionRate = 0.0f;
    float lifetime = 0.0f;
    float lifetimeRandom = 0.0f;
    float speed = 0.0f;
    float speedRandom = 0.0f;
    float spread = 0.0f;
    float gravityStrength = 0.0f;
    float startSize = 0.0f;
    float startSizeRandom = 0.0f;
    float endSize = 0.0f;
    float endSizeRandom = 0.0f;
    NiPoint3 emitterPosition = {0, 0, 0};
    NiPoint3 emitterDirection = {0, 0, 1};
    float emitRadius = 0.0f;
    float emitAngle = 0.0f;
    QString texturePath;
    quint32 numColumns = 1;
    quint32 numRows = 1;
    bool additiveBlending = false;
    bool alphaTest = false;
    float startColorR = 1.0f, startColorG = 1.0f, startColorB = 1.0f, startColorA = 1.0f;
    float endColorR = 1.0f, endColorG = 1.0f, endColorB = 1.0f, endColorA = 0.0f;
};

class NifParticleSystem : public NifObject {
public:
    ParticleSystemSettings settings;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifPSysEmitter : public NifObject {
public:
    quint32 emitterType = 0;
    NiPoint3 center = {0, 0, 0};
    NiPoint3 extents = {1, 1, 1};
    float radius = 1.0f;
    float angle = 3.14159f;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifPSysModifier : public NifObject {
public:
    enum Type { Gravity, Drag, Color, Rotation, GrowFade, Vortex, Bomb };
    Type type = Gravity;
    quint32 target = 0;
    float strength = 0.0f;
    float damping = 0.0f;
    float minColorR = 0, minColorG = 0, minColorB = 0, minColorA = 0;
    float maxColorR = 0, maxColorG = 0, maxColorB = 0, maxColorA = 0;
    float minAngle = 0, maxAngle = 0;
    float fadeInTime = 0, fadeOutTime = 0;
    float growTime = 0, shrinkTime = 0;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifLODNode : public NifNode {
public:
    quint32 numLODLevels = 0;
    struct LodLevel {
        float minScreenSize = 0.0f;
        float maxScreenSize = 0.0f;
    };
    QList<LodLevel> lodLevels;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifBillboardNode : public NifNode {
public:
    quint32 mode = 0;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

class NifBSLightingShaderProperty : public NifObject {
public:
    quint32 shaderType = 0;
    quint32 shaderFlags1 = 0;
    quint32 shaderFlags2 = 0;
    float glossiness = 1.0f;
    float specularR = 1.0f, specularG = 1.0f, specularB = 1.0f;
    float alpha = 1.0f;

    void parse(QIODevice& device, quint32 version, const QByteArray& fileHeader) override;
    void write(QIODevice& device, quint32 version) const override;
};

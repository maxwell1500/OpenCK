#ifndef NIFPARSER_HPP
#define NIFPARSER_HPP

#include <QString>
#include <QVector>
#include <QMap>
#include <QByteArray>

namespace Nif {

struct Vector2 {
    float u, v;
};

struct Vector3 {
    float x, y, z;
};

struct Color4 {
    float r, g, b, a;
};

struct Color3 {
    float r, g, b;
};

// --- Animation data structures ---

struct Vector3Keyframe {
    float time;
    Vector3 value;
};

struct QuaternionKeyframe {
    float time;
    float w, x, y, z;
};

struct TransformKeyframe {
    float time;
    Vector3 translation;
    QuaternionKeyframe rotation;
    Vector3 scale;
};

struct AnimationClip {
    QString name;
    float startTime = 0.0f;
    float endTime = 0.0f;
    QVector<TransformKeyframe> keyframes; // node transforms over time
};

struct NiKeyframeController {
    quint32 targetNode = 0; // block ref of target node
    QVector<TransformKeyframe> keyframes;
    QString clipName;
};

struct CollisionShape {
    enum Type { Sphere, Box, Capsule };
    Type type = Sphere;
    Vector3 center = {0, 0, 0};
    Vector3 extents = {1, 1, 1};
    float radius = 1.0f;
};

struct TriShape {
    QString name;
    QVector<Vector3> vertices;
    QVector<Vector2> uvs;
    QVector<Vector3> normals;
    QVector<Color4> colors;
    QVector<unsigned int> indices;

    // Material / texture
    Color4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Color3 specularColor = {0.0f, 0.0f, 0.0f};
    float specularExponent = 32.0f;
    Color3 emissionColor = {0.0f, 0.0f, 0.0f};
    QString texture;
    float opacity = 1.0f;
    enum class AlphaMode { None, Blend, Additive, Multiply };
    AlphaMode alphaMode = AlphaMode::None;

    // --- Mesh editing primitives ---
    void translate(float dx, float dy, float dz);
    void scale(float factor);
    void setVertex(int index, const Vector3& v);
    Vector3 getVertex(int index) const;
    void setUV(int index, const Vector2& uv);
    Vector2 getUV(int index) const;
    void addVertex(const Vector3& v, const Vector2& uv, const Color4& c);
    void removeVertex(int index);
    void recalculateNormals();
    void extrudeFace(int faceIndex, float distance);
    void bevelFace(int faceIndex, float amount);
};

struct NifParticleSystemSettings {
    float emissionRate = 20.0f;
    float lifetime = 1.0f;
    float lifetimeRandom = 0.0f;
    float speed = 1.0f;
    float speedRandom = 0.0f;
    float spread = 0.0f;
    float gravityStrength = 0.0f;
    float startSize = 1.0f;
    float startSizeRandom = 0.0f;
    float endSize = 0.0f;
    float endSizeRandom = 0.0f;
    float emitRadius = 0.0f;
    float emitAngle = 3.14159f;
    Vector3 emitterPosition = {0, 0, 0};
    Vector3 emitterDirection = {0, 0, 1};
    QString texturePath;
    quint32 numColumns = 1;
    quint32 numRows = 1;
    quint32 rendererType = 0;
    bool additiveBlending = false;
    bool alphaTest = false;
    float startColorR = 1.0f, startColorG = 1.0f, startColorB = 1.0f, startColorA = 1.0f;
    float endColorR = 1.0f, endColorG = 1.0f, endColorB = 1.0f, endColorA = 0.0f;
};

struct Node {
    QString name;
    QVector<Node*> children;
    QVector<TriShape> shapes;
    QVector<CollisionShape> collisionShapes;
    QVector<NifParticleSystemSettings> particleSystems;
    Vector3 position = {0, 0, 0};
    Vector3 rotation = {0, 0, 0};

    // Animation data
    QVector<NiKeyframeController> animations;
    bool hasAnimation = false;

    // LOD data
    bool isLODNode = false;
    float lodMinScreenSize = 0.0f;
    float lodMaxScreenSize = 1e30f;
    QVector<float> lodMinScreens;
    QVector<float> lodMaxScreens;

    // Billboard data
    bool isBillboardNode = false;
    quint32 billboardMode = 0;

    TransformKeyframe getInterpolatedFrame(float time) const;

    ~Node() {
        for (auto child : children) {
            delete child;
        }
        children.clear();
    }
};

class NifParser {
public:
    ~NifParser();

    bool load(const QString& fileName);
    bool save(const QString& fileName) const;

    Node* getRoot() const { return root; }
    void setRoot(Node* r) { root = r; }

    quint32 getVersion() const { return m_version; }
    void setVersion(quint32 v) { m_version = v; }
    QString getVersionString() const;

    // --- Whole-tree editing helpers ---
    void translateAll(float dx, float dy, float dz);
    void scaleAll(float factor);
    int totalVertexCount() const;
    int shapeCount() const;

    // --- Animation metadata ---
    float getAnimationDuration() const;
    int getAnimationClipCount() const;
    QString getAnimationClipName(int index) const;
    QVector<NiKeyframeController> getAllAnimationControllers() const;
    QVector<AnimationClip> getAnimClips() const;

private:
    bool parseHeader(QDataStream& stream);
    bool writeHeader(QDataStream& stream, const QString& fileName) const;
    void writeShape(QDataStream& stream, const TriShape& shape) const;
    void writeNodeTree(QDataStream& stream, const Node* node) const;
    bool readShape(QDataStream& stream, TriShape& shape);

    Node* root = nullptr;
    quint32 m_version = 0;
};

} // namespace Nif

#endif // NIFPARSER_HPP

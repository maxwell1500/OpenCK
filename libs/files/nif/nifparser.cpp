#include "nifparser.hpp"

#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <cstdio>
#include <QBuffer>
#include <QDebug>
#include <cmath>
#include <QMap>
#include <QStack>
#include <functional>

#include "logger.hpp"
#include "nifrecord.hpp"
#include "../ba2/ba2archive.hpp"

namespace Nif {

// ---------------------------------------------------------------------------
// TriShape editing primitives
// ---------------------------------------------------------------------------

void TriShape::translate(float dx, float dy, float dz)
{
    for (auto& v : vertices) {
        v.x += dx;
        v.y += dy;
        v.z += dz;
    }
}

void TriShape::scale(float factor)
{
    if (factor == 0.0f) return;
    for (auto& v : vertices) {
        v.x *= factor;
        v.y *= factor;
        v.z *= factor;
    }
}

void TriShape::setVertex(int index, const Vector3& v)
{
    if (index >= 0 && index < vertices.size()) {
        vertices[index] = v;
    }
}

Vector3 TriShape::getVertex(int index) const
{
    if (index >= 0 && index < vertices.size()) {
        return vertices[index];
    }
    return {0.0f, 0.0f, 0.0f};
}

void TriShape::setUV(int index, const Vector2& uv)
{
    if (index >= 0 && index < uvs.size()) {
        uvs[index] = uv;
    }
}

Vector2 TriShape::getUV(int index) const
{
    if (index >= 0 && index < uvs.size()) {
        return uvs[index];
    }
    return {0.0f, 0.0f};
}

void TriShape::addVertex(const Vector3& v, const Vector2& uv, const Color4& c)
{
    vertices.append(v);
    uvs.append(uv);
    normals.append(Vector3{0.0f, 1.0f, 0.0f});
    colors.append(c);
}

void TriShape::removeVertex(int index)
{
    if (index < 0 || index >= vertices.size()) return;
    vertices.removeAt(index);
    if (index < uvs.size()) uvs.removeAt(index);
    if (index < normals.size()) normals.removeAt(index);
    if (index < colors.size()) colors.removeAt(index);
    // Remove any indices referencing this vertex and shift higher indices down.
    for (int i = 0; i < indices.size();) {
        if (static_cast<int>(indices[i]) == index) {
            indices.removeAt(i);
        } else {
            if (static_cast<int>(indices[i]) > index) {
                indices[i] -= 1;
            }
            ++i;
        }
    }
}

void TriShape::recalculateNormals()
{
    if (vertices.isEmpty() || indices.isEmpty()) return;

    // Clear existing normals
    normals.clear();
    normals.resize(vertices.size(), Vector3{0.0f, 0.0f, 0.0f});

    // Calculate face normals from indexed triangles
    for (int i = 0; i < indices.size(); i += 3) {
        if (i + 2 >= indices.size()) break;

        const int i0 = indices[i];
        const int i1 = indices[i + 1];
        const int i2 = indices[i + 2];

        if (i0 < 0 || i0 >= vertices.size() ||
            i1 < 0 || i1 >= vertices.size() ||
            i2 < 0 || i2 >= vertices.size()) continue;

        // Two edge vectors
        const float e1x = vertices[i1].x - vertices[i0].x;
        const float e1y = vertices[i1].y - vertices[i0].y;
        const float e1z = vertices[i1].z - vertices[i0].z;

        const float e2x = vertices[i2].x - vertices[i0].x;
        const float e2y = vertices[i2].y - vertices[i0].y;
        const float e2z = vertices[i2].z - vertices[i0].z;

        // Cross product
        const float nx = e1y * e2z - e1z * e2y;
        const float ny = e1z * e2x - e1x * e2z;
        const float nz = e1x * e2y - e1y * e2x;

        // Accumulate normals for each vertex
        normals[i0].x += nx;
        normals[i0].y += ny;
        normals[i0].z += nz;

        normals[i1].x += nx;
        normals[i1].y += ny;
        normals[i1].z += nz;

        normals[i2].x += nx;
        normals[i2].y += ny;
        normals[i2].z += nz;
    }

    // Normalize all normals
    for (int i = 0; i < normals.size(); ++i) {
        const float len = sqrtf(normals[i].x * normals[i].x +
                                normals[i].y * normals[i].y +
                                normals[i].z * normals[i].z);
        if (len > 0.0001f) {
            normals[i].x /= len;
            normals[i].y /= len;
            normals[i].z /= len;
        } else {
            normals[i] = Vector3{0.0f, 1.0f, 0.0f};
        }
    }
}

void TriShape::extrudeFace(int faceIndex, float distance)
{
    const int start = faceIndex * 3;
    if (start + 2 >= indices.size()) return;

    const int i0 = static_cast<int>(indices[start]);
    const int i1 = static_cast<int>(indices[start + 1]);
    const int i2 = static_cast<int>(indices[start + 2]);

    if (i0 < 0 || i0 >= vertices.size() ||
        i1 < 0 || i1 >= vertices.size() ||
        i2 < 0 || i2 >= vertices.size()) return;

    const Vector3& v0 = vertices[i0];
    const Vector3& v1 = vertices[i1];
    const Vector3& v2 = vertices[i2];

    // Compute face normal via cross product of two edges
    const float e1x = v1.x - v0.x;
    const float e1y = v1.y - v0.y;
    const float e1z = v1.z - v0.z;
    const float e2x = v2.x - v0.x;
    const float e2y = v2.y - v0.y;
    const float e2z = v2.z - v0.z;

    float nx = e1y * e2z - e1z * e2y;
    float ny = e1z * e2x - e1x * e2z;
    float nz = e1x * e2y - e1y * e2x;
    const float len = sqrtf(nx * nx + ny * ny + nz * nz);
    if (len < 0.0001f) return;
    nx /= len;
    ny /= len;
    nz /= len;

    // Create 3 new vertices offset along the face normal
    const Vector3 n0 = {v0.x + nx * distance, v0.y + ny * distance, v0.z + nz * distance};
    const Vector3 n1 = {v1.x + nx * distance, v1.y + ny * distance, v1.z + nz * distance};
    const Vector3 n2 = {v2.x + nx * distance, v2.y + ny * distance, v2.z + nz * distance};

    const Vector2 uv0 = (i0 < uvs.size()) ? uvs[i0] : Vector2{0.0f, 0.0f};
    const Vector2 uv1 = (i1 < uvs.size()) ? uvs[i1] : Vector2{0.0f, 0.0f};
    const Vector2 uv2 = (i2 < uvs.size()) ? uvs[i2] : Vector2{0.0f, 0.0f};
    const Color4 col = (i0 < colors.size()) ? colors[i0] : Color4{1.0f, 1.0f, 1.0f, 1.0f};

    const int newBase = vertices.size();
    addVertex(n0, uv0, col);
    addVertex(n1, uv1, col);
    addVertex(n2, uv2, col);

    // Add 6 side triangles (2 per edge quad)
    // Side along edge v0-v1
    indices.append(static_cast<unsigned int>(i0));
    indices.append(static_cast<unsigned int>(i1));
    indices.append(static_cast<unsigned int>(newBase + 1));
    indices.append(static_cast<unsigned int>(i0));
    indices.append(static_cast<unsigned int>(newBase + 1));
    indices.append(static_cast<unsigned int>(newBase));
    // Side along edge v1-v2
    indices.append(static_cast<unsigned int>(i1));
    indices.append(static_cast<unsigned int>(i2));
    indices.append(static_cast<unsigned int>(newBase + 2));
    indices.append(static_cast<unsigned int>(i1));
    indices.append(static_cast<unsigned int>(newBase + 2));
    indices.append(static_cast<unsigned int>(newBase + 1));
    // Side along edge v2-v0
    indices.append(static_cast<unsigned int>(i2));
    indices.append(static_cast<unsigned int>(i0));
    indices.append(static_cast<unsigned int>(newBase));
    indices.append(static_cast<unsigned int>(i2));
    indices.append(static_cast<unsigned int>(newBase));
    indices.append(static_cast<unsigned int>(newBase + 2));
}

void TriShape::bevelFace(int faceIndex, float amount)
{
    const int start = faceIndex * 3;
    if (start + 2 >= indices.size()) return;

    const int i0 = static_cast<int>(indices[start]);
    const int i1 = static_cast<int>(indices[start + 1]);
    const int i2 = static_cast<int>(indices[start + 2]);

    if (i0 < 0 || i0 >= vertices.size() ||
        i1 < 0 || i1 >= vertices.size() ||
        i2 < 0 || i2 >= vertices.size()) return;

    const Vector3& v0 = vertices[i0];
    const Vector3& v1 = vertices[i1];
    const Vector3& v2 = vertices[i2];

    // Compute face normal
    const float e1x = v1.x - v0.x;
    const float e1y = v1.y - v0.y;
    const float e1z = v1.z - v0.z;
    const float e2x = v2.x - v0.x;
    const float e2y = v2.y - v0.y;
    const float e2z = v2.z - v0.z;

    float nx = e1y * e2z - e1z * e2y;
    float ny = e1z * e2x - e1x * e2z;
    float nz = e1x * e2y - e1y * e2x;
    const float len = sqrtf(nx * nx + ny * ny + nz * nz);
    if (len < 0.0001f) return;
    nx /= len;
    ny /= len;
    nz /= len;

    // Compute midpoints of each edge, offset inward along the face normal
    const Vector3 m01 = {(v0.x + v1.x) * 0.5f + nx * amount,
                          (v0.y + v1.y) * 0.5f + ny * amount,
                          (v0.z + v1.z) * 0.5f + nz * amount};
    const Vector3 m12 = {(v1.x + v2.x) * 0.5f + nx * amount,
                          (v1.y + v2.y) * 0.5f + ny * amount,
                          (v1.z + v2.z) * 0.5f + nz * amount};
    const Vector3 m20 = {(v2.x + v0.x) * 0.5f + nx * amount,
                          (v2.y + v0.y) * 0.5f + ny * amount,
                          (v2.z + v0.z) * 0.5f + nz * amount};

    // Interpolated UVs for midpoints
    const Vector2 uv0 = (i0 < uvs.size()) ? uvs[i0] : Vector2{0.0f, 0.0f};
    const Vector2 uv1 = (i1 < uvs.size()) ? uvs[i1] : Vector2{0.0f, 0.0f};
    const Vector2 uv2 = (i2 < uvs.size()) ? uvs[i2] : Vector2{0.0f, 0.0f};
    const Vector2 uvM01 = {(uv0.u + uv1.u) * 0.5f, (uv0.v + uv1.v) * 0.5f};
    const Vector2 uvM12 = {(uv1.u + uv2.u) * 0.5f, (uv1.v + uv2.v) * 0.5f};
    const Vector2 uvM20 = {(uv2.u + uv0.u) * 0.5f, (uv2.v + uv0.v) * 0.5f};

    const Color4 col = (i0 < colors.size()) ? colors[i0] : Color4{1.0f, 1.0f, 1.0f, 1.0f};

    const int newBase = vertices.size();
    addVertex(m01, uvM01, col);
    addVertex(m12, uvM12, col);
    addVertex(m20, uvM20, col);

    // Remove the original triangle face
    indices.removeAt(start);
    indices.removeAt(start);
    indices.removeAt(start);

    // Add 3 corner triangles replacing the original face
    indices.append(static_cast<unsigned int>(i0));
    indices.append(static_cast<unsigned int>(newBase));
    indices.append(static_cast<unsigned int>(newBase + 2));
    indices.append(static_cast<unsigned int>(i1));
    indices.append(static_cast<unsigned int>(newBase + 1));
    indices.append(static_cast<unsigned int>(newBase));
    indices.append(static_cast<unsigned int>(i2));
    indices.append(static_cast<unsigned int>(newBase + 2));
    indices.append(static_cast<unsigned int>(newBase + 1));
}

// ---------------------------------------------------------------------------
// Real Bethesda NIF import via nifrecord
// ---------------------------------------------------------------------------

static bool parseNifHeader(QIODevice& device, quint32& version, QByteArray& fileHeader)
{
    // Read magic (4 bytes)
    QByteArray magic(4, 0);
    if (device.read(magic.data(), 4) != 4) return false;
    fileHeader = magic;

    // Read version - Bethesda NIF uses 5-byte version string "major.minor.patch"
    // but the binary format stores it as: 3 bytes (major, minor, patch) + null terminator
    // Actually, looking at nifrecord code, version is read as quint32 from blocks
    // The header format varies. Let's try reading 3 version bytes.
    quint8 verBytes[3] = {0};
    if (device.read(reinterpret_cast<char*>(verBytes), 3) != 3) return false;

    // Bethesda version encoding: major << 16 | minor << 8 | patch
    version = (static_cast<quint32>(verBytes[0]) << 16) |
              (static_cast<quint32>(verBytes[1]) << 8) |
              static_cast<quint32>(verBytes[2]);

    // Read filename length (quint16) and filename
    quint16 fileNameLen = 0;
    if (device.read(reinterpret_cast<char*>(&fileNameLen), 2) != 2) return false;
    if (fileNameLen > 1024) {
        device.seek(device.pos() + fileNameLen); // skip
    } else {
        QByteArray fileNameBytes(fileNameLen, 0);
        device.read(fileNameBytes.data(), fileNameLen);
    }

    return true;
}

static QMap<quint32, NifObject*> parseAllBlocks(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    QMap<quint32, NifObject*> blockMap;

    while (device.pos() < device.size()) {
        // Read className length and name to determine block type
        quint32 classNameLen = 0;
        if (device.read(reinterpret_cast<char*>(&classNameLen), 4) != 4) break;
        if (classNameLen > 256) break;

        QByteArray classNameBytes(classNameLen, 0);
        device.read(classNameBytes.data(), classNameLen);
        QString className = QString::fromLatin1(classNameBytes);

        // Read dataRef
        quint32 dataRef = 0;
        device.read(reinterpret_cast<char*>(&dataRef), 4);

        // Create appropriate block type
        NifObject* obj = nullptr;
        if (className == "NiAVObject" || className == "NifNode") {
            // NiNode is the actual class, NiAVObject is base - treat as NifNode
            obj = new NifNode();
        } else if (className.startsWith("NiTriShapeData") || className.startsWith("NiBasedGeomData")) {
            // Data blocks before shapes: "NiTriShapeData" also starts with
            // "NiTriShape" and must not dispatch as a shape.
            obj = new NifTriShapeData();
        } else if (className.startsWith("NiTriShape")) {
            obj = new NifTriShape();
        } else if (className.startsWith("NiTexture")) {
            obj = new NifTexture();
        } else if (className.startsWith("NiMaterial")) {
            obj = new NifMaterial();
        } else if (className.startsWith("NiAlphaProperty")) {
            obj = new NifAlphaProperty();
        } else if (className.startsWith("NiBinaryExtraData")) {
            obj = new NifBinaryExtraData();
        } else if (className.startsWith("NiStringExtraData")) {
            obj = new NifStringExtraData();
        } else if (className.contains("NiKeyframeController", Qt::CaseInsensitive) ||
                   className == "NiKeyFrameController") {
            obj = new NifKeyframeController();
        } else if (className.contains("NiControllerManager", Qt::CaseInsensitive) ||
                   className == "NiControllerSequenceManager") {
            obj = new NifControllerManager();
        } else if (className.startsWith("NiKeyframeData") || className == "NiAnimKeyFrameData") {
            obj = new NifKeyframeData();
        } else if (className.startsWith("NiTransformData")) {
            obj = new NifTransformData();
        } else if (className == "NiSkinInstance" || className == "BSDismemberSkinInstance") {
            obj = new NifSkinInstance();
        } else if (className == "NiSkinData") {
            obj = new NifSkinData();
        } else if (className == "NiLODNode" || className == "BSLODNode") {
            obj = new NifLODNode();
        } else if (className == "NiBillboardNode") {
            obj = new NifBillboardNode();
        } else if (className.startsWith("NiNode") || className == "BSFadeNode" || className == "BSSearchLightNode" || className == "BSPortalNode" || className == "BSParentlessChild") {
            obj = new NifNode();
        } else if (className.startsWith("BSStripTriShapeData")) {
            // Bethesda-specific tri shape data - treat as regular TriShapeData
            obj = new NifTriShapeData();
        } else if (className.contains("BSAnimationGraphShader", Qt::CaseInsensitive)) {
            obj = new NifNode();  // Treat as a node with animation references
        } else if (className.contains("NiParticleSystem", Qt::CaseInsensitive)) {
            obj = new NifParticleSystem();
        } else if (className.contains("NiPSys", Qt::CaseInsensitive) && className.contains("Emitter")) {
            obj = new NifPSysEmitter();
        } else if (className.contains("NiPSys", Qt::CaseInsensitive)) {
            obj = new NifPSysModifier();
        } else if (className.contains("BSLightingShaderProperty", Qt::CaseInsensitive)) {
            obj = new NifBSLightingShaderProperty();
        } else if (className.contains("BSShaderProperty", Qt::CaseInsensitive)) {
            obj = new NifBSLightingShaderProperty();
        } else if (className.startsWith("NiExtraData")) {
            // Generic extra data - skip, we handle specific types above
            continue;
        } else {
            // Unknown block type - skip it by creating a base NifObject
            obj = new NifObject();
        }

        obj->dataRef = dataRef;
        obj->className = className;

        // Parse the block (reads type-specific data from device)
        const qint64 posBeforeParse = device.pos();
        obj->parse(device, version, fileHeader);
        if (device.pos() <= posBeforeParse) {
            // A block that consumes no bytes would spin this loop forever.
            // That happens on non-dialect input (e.g. Gamebryo binaries fed
            // by mistake: the header seek lands mid-file and an unknown
            // block parses to zero bytes), so stop cleanly instead of
            // hanging the caller.
            delete obj;
            break;
        }

        blockMap[dataRef] = obj;
    }

    return blockMap;
}

static void generateBoundingCollisionShapes(Node* root) {
    if (!root || !root->collisionShapes.isEmpty()) return;
    if (root->shapes.isEmpty()) return;

    float minX = 1e30f, minY = 1e30f, minZ = 1e30f;
    float maxX = -1e30f, maxY = -1e30f, maxZ = -1e30f;

    for (const auto& shape : root->shapes) {
        for (const auto& v : shape.vertices) {
            if (v.x < minX) minX = v.x;
            if (v.y < minY) minY = v.y;
            if (v.z < minZ) minZ = v.z;
            if (v.x > maxX) maxX = v.x;
            if (v.y > maxY) maxY = v.y;
            if (v.z > maxZ) maxZ = v.z;
        }
    }

    CollisionShape cs;
    cs.type = CollisionShape::Sphere;
    cs.center = {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f};
    float dx = maxX - minX;
    float dy = maxY - minY;
    float dz = maxZ - minZ;
    cs.radius = sqrtf(dx * dx + dy * dy + dz * dz) * 0.5f;
    if (cs.radius < 0.001f) cs.radius = 1.0f;
    root->collisionShapes.append(cs);
}

static void extractGeometry(const QMap<quint32, NifObject*>& blocks, NifNode* nifRootNode, Node* ourRoot)
{
    // Map from NIF block ref to our Node* for animation linking
    QMap<quint32, Node*> refToNode;
    // Map from NiTriShape block ref to (our parent node, shape index) so the
    // skin-link pass below can attach NiSkinInstance data (§8.1)
    QMap<quint32, QPair<Node*, int>> skinShapeTargets;

    // Walk all AVObjects to find NiTriShape nodes
    QStack<std::pair<NifAVObject*, Node*>> stack;

    // Start with root node's children
    if (nifRootNode) {
        for (auto child : nifRootNode->children) {
            auto avObj = blocks.value(child);
            if (avObj) {
                auto nifAvObj = dynamic_cast<NifAVObject*>(avObj);
                if (nifAvObj) {
                    Node* childNode = new Node();
                    childNode->name = nifAvObj->name;
                    ourRoot->children.append(childNode);
                    refToNode[nifAvObj->dataRef] = childNode;
                    stack.push(std::make_pair(nifAvObj, childNode));
                }
            }
        }
    }

    while (!stack.isEmpty()) {
        auto avObj = stack.top().first;
        auto parentNode = stack.top().second;
        stack.pop();

        // Check if this is a NiTriShape
        auto triShape = dynamic_cast<NifTriShape*>(avObj);
        if (triShape) {
            // Look up geometry data by ref
            auto triData = blocks.value(triShape->refGeometryData);
            auto shapeData = dynamic_cast<NifTriShapeData*>(triData);

            if (shapeData && !shapeData->vertices.isEmpty()) {
                TriShape shape;
                shape.name = avObj->name;

                // Extract vertices, UVs, normals, vertex colors
                for (int i = 0; i < shapeData->vertices.size(); ++i) {
                    const auto& vert = shapeData->vertices[i];
                    Vector3 v{vert.x, vert.y, vert.z};
                    Vector2 uv{i < shapeData->uvs.size() ? shapeData->uvs[i].u : 0.0f,
                               i < shapeData->uvs.size() ? shapeData->uvs[i].v : 0.0f};
                    Vector3 n{i < shapeData->normals.size() ? shapeData->normals[i].x : 0.0f,
                              i < shapeData->normals.size() ? shapeData->normals[i].y : 0.0f,
                              i < shapeData->normals.size() ? shapeData->normals[i].z : 0.0f};
                    Color4 c{i < shapeData->vertexColors.size() ? shapeData->vertexColors[i].r : 1.0f,
                             i < shapeData->vertexColors.size() ? shapeData->vertexColors[i].g : 1.0f,
                             i < shapeData->vertexColors.size() ? shapeData->vertexColors[i].b : 1.0f,
                             i < shapeData->vertexColors.size() ? shapeData->vertexColors[i].a : 1.0f};
                    shape.addVertex(v, uv, c);
                    shape.normals.last() = n;
                }

                // Extract indices
                for (auto idx : shapeData->indices) {
                    shape.indices.append(idx);
                }

                // Try to find material/texture from controllers and extra data
                for (auto ref : avObj->controllers) {
                    auto ctrl = blocks.value(ref);
                    if (!ctrl) continue;

                    auto mat = dynamic_cast<NifMaterial*>(ctrl);
                    if (mat) {
                        shape.baseColor.r = mat->diffuseColor.r;
                        shape.baseColor.g = mat->diffuseColor.g;
                        shape.baseColor.b = mat->diffuseColor.b;
                        shape.baseColor.a = mat->opacity;
                        shape.specularColor = {mat->specularColor.r, mat->specularColor.g, mat->specularColor.b};
                        shape.specularExponent = mat->specularExponent;
                        shape.emissionColor = {mat->emission, mat->emission, mat->emission};
                    }

                    auto shaderProp = dynamic_cast<NifBSLightingShaderProperty*>(ctrl);
                    if (shaderProp) {
                        shape.specularColor = {shaderProp->specularR, shaderProp->specularG, shaderProp->specularB};
                        shape.specularExponent = shaderProp->glossiness;
                        shape.opacity = shaderProp->alpha;
                    }
                }

                // Look for texture in extra data chain
                quint32 extraRef = avObj->refExtraData;
                while (extraRef > 0 && extraRef < 1000000) {
                    auto extra = blocks.value(extraRef);
                    if (!extra) break;

                    auto strExtra = dynamic_cast<NifStringExtraData*>(extra);
                    if (strExtra && strExtra->extraDataName == "Texture File") {
                        shape.texture = strExtra->data;
                        break;
                    }

                    auto texObj = dynamic_cast<NifTexture*>(extra);
                    if (texObj) {
                        if (!texObj->textureFile.isEmpty()) {
                            shape.texture = texObj->textureFile;
                        } else if (!texObj->textureName.isEmpty()) {
                            shape.texture = texObj->textureName;
                        }
                        break;
                    }

                    break;
                }

                // Also check BSLightingShaderProperty for texture path
                // In Skyrim NIFs, texture paths are often stored as extra data on shader properties
                if (shape.texture.isEmpty()) {
                    for (auto ref : avObj->controllers) {
                        auto ctrl = blocks.value(ref);
                        if (!ctrl) continue;

                        auto shaderProp = dynamic_cast<NifBSLightingShaderProperty*>(ctrl);
                        if (shaderProp) {
                            // Check shader property's extra data chain for texture
                            auto shaderAvObj = dynamic_cast<NifAVObject*>(ctrl);
                            if (shaderAvObj) {
                                quint32 shaderExtraRef = shaderAvObj->refExtraData;
                                while (shaderExtraRef > 0 && shaderExtraRef < 1000000) {
                                    auto shaderExtra = blocks.value(shaderExtraRef);
                                    if (!shaderExtra) break;

                                    auto texStr = dynamic_cast<NifStringExtraData*>(shaderExtra);
                                    if (texStr && !texStr->data.isEmpty()) {
                                        shape.texture = texStr->data;
                                        break;
                                    }

                                    auto texObj2 = dynamic_cast<NifTexture*>(shaderExtra);
                                    if (texObj2 && !texObj2->textureFile.isEmpty()) {
                                        shape.texture = texObj2->textureFile;
                                        break;
                                    }

                                    break;
                                }
                            }
                            if (!shape.texture.isEmpty()) break;
                        }
                    }
                }

                parentNode->shapes.append(shape);
                skinShapeTargets[triShape->dataRef] =
                    qMakePair(parentNode, parentNode->shapes.size() - 1);
            }
        }

        // Push children onto stack
        for (auto childRef : avObj->children) {
            auto childObj = blocks.value(childRef);
            if (!childObj) continue;

            // Check for NifParticleSystem blocks (they don't inherit from NifAVObject)
            auto psBlock = dynamic_cast<NifParticleSystem*>(childObj);
            if (psBlock) {
                NifParticleSystemSettings ps;
                ps.emissionRate = psBlock->settings.emissionRate;
                ps.lifetime = psBlock->settings.lifetime;
                ps.lifetimeRandom = psBlock->settings.lifetimeRandom;
                ps.speed = psBlock->settings.speed;
                ps.speedRandom = psBlock->settings.speedRandom;
                ps.spread = psBlock->settings.spread;
                ps.gravityStrength = psBlock->settings.gravityStrength;
                ps.startSize = psBlock->settings.startSize;
                ps.startSizeRandom = psBlock->settings.startSizeRandom;
                ps.endSize = psBlock->settings.endSize;
                ps.endSizeRandom = psBlock->settings.endSizeRandom;
                ps.emitRadius = psBlock->settings.emitRadius;
                ps.emitAngle = psBlock->settings.emitAngle;
                ps.emitterPosition = {psBlock->settings.emitterPosition.x,
                                      psBlock->settings.emitterPosition.y,
                                      psBlock->settings.emitterPosition.z};
                ps.emitterDirection = {psBlock->settings.emitterDirection.x,
                                       psBlock->settings.emitterDirection.y,
                                       psBlock->settings.emitterDirection.z};
                ps.texturePath = psBlock->settings.texturePath;
                ps.numColumns = psBlock->settings.numColumns;
                ps.numRows = psBlock->settings.numRows;
                ps.rendererType = psBlock->settings.rendererType;
                ps.additiveBlending = psBlock->settings.additiveBlending;
                ps.alphaTest = psBlock->settings.alphaTest;
                ps.startColorR = psBlock->settings.startColorR;
                ps.startColorG = psBlock->settings.startColorG;
                ps.startColorB = psBlock->settings.startColorB;
                ps.startColorA = psBlock->settings.startColorA;
                ps.endColorR = psBlock->settings.endColorR;
                ps.endColorG = psBlock->settings.endColorG;
                ps.endColorB = psBlock->settings.endColorB;
                ps.endColorA = psBlock->settings.endColorA;
                parentNode->particleSystems.append(ps);
                LOG_INFO(QString("Extracted particle system from block %1 on node '%2'")
                             .arg(childRef).arg(parentNode->name));
                continue;
            }

            auto childAvObj = dynamic_cast<NifAVObject*>(childObj);
            if (childAvObj) {
                Node* childNode = new Node();
                childNode->name = childAvObj->name;

                // Check for LOD node
                auto lodNode = dynamic_cast<NifLODNode*>(childObj);
                if (lodNode) {
                    childNode->isLODNode = true;
                    childNode->lodMinScreens.clear();
                    childNode->lodMaxScreens.clear();
                    for (const auto& level : lodNode->lodLevels) {
                        childNode->lodMinScreens.append(level.minScreenSize);
                        childNode->lodMaxScreens.append(level.maxScreenSize);
                    }
                    if (!lodNode->lodLevels.isEmpty()) {
                        childNode->lodMinScreenSize = lodNode->lodLevels[0].minScreenSize;
                        childNode->lodMaxScreenSize = lodNode->lodLevels[0].maxScreenSize;
                    }
                }

                // Check for billboard node
                auto bbNode = dynamic_cast<NifBillboardNode*>(childObj);
                if (bbNode) {
                    childNode->isBillboardNode = true;
                    childNode->billboardMode = bbNode->mode;
                }

                parentNode->children.append(childNode);
                refToNode[childAvObj->dataRef] = childNode;
                stack.push(std::make_pair(childAvObj, childNode));
            }
        }
    }

    // Detect NiBSPHShapeData / NiBSBound collision blocks
    for (auto it = blocks.constBegin(); it != blocks.constEnd(); ++it) {
        const QString& cn = it.value()->className;
        if (cn.contains("NiBSPHShapeData", Qt::CaseInsensitive) ||
            cn.contains("NiBSBound", Qt::CaseInsensitive)) {
            CollisionShape cs;
            cs.type = CollisionShape::Sphere;
            cs.center = {0, 0, 0};
            cs.radius = 1.0f;
            ourRoot->collisionShapes.append(cs);
        }
    }

    // Fallback: generate bounding sphere from mesh if no collision data found
    generateBoundingCollisionShapes(ourRoot);

    // Connect NiControllerManager clip names to their NifKeyframeController blocks
    for (auto it = blocks.constBegin(); it != blocks.constEnd(); ++it) {
        auto ctrlMgr = dynamic_cast<NifControllerManager*>(it.value());
        if (!ctrlMgr) continue;

        for (int i = 0; i < ctrlMgr->controllerRefs.size(); ++i) {
            quint32 ctrlRef = ctrlMgr->controllerRefs[i];
            auto ctrlObj = blocks.value(ctrlRef);
            if (ctrlObj) {
                auto kfCtrl = dynamic_cast<NifKeyframeController*>(ctrlObj);
                if (kfCtrl && i < ctrlMgr->clipNames.size()) {
                    kfCtrl->clipName = ctrlMgr->clipNames[i];
                }
            }
        }

        for (const QString& clipName : ctrlMgr->clipNames) {
            LOG_INFO(QString("Found animation clip: %1").arg(clipName));
        }
    }

    // Extract animation data from NiKeyframeController blocks
    for (auto it = blocks.constBegin(); it != blocks.constEnd(); ++it) {
        auto kfCtrl = dynamic_cast<NifKeyframeController*>(it.value());
        if (!kfCtrl || kfCtrl->targetNode == 0) continue;

        Node* targetNode = refToNode.value(kfCtrl->targetNode);
        if (!targetNode) continue;

        Nif::NiKeyframeController anim;
        anim.targetNode = kfCtrl->targetNode;
        anim.clipName = kfCtrl->clipName;

        // Try to read inline keyframe data from the keyframeDataRef block
        if (kfCtrl->keyframeDataRef > 0) {
            auto dataBlock = blocks.value(kfCtrl->keyframeDataRef);
            if (dataBlock) {
                auto kfData = dynamic_cast<NifKeyframeData*>(dataBlock);
                if (kfData && !kfData->keyframes.isEmpty()) {
                    anim.keyframes = kfData->keyframes;
                } else {
                    auto transformData = dynamic_cast<NifTransformData*>(dataBlock);
                    if (transformData) {
                        int maxKeys = qMax(static_cast<int>(transformData->numTranslationKeys),
                                      qMax(static_cast<int>(transformData->numRotationKeys),
                                           static_cast<int>(transformData->numScaleKeys)));
                        for (int i = 0; i < maxKeys; ++i) {
                            Nif::TransformKeyframe tk;
                            tk.time = (i < transformData->translateKeys.size()) ?
                                      transformData->translateKeys[i].time :
                                      (i < transformData->rotateKeys.size()) ? transformData->rotateKeys[i].time : 0;
                            if (i < transformData->translateKeys.size()) {
                                tk.translation = transformData->translateKeys[i].value;
                            }
                            if (i < transformData->rotateKeys.size()) {
                                tk.rotation = transformData->rotateKeys[i];
                            }
                            if (i < transformData->scaleKeys.size()) {
                                tk.scale = transformData->scaleKeys[i].value;
                            }
                            anim.keyframes.append(tk);
                        }
                    }
                }
            }
        }

        targetNode->animations.append(anim);
        targetNode->hasAnimation = true;
    }

    // Link NiSkinInstance blocks to their shapes (per-vertex skinning, §8.1).
    // Shapes without a skin instance keep rigid owner-node deformation.
    for (auto it = blocks.constBegin(); it != blocks.constEnd(); ++it) {
        auto skinInst = dynamic_cast<NifSkinInstance*>(it.value());
        if (!skinInst)
            continue;
        if (!skinShapeTargets.contains(skinInst->refTargetShape)) {
            LOG_WARNING(QString("NiSkinInstance block %1 targets unknown shape block %2")
                            .arg(it.key()).arg(skinInst->refTargetShape));
            continue;
        }
        const QPair<Node*, int> target = skinShapeTargets.value(skinInst->refTargetShape);
        Node* shapeNode = target.first;
        const int shapeIdx = target.second;
        if (!shapeNode || shapeIdx < 0 || shapeIdx >= shapeNode->shapes.size())
            continue;
        TriShape& shape = shapeNode->shapes[shapeIdx];

        auto skinDataObj = blocks.value(skinInst->refSkinData);
        auto skinData = dynamic_cast<NifSkinData*>(skinDataObj);
        if (!skinData) {
            LOG_WARNING(QString("NiSkinInstance block %1 references missing NiSkinData %2")
                            .arg(it.key()).arg(skinInst->refSkinData));
            continue;
        }
        if (skinInst->bones.size() != skinData->bones.size()) {
            LOG_WARNING(QString("NiSkinInstance block %1 bone count %2 != NiSkinData %3 bone count %4")
                            .arg(it.key()).arg(skinInst->bones.size())
                            .arg(skinInst->refSkinData).arg(skinData->bones.size()));
            continue;
        }

        QVector<SkinBone> linkedBones;
        bool bonesOk = true;
        for (quint32 boneRef : skinInst->bones) {
            Node* boneNode = refToNode.value(boneRef);
            if (!boneNode) {
                LOG_WARNING(QString("NiSkinInstance block %1 references unknown bone block %2")
                                .arg(it.key()).arg(boneRef));
                bonesOk = false;
                break;
            }
            SkinBone bone;
            bone.boneName = boneNode->name;
            bone.boneNode = boneNode;
            linkedBones.append(bone);
        }
        if (!bonesOk || linkedBones.isEmpty())
            continue;

        QVector<SkinVertexWeight> linkedWeights;
        for (int b = 0; b < skinData->bones.size(); ++b) {
            for (const auto& inf : skinData->bones[b].weights) {
                if (inf.vertex >= static_cast<quint32>(shape.vertices.size())) {
                    LOG_WARNING(QString("NiSkinInstance block %1 weight on out-of-range vertex %2")
                                    .arg(it.key()).arg(inf.vertex));
                    continue;
                }
                if (inf.weight <= 0.0f)
                    continue;
                SkinVertexWeight w;
                w.vertex = inf.vertex;
                w.bone = static_cast<quint32>(b);
                w.weight = inf.weight;
                linkedWeights.append(w);
            }
        }
        if (linkedWeights.isEmpty())
            continue;

        shape.skinBones = linkedBones;
        shape.skinWeights = linkedWeights;
        LOG_INFO(QString("Linked skinning to shape '%1': %2 bones, %3 weights")
                     .arg(shape.name).arg(shape.skinBones.size()).arg(shape.skinWeights.size()));
    }

    // Starfield BSSkin triplets (§8.3): [BSGeometry][extras]*[SkinAttach]
    // [BSSkin::Instance][BSSkin::BoneData] (a BSClothExtraData may sit
    // between Attach and Instance). Populates TriShape bone names from the
    // attach block and per-vertex weights from the resolved external .mesh
    // (weights are attach-local: maxBone == attachBones-1 on every shipped
    // face shape). Bones link to scene nodes by name when the skeleton is
    // present in the file; otherwise the bind-pose blend is an identity.
    {
        QMap<QString, Node*> nameToNode;
        std::function<void(Node*)> collectNames = [&](Node* n) {
            if (!n) return;
            if (!n->name.isEmpty() && !nameToNode.contains(n->name))
                nameToNode.insert(n->name, n);
            for (Node* c : n->children) collectNames(c);
        };
        collectNames(ourRoot);
        qint32 lastGeom = -1;
        qint32 pendingAttach = -1;
        for (auto it = blocks.constBegin(); it != blocks.constEnd(); ++it) {
            const QString& cn = it.value()->className;
            if (cn == QLatin1String("BSGeometry")) {
                if (dynamic_cast<NifTriShape*>(it.value()) != nullptr)
                    lastGeom = static_cast<qint32>(it.key());
                continue;
            }
            if (cn == QLatin1String("SkinAttach")) {
                if (lastGeom >= 0 && dynamic_cast<NifSkinAttach*>(it.value()) != nullptr)
                    pendingAttach = static_cast<qint32>(it.key());
                continue;
            }
            if (cn != QLatin1String("BSSkin::Instance") || pendingAttach < 0)
                continue;
            auto* inst = dynamic_cast<NifBSSkinInstance*>(it.value());
            auto* attach = dynamic_cast<NifSkinAttach*>(
                blocks.value(static_cast<quint32>(pendingAttach)));
            pendingAttach = -1;
            if (!inst || !attach) continue;
            auto* shell = dynamic_cast<NifTriShape*>(
                blocks.value(static_cast<quint32>(lastGeom)));
            if (!shell) continue;
            auto target = skinShapeTargets.find(shell->dataRef);
            if (target == skinShapeTargets.end()) continue;
            Node* parentNode = target.value().first;
            const int shapeIdx = target.value().second;
            if (!parentNode || shapeIdx < 0 || shapeIdx >= parentNode->shapes.size())
                continue;
            TriShape& shape = parentNode->shapes[shapeIdx];
            shape.skinBones.clear();
            for (const QString& boneName : attach->boneNames) {
                SkinBone bone;
                bone.boneName = boneName;
                bone.boneNode = nameToNode.value(boneName, nullptr);
                shape.skinBones.append(bone);
            }
            shape.skinWeights.clear();
            if (auto* mdata = dynamic_cast<NifTriShapeData*>(
                    blocks.value(shell->refGeometryData))) {
                const quint32 wpv = mdata->skinWeightsPerVertex;
                const int n = mdata->skinBoneIndices.size();
                if (wpv > 0 && n == mdata->skinBoneWeights.size()) {
                    for (int e = 0, v = 0; e + static_cast<int>(wpv) <= n;
                         e += static_cast<int>(wpv), ++v) {
                        for (quint32 k = 0; k < wpv; ++k) {
                            const quint16 b = mdata->skinBoneIndices[e + k];
                            const quint16 w = mdata->skinBoneWeights[e + k];
                            if (w == 0 || b >= shape.skinBones.size()) continue;
                            SkinVertexWeight sw;
                            sw.vertex = static_cast<quint32>(v);
                            sw.bone = b;
                            sw.weight = w / 65535.0f;
                            shape.skinWeights.append(sw);
                        }
                    }
                }
            }
            LOG_INFO(QString("Linked BSSkin to shape '%1': %2 bones, %3 weights")
                         .arg(shape.name).arg(shape.skinBones.size())
                         .arg(shape.skinWeights.size()));
        }
    }
}

// ---------------------------------------------------------------------------
// Gamebryo 20.2.0.7 reader (§8.3). Shipped Starfield NIFs are real Gamebryo
// binaries: index-based blocks (type table + per-block size table) whose
// names live in a header string table. Layouts below are validated against
// every loose shipped NIF (header grammar from nif.xml Header/BSStreamHeader,
// NiNode/BSGeometry bodies from NifSkope's Starfield definitions, all
// re-checked byte-for-byte in Python first). Anything unexpected rejects
// the file (returns false) instead of guessing.
// ---------------------------------------------------------------------------

namespace Gamebryo {

struct Header {
    quint32 userVersion = 0;
    quint32 numBlocks = 0;
    quint32 bsVersion = 0;
    QStringList blockTypes;
    QVector<quint16> typeIndex;
    QVector<quint32> blockSize;
    QStringList strings;
    QVector<qint64> offsets;
    quint32 numGroups = 0;
};

// Bounds-checked little-endian reader. Every read validates range and
// stream status; ok flips false (sticky) on the first failure.
struct Reader {
    QDataStream& s;
    const qint64 fileSize;
    bool ok = true;

    quint8 u8() { quint8 v = 0; check(1); if (ok) s >> v; return v; }
    quint16 u16() { quint16 v = 0; check(2); if (ok) s >> v; return v; }
    quint32 u32() { quint32 v = 0; check(4); if (ok) s >> v; return v; }
    qint32 i32() { qint32 v = 0; check(4); if (ok) s >> v; return v; }
    quint64 u64() { quint64 v = 0; check(8); if (ok) s >> v; return v; }
    // NOTE: operator>>(float&) over-reads in this build (see parseBsMeshData);
    // floats are read as bits explicitly.
    float f32() {
        quint32 bits = 0;
        check(4);
        if (ok) s >> bits;
        float v = 0.0f;
        memcpy(&v, &bits, 4);
        return v;
    }
    void skip(qint64 n) {
        if (n < 0 || s.device()->pos() + n > fileSize) { ok = false; return; }
        if (!s.device()->seek(s.device()->pos() + n)) ok = false;
    }
    // NIF SizedString: u32 length + bytes (may contain backslashes/NULs).
    QString sizedString(quint32 maxLen = 4096) {
        const quint32 len = u32();
        if (!ok || len > maxLen) { ok = false; return QString(); }
        QByteArray bytes(static_cast<int>(len), 0);
        check(len);
        if (!ok) return QString();
        if (s.readRawData(bytes.data(), len) != static_cast<int>(len)) { ok = false; return QString(); }
        return QString::fromLatin1(bytes);
    }
    // Raw bytes (opaque payloads): exact-length read, no interpretation.
    QByteArray rawBytes(qint64 n, qint64 maxLen = 1048576) {
        QByteArray out;
        if (n < 0 || n > maxLen) { ok = false; return out; }
        check(n);
        if (!ok) return out;
        out.resize(static_cast<int>(n));
        if (n > 0 && s.readRawData(out.data(), n) != n) { ok = false; out.clear(); }
        return out;
    }
    // NIF ExportString: u8 length (including NUL) + bytes.
    QString exportString() {
        const quint8 len = u8();
        if (!ok || len == 0 || len > 250) { ok = false; return QString(); }
        QByteArray bytes(len, 0);
        check(len);
        if (!ok) return QString();
        if (s.readRawData(bytes.data(), len) != static_cast<int>(len)) { ok = false; return QString(); }
        return QString::fromLatin1(bytes.constData(), len);
    }

private:
    void check(qint64 n) {
        if (!ok || s.status() != QDataStream::Ok
            || s.device()->pos() + n > fileSize)
            ok = false;
    }
};

static bool isAsciiName(const QString& s)
{
    if (s.isEmpty() || s.size() > 128) return false;
    for (QChar c : s)
        if (!c.isPrint() || c.unicode() > 127) return false;
    return true;
}

bool parseHeader(QFile& file, Header& h)
{
    QDataStream s(&file);
    s.setByteOrder(QDataStream::LittleEndian);
    Reader r{ s, file.size() };

    // Magic line, NUL-free, exact version.
    QByteArray magic;
    while (magic.size() < 128) {
        const quint8 c = r.u8();
        if (!r.ok) return false;
        if (c == '\n') break;
        magic.append(static_cast<char>(c));
    }
    if (magic != "Gamebryo File Format, Version 20.2.0.7") return false;

    if (r.u32() != 0x14020007) return false;   // version dword
    if (r.u8() != 1) return false;             // little-endian
    h.userVersion = r.u32();
    h.numBlocks = r.u32();
    if (!r.ok || h.numBlocks == 0 || h.numBlocks > 2000000) return false;
    h.bsVersion = r.u32();
    if (h.bsVersion == 0) return false;

    r.exportString();                          // Author
    r.u32();                                   // Unknown Int (BS > 130)
    r.exportString();                          // Export Script
    r.exportString();                          // Max Filepath
    if (!r.ok) return false;

    const quint16 numTypes = r.u16();
    if (!r.ok || numTypes == 0 || numTypes > 1000) return false;
    for (int i = 0; i < numTypes; ++i) {
        const QString t = r.sizedString(128);
        if (!r.ok || !isAsciiName(t)) return false;
        h.blockTypes.append(t);
    }
    h.typeIndex.reserve(h.numBlocks);
    for (quint32 i = 0; i < h.numBlocks; ++i) {
        const quint16 ti = r.u16();
        if (!r.ok || ti >= static_cast<quint16>(h.blockTypes.size())) return false;
        h.typeIndex.append(ti);
    }
    h.blockSize.reserve(h.numBlocks);
    for (quint32 i = 0; i < h.numBlocks; ++i)
        h.blockSize.append(r.u32());
    if (!r.ok) return false;

    const quint32 numStrings = r.u32();
    const quint32 maxStringLen = r.u32();
    if (!r.ok || numStrings > 200000 || maxStringLen > 4096) return false;
    for (quint32 i = 0; i < numStrings; ++i) {
        h.strings.append(r.sizedString(maxStringLen + 1));
        if (!r.ok) return false;
    }
    h.numGroups = r.u32();
    if (!r.ok || h.numGroups > 10000) return false;
    r.skip(static_cast<qint64>(h.numGroups) * 4);
    if (!r.ok) return false;

    h.offsets.reserve(h.numBlocks);
    qint64 pos = file.pos();
    for (quint32 i = 0; i < h.numBlocks; ++i) {
        h.offsets.append(pos);
        pos += h.blockSize[i];
        if (pos > file.size()) return false;
    }
    return true;
}

// NiObjectNET + NiAVObject prefix shared by NiNode/BSGeometry at BS172:
// name index, extra-data list, controller, flags, TRS, collision ref.
// Advances the reader past the prefix; name/flags out.
bool parseAvPrefix(Reader& r, const Header& h, QString& nameOut, quint32& flagsOut)
{
    const quint32 nameIdx = r.u32();
    if (!r.ok || nameIdx >= static_cast<quint32>(h.strings.size())) return false;
    nameOut = h.strings.at(nameIdx);
    const quint32 numExtra = r.u32();
    if (!r.ok || numExtra > 10000) return false;
    for (quint32 i = 0; i < numExtra; ++i) {
        const qint32 ref = r.i32();
        if (!r.ok || ref < -1 || ref >= static_cast<qint32>(h.numBlocks)) return false;
    }
    r.i32();                       // controller
    flagsOut = r.u32();
    r.skip(12 + 36 + 4);           // translation, rotation, scale
    r.i32();                       // collision object
    return r.ok;
}

NifNode* parseNode(QFile& file, const Header& h, quint32 index, qint64& consumed)
{
    QDataStream s(&file);
    s.setByteOrder(QDataStream::LittleEndian);
    Reader r{ s, file.size() };
    if (!file.seek(h.offsets[index])) return nullptr;
    const qint64 start = file.pos();

    QString name;
    quint32 flags = 0;
    if (!parseAvPrefix(r, h, name, flags)) return nullptr;
    const quint32 numChildren = r.u32();
    if (!r.ok || numChildren > 100000) return nullptr;
    NifNode* node = new NifNode();
    node->className = QStringLiteral("NiNode");
    node->dataRef = index;
    node->name = name;
    for (quint32 i = 0; i < numChildren; ++i) {
        const qint32 ref = r.i32();
        if (!r.ok) { delete node; return nullptr; }
        if (ref >= 0 && ref < static_cast<qint32>(h.numBlocks))
            node->children.append(static_cast<quint32>(ref));
    }
    if (!r.ok) { delete node; return nullptr; }
    consumed = file.pos() - start;
    return node;
}

// Starfield BSGeometry shell (NifSkope's #STF# definition): bounds, box,
// skin/shader/alpha refs, then 4 mesh slots. Slots either carry an external
// .mesh path (Flags & 512 == 0, the shipped case — meshes live in BA2s) or
// inline BSMeshData (Flags & 512; not decoded here). Returns a vert-less
// NifTriShape shell plus any external paths. consumed must equal the
// declared block size or the block is rejected.
NifTriShape* parseGeometry(QFile& file, const Header& h, quint32 index,
                           QStringList& externalMeshes, qint64& consumed)
{
    QDataStream s(&file);
    s.setByteOrder(QDataStream::LittleEndian);
    Reader r{ s, file.size() };
    if (!file.seek(h.offsets[index])) return nullptr;
    const qint64 start = file.pos();

    QString name;
    quint32 flags = 0;
    if (!parseAvPrefix(r, h, name, flags)) return nullptr;
    r.skip(16);                    // bounding sphere
    r.skip(24);                    // bounding box
    for (int i = 0; i < 3; ++i) {
        const qint32 ref = r.i32();  // skin, shader, alpha
        if (!r.ok || ref < -1 || ref >= static_cast<qint32>(h.numBlocks)) return nullptr;
    }
    NifTriShape* shape = new NifTriShape();
    shape->className = QStringLiteral("BSGeometry");
    shape->dataRef = index;
    shape->name = name;
    shape->refGeometryData = 0xFFFFFFFFu;   // no local data block
    for (int slot = 0; slot < 4; ++slot) {
        if (r.u8() == 0) continue;          // empty slot
        if (!r.ok) { delete shape; return nullptr; }
        r.u32(); r.u32(); r.u32();          // indices size, num verts, flags
        if (!r.ok) { delete shape; return nullptr; }
        if (flags & 512) {
            // Inline BSMeshData: not decoded in this slice. Reject the
            // block (it seeks past by size) rather than misparse it.
            delete shape;
            return nullptr;
        }
        const QString path = r.sizedString(512);
        if (!r.ok || path.isEmpty()) { delete shape; return nullptr; }
        externalMeshes.append(path);
        shape->externalMeshPaths.append(path);
    }
    if (!r.ok) { delete shape; return nullptr; }
    consumed = file.pos() - start;
    return shape;
}

// Starfield skin blocks (§8.3): SkinAttach (bone names), BSSkin::Instance
// (target/bonedata refs, per-bone 16B opaque payloads), BSSkin::BoneData
// (per-bone 4x4 matrix + scale). Strict counts (bones < 100000); the
// caller verifies exact block-size consumption.
NifSkinAttach* parseSkinAttach(QFile& file, const Header& h, quint32 index, qint64& consumed)
{
    QDataStream s(&file);
    s.setByteOrder(QDataStream::LittleEndian);
    Reader r{ s, file.size() };
    if (!file.seek(h.offsets[index])) return nullptr;
    const qint64 start = file.pos();

    auto* attach = new NifSkinAttach();
    attach->className = QStringLiteral("SkinAttach");
    attach->dataRef = index;
    attach->unknown = r.u32();
    const quint32 nameCount = r.u32();
    if (!r.ok || nameCount > 100000) { delete attach; return nullptr; }
    for (quint32 i = 0; i < nameCount; ++i) {
        const QString name = r.sizedString(256);
        if (!r.ok) { delete attach; return nullptr; }
        attach->boneNames.append(name);
    }
    if (!r.ok) { delete attach; return nullptr; }
    consumed = file.pos() - start;
    return attach;
}

NifBSSkinInstance* parseSkinInstance(QFile& file, const Header& h, quint32 index, qint64& consumed)
{
    QDataStream s(&file);
    s.setByteOrder(QDataStream::LittleEndian);
    Reader r{ s, file.size() };
    if (!file.seek(h.offsets[index])) return nullptr;
    const qint64 start = file.pos();

    auto* inst = new NifBSSkinInstance();
    inst->className = QStringLiteral("BSSkin::Instance");
    inst->dataRef = index;
    inst->refTarget = r.u32();
    inst->refBoneData = r.u32();
    const quint32 boneCount = r.u32();
    inst->headerFlag = r.i32();
    // 0xFFFFFFFF is the null-ref convention; anything else must index a block.
    const bool refsOk = (inst->refTarget == 0xFFFFFFFFu || inst->refTarget < h.numBlocks)
        && (inst->refBoneData == 0xFFFFFFFFu || inst->refBoneData < h.numBlocks);
    if (!r.ok || boneCount > 100000 || !refsOk) {
        delete inst;
        return nullptr;
    }
    for (quint32 i = 0; i < boneCount; ++i) {
        const QByteArray payload = r.rawBytes(16);
        if (!r.ok) { delete inst; return nullptr; }
        inst->bonePayloads.append(payload);
    }
    if (!r.ok) { delete inst; return nullptr; }
    consumed = file.pos() - start;
    return inst;
}

// BSFaceGenNiNode roots are NiNode bodies with 2 trailing bytes of unknown
// purpose (0x0001 in all samples). Parsed as NifNode so the hierarchy walk
// works unchanged; the tail is preserved on the node.
NifNode* parseFaceGenNode(QFile& file, const Header& h, quint32 index, qint64& consumed)
{
    NifNode* node = parseNode(file, h, index, consumed);
    if (!node) return nullptr;
    node->className = QStringLiteral("BSFaceGenNiNode");
    QDataStream s(&file);
    s.setByteOrder(QDataStream::LittleEndian);
    Reader r{ s, file.size() };
    node->faceGenTail = r.u16();
    if (!r.ok) { delete node; return nullptr; }
    consumed = file.pos() - h.offsets[index];
    return node;
}

NifBSSkinBoneData* parseSkinBoneData(QFile& file, const Header& h, quint32 index, qint64& consumed)
{
    QDataStream s(&file);
    s.setByteOrder(QDataStream::LittleEndian);
    Reader r{ s, file.size() };
    if (!file.seek(h.offsets[index])) return nullptr;
    const qint64 start = file.pos();

    auto* bd = new NifBSSkinBoneData();
    bd->className = QStringLiteral("BSSkin::BoneData");
    bd->dataRef = index;
    const quint32 boneCount = r.u32();
    if (!r.ok || boneCount > 100000) { delete bd; return nullptr; }
    for (quint32 i = 0; i < boneCount; ++i) {
        NifBSSkinBoneData::Bone bone;
        for (int f = 0; f < 16; ++f)
            bone.matrix[f] = r.f32();
        const quint32 scaleBits = r.u32();
        if (!r.ok) { delete bd; return nullptr; }
        memcpy(&bone.scale, &scaleBits, 4);
        bd->bones.append(bone);
    }
    if (!r.ok) { delete bd; return nullptr; }
    consumed = file.pos() - start;
    return bd;
}

} // namespace Gamebryo

namespace {

// Process-wide BA2 cache: opening multi-GB mesh archives costs ~a second
// each, so archives (and their filename indices) are opened once and shared
// by every NIF load. NIF loads run on the main thread; not thread-safe.
struct ArchiveCacheEntry {
    std::shared_ptr<Ba2Archive> archive;
    QHash<QString, quint32> byLowerPath;
    bool indexed = false;
};

QMap<QString, ArchiveCacheEntry>& archiveCache()
{
    static QMap<QString, ArchiveCacheEntry> cache;
    return cache;
}

bool isHexWord(const QString& s)
{
    if (s.isEmpty() || s.size() > 64) return false;
    for (const QChar c : s) {
        const char a = c.toLower().toLatin1();
        if (!(a >= '0' && a <= '9') && !(a >= 'a' && a <= 'f')) return false;
    }
    return true;
}

} // namespace

MeshArchiveResolver MeshArchiveResolver::forNif(const QString& nifPath)
{
    MeshArchiveResolver r;
    QDir d = QFileInfo(nifPath).absoluteDir();
    for (int i = 0; i < 6; ++i) {
        if (d.dirName().compare(QStringLiteral("meshes"), Qt::CaseInsensitive) == 0) {
            QDir data = d;
            data.cdUp();
            r.m_dataDir = data.absolutePath();
            break;
        }
        QDir parent = d;
        if (!parent.cdUp() || parent == d) break;
        d = parent;
    }
    if (r.m_dataDir.isEmpty()) {
        const QString env = qEnvironmentVariable("OPENCK_DATA_DIR",
            QStringLiteral("C:/XboxGames/Starfield/Content/Data"));
        if (QDir(env).exists())
            r.m_dataDir = env;
        else
            return r;
    }
    const QStringList ba2s = QDir(r.m_dataDir).entryList(
        QStringList({ QStringLiteral("*Meshes*.ba2") }), QDir::Files);
    for (const QString& b : ba2s) {
        if (r.m_archives.size() >= 8) break;
        r.addArchive(QDir(r.m_dataDir).filePath(b));
    }
    return r;
}

bool MeshArchiveResolver::hasArchives() const
{
    return !m_archives.isEmpty();
}

bool MeshArchiveResolver::addArchive(const QString& ba2Path)
{
    auto& cache = archiveCache();
    auto hit = cache.find(ba2Path);
    if (hit == cache.end()) {
        auto archive = std::make_shared<Ba2Archive>();
        if (!archive->open(ba2Path))
            return false;
        hit = cache.insert(ba2Path, ArchiveCacheEntry{ archive, {}, false });
    }
    m_archives.insert(ba2Path, hit.value().archive);
    return true;
}

const QHash<QString, quint32>* MeshArchiveResolver::nameIndexFor(const QString& ba2Path)
{
    static const QHash<QString, quint32> empty;
    auto ait = m_archives.find(ba2Path);
    if (ait == m_archives.end()) return &empty;
    auto& cache = archiveCache();
    auto cit = cache.find(ba2Path);
    if (cit == cache.end()) return &empty;
    if (!cit.value().indexed) {
        const QVector<Ba2FileEntry>& entries = ait.value()->entries();
        cit.value().byLowerPath.reserve(entries.size());
        for (int i = 0; i < entries.size(); ++i)
            cit.value().byLowerPath.insert(entries.at(i).relativePath.toLower(), static_cast<quint32>(i));
        cit.value().indexed = true;
    }
    return &cit.value().byLowerPath;
}

QByteArray MeshArchiveResolver::meshBytes(const QString& meshPath)
{
    if (meshPath.isEmpty()) return {};
    auto cached = m_bytesCache.find(meshPath);
    if (cached != m_bytesCache.end()) return cached.value();

    QByteArray out;
    // Normalize: forward slashes, single .mesh suffix.
    QString norm = meshPath;
    norm.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (norm.endsWith(QStringLiteral(".mesh"), Qt::CaseInsensitive))
        norm.chop(5);
    // Full "Geometries/h1/h2" form (face NIFs carry it with the suffix).
    if (norm.startsWith(QStringLiteral("geometries/"), Qt::CaseInsensitive)) {
        const QString arch = norm.toLower() + QStringLiteral(".mesh");
        for (auto it = m_archives.begin(); it != m_archives.end(); ++it) {
            const quint32 e = nameIndexFor(it.key())->value(arch, 0xFFFFFFFFu);
            if (e == 0xFFFFFFFFu) continue;
            if (it.value()->extractToBytes(e, out) && !out.isEmpty()) break;
            out.clear();
        }
        if (!out.isEmpty()) {
            m_bytesCache.insert(meshPath, out);
            return out;
        }
    }
    // Hash form "h1\h2" addresses geometries/h1/h2.mesh directly.
    const int slash = norm.indexOf(QLatin1Char('/'));
    if (slash > 0 && isHexWord(norm.left(slash)) && isHexWord(norm.mid(slash + 1))) {
        const QString arch = QStringLiteral("geometries/")
            + norm.left(slash).toLower() + QLatin1Char('/')
            + norm.mid(slash + 1).toLower() + QStringLiteral(".mesh");
        for (auto it = m_archives.begin(); it != m_archives.end(); ++it) {
            const quint32 e = nameIndexFor(it.key())->value(arch, 0xFFFFFFFFu);
            if (e == 0xFFFFFFFFu) continue;
            if (it.value()->extractToBytes(e, out) && !out.isEmpty()) break;
            out.clear();
        }
        if (!out.isEmpty()) {
            m_bytesCache.insert(meshPath, out);
            return out;
        }
    }
    // Loose file fallback (modder-supplied meshes next to the NIFs).
    if (!m_dataDir.isEmpty()) {
        QFile loose(QDir(m_dataDir).filePath(QStringLiteral("meshes/") + norm + QStringLiteral(".mesh")));
        if (loose.open(QIODevice::ReadOnly)) {
            out = loose.readAll();
            m_bytesCache.insert(meshPath, out);
            return out;
        }
    }
    // Name-style paths ("SomeFolder\\SomeMesh") match no BA2 entry anywhere
    // in the shipped install (verified across all Meshes archives); they are
    // left unresolved rather than guessed at. No substring search: it would
    // risk false matches with zero demonstrated benefit.
    return {};
}

static float halfToFloat(quint16 h)
{
    const quint32 sign = (h >> 15) & 1;
    quint32 exp = (h >> 10) & 0x1F;
    quint32 mant = h & 0x3FF;
    quint32 bits;
    if (exp == 0) {
        if (mant == 0) {
            bits = sign << 31;
        } else {
            // Subnormal: normalize.
            exp = 1;
            while ((mant & 0x400) == 0) { mant <<= 1; --exp; }
            mant &= 0x3FF;
            bits = (sign << 31) | ((exp + 112) << 23) | (mant << 13);
        }
    } else if (exp == 31) {
        bits = (sign << 31) | (0xFF << 23) | (mant << 13);
    } else {
        bits = (sign << 31) | ((exp + 112) << 23) | (mant << 13);
    }
    float f = 0.0f;
    memcpy(&f, &bits, 4);
    return f;
}

// UDecVector4 10-10-10-2 packing: xyz 10-bit, w 2-bit. xyz decoded to
// [-1, 1]; the exact normal encoding is unvalidated against shipped data,
// so treat values as approximate until a skinned/normal-mapped mesh lands.
static Vector3 udecToVector3(quint32 v)
{
    Vector3 out;
    out.x = static_cast<float>((v & 0x3FF)) / 1023.0f * 2.0f - 1.0f;
    out.y = static_cast<float>(((v >> 10) & 0x3FF)) / 1023.0f * 2.0f - 1.0f;
    out.z = static_cast<float>(((v >> 20) & 0x3FF)) / 1023.0f * 2.0f - 1.0f;
    return out;
}

bool parseBsMeshData(const QByteArray& bytes, BsMeshData& out)
{
    out = BsMeshData();
    QDataStream s(bytes);
    s.setByteOrder(QDataStream::LittleEndian);
    const qint64 end = bytes.size();
    auto need = [&](qint64 n) -> bool {
        return s.status() == QDataStream::Ok && s.device()->pos() + n <= end;
    };
    // Counts are validated before use so a corrupt stream fails here
    // instead of over-allocating.
    auto countOk = [&](quint32 n, quint32 elem, quint32 maxElems) -> bool {
        return n <= maxElems
            && s.device()->pos() + static_cast<qint64>(n) * elem <= end;
    };

    quint32 version = 0, indexSize = 0;
    if (!need(8)) return false;
    s >> version >> indexSize;
    out.version = version;
    if (indexSize > 30000000u || indexSize % 3 != 0) return false;
    if (!countOk(indexSize / 3, 6, 10000000u)) return false;
    out.triangles.reserve(indexSize);
    for (quint32 i = 0; i < indexSize; ++i) {
        quint16 idx = 0;
        s >> idx;
        if (s.status() != QDataStream::Ok) return false;
        out.triangles.append(idx);
    }

    float scale = 1.0f;
    quint32 weightsPerVertex = 0, numVerts = 0;
    if (!need(12)) return false;
    {
        // NOTE: operator>>(float&) over-reads here (consumes 8, yields 0.0);
        // read the bits explicitly until the Qt/MSVC overload is understood.
        quint32 scaleBits = 0;
        s >> scaleBits;
        memcpy(&scale, &scaleBits, 4);
    }
    s >> weightsPerVertex >> numVerts;
    if (weightsPerVertex > 8) return false;
    out.vertexScale = scale;
    out.weightsPerVertex = weightsPerVertex;
    if (!countOk(numVerts, 6, 10000000u)) return false;
    out.vertices.reserve(numVerts);
    for (quint32 i = 0; i < numVerts; ++i) {
        qint16 x = 0, y = 0, z = 0;
        s >> x >> y >> z;
        if (s.status() != QDataStream::Ok) return false;
        Vector3 v;
        v.x = x * scale / 65536.0f; v.y = y * scale / 65536.0f; v.z = z * scale / 65536.0f;
        out.vertices.append(v);
    }

    quint32 num = 0;
    if (!need(4)) return false;
    s >> num;   // UVs
    if (!countOk(num, 4, 10000000u)) return false;
    for (quint32 i = 0; i < num; ++i) {
        quint16 u = 0, v = 0;
        s >> u >> v;
        if (s.status() != QDataStream::Ok) return false;
        Vector2 t;
        t.u = halfToFloat(u); t.v = halfToFloat(v);
        out.uvs.append(t);
    }
    if (!need(4)) return false;
    s >> num;   // UVs 2
    if (num > 0) {
        if (!countOk(num, 4, 10000000u)) return false;
        for (quint32 i = 0; i < num; ++i) {
            quint16 u = 0, v = 0;
            s >> u >> v;
            if (s.status() != QDataStream::Ok) return false;
            Vector2 t;
            t.u = halfToFloat(u); t.v = halfToFloat(v);
            out.uv2.append(t);
        }
    }
    if (!need(4)) return false;
    s >> num;   // colors (BGRA bytes)
    if (num > 0) {
        if (!countOk(num, 4, 10000000u)) return false;
        for (quint32 i = 0; i < num; ++i) {
            quint8 b = 0, g = 0, r = 0, a = 0;
            s >> b >> g >> r >> a;
            if (s.status() != QDataStream::Ok) return false;
            Color4 c;
            c.r = r / 255.0f; c.g = g / 255.0f; c.b = b / 255.0f; c.a = a / 255.0f;
            out.colors.append(c);
        }
    }
    if (!need(4)) return false;
    s >> num;   // normals
    if (!countOk(num, 4, 10000000u)) return false;
    for (quint32 i = 0; i < num; ++i) {
        quint32 packed = 0;
        s >> packed;
        if (s.status() != QDataStream::Ok) return false;
        out.normals.append(udecToVector3(packed));
    }
    if (!need(4)) return false;
    s >> num;   // tangents
    if (!countOk(num, 4, 10000000u)) return false;
    for (quint32 i = 0; i < num; ++i) {
        quint32 packed = 0;
        s >> packed;
        if (s.status() != QDataStream::Ok) return false;
        out.tangents.append(udecToVector3(packed));
    }
    if (!need(4)) return false;
    s >> num;   // bone weights
    if (!countOk(num, 4, 10000000u)) return false;
    for (quint32 i = 0; i < num; ++i) {
        BsMeshBoneWeight w;
        s >> w.bone >> w.weightRaw;
        if (s.status() != QDataStream::Ok) return false;
        out.weights.append(w);
    }
    if (out.version >= 1) {
        if (!need(4)) return false;
        s >> num;   // LODs
        for (quint32 i = 0; i < num; ++i) {
            quint32 lodSize = 0;
            if (!need(4)) return false;
            s >> lodSize;
            if (lodSize % 3 != 0 || !countOk(lodSize / 3, 6, 10000000u)) return false;
            for (quint32 k = 0; k < lodSize; ++k) {
                quint16 idx = 0;
                s >> idx;
                if (s.status() != QDataStream::Ok) return false;
            }
        }
    }
    // Trailing sections may be absent at EOF (shipped skinned meshes end
    // right after the LOD count): a missing count reads as zero.
    auto readCountOpt = [&](quint32& count) -> bool {
        if (s.device()->pos() == end) { count = 0; return true; }
        if (!need(4)) return false;
        s >> count;
        return s.status() == QDataStream::Ok;
    };
    if (!readCountOpt(num)) return false;   // meshlets (16 bytes each)
    if (!countOk(num, 16, 1000000u)) return false;
    for (quint32 i = 0; i < num; ++i) {
        quint32 a = 0, b = 0, c = 0, d = 0;
        s >> a >> b >> c >> d;
        if (s.status() != QDataStream::Ok) return false;
    }
    if (!readCountOpt(num)) return false;   // cull data (24 bytes each)
    if (!countOk(num, 24, 1000000u)) return false;
    for (quint32 i = 0; i < num; ++i) {
        for (int k = 0; k < 6; ++k) {
            quint32 bits = 0;
            s >> bits;
            if (s.status() != QDataStream::Ok) return false;
        }
    }

    if (s.status() != QDataStream::Ok) return false;
    if (s.device()->pos() != end) return false;   // exact consumption only
    if (!out.triangles.isEmpty()) {
        quint32 top = 0;
        for (quint32 idx : out.triangles)
            top = qMax(top, idx);
        if (top >= static_cast<quint32>(out.vertices.size())) return false;
    }
    return true;
}

static bool loadRealNif(NifParser& parser, const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open NIF file: %1").arg(fileName));
        return false;
    }

    quint32 version = 0;
    QByteArray fileHeader;
    if (!parseNifHeader(file, version, fileHeader)) {
        LOG_ERROR("Failed to parse NIF header");
        return false;
    }

    parser.setVersion(version);
    LOG_INFO(QString("Real NIF version: 0x%1").arg(version, 8, 16, QChar('0')));

    // Parse all blocks using nifrecord
    auto blocks = parseAllBlocks(file, version, fileHeader);
    file.close();

    LOG_INFO(QString("Parsed %1 NIF blocks").arg(blocks.size()));

    if (blocks.isEmpty()) {
        LOG_ERROR("No NIF blocks found");
        return false;
    }

    // Find root node - typically the first NiNode or the object with ref 0
    NifNode* rootNode = nullptr;
    for (auto it = blocks.constBegin(); it != blocks.constEnd(); ++it) {
        auto node = dynamic_cast<NifNode*>(it.value());
        if (node && it.key() == 0) {
            rootNode = node;
            break;
        }
    }

    if (!rootNode) {
        // Try to find any NiNode as root
        for (auto it = blocks.constBegin(); it != blocks.constEnd(); ++it) {
            auto node = dynamic_cast<NifNode*>(it.value());
            if (node) {
                rootNode = node;
                break;
            }
        }
    }

    if (!rootNode) {
        LOG_ERROR("No NiNode found in NIF file");
        return false;
    }

    // Create our model and extract geometry
    Node* ourRoot = new Node();
    ourRoot->name = QFileInfo(fileName).baseName();
    parser.setRoot(ourRoot);

    extractGeometry(blocks, rootNode, ourRoot);

    // Clean up nifrecord objects (but not the root node which we own)
    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (it.value() != rootNode) {
            delete it.value();
        }
    }
    delete rootNode;

    LOG_INFO(QString("Loaded %1 shapes, %2 vertices from real NIF")
                 .arg(parser.getRoot()->shapes.size())
                 .arg(parser.totalVertexCount()));

    return true;
}

// ---------------------------------------------------------------------------
// NifParser
// ---------------------------------------------------------------------------

static bool loadRealNifGamebryo(NifParser& parser, const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open NIF file: %1").arg(fileName));
        return false;
    }

    Gamebryo::Header header;
    if (!Gamebryo::parseHeader(file, header)) {
        LOG_ERROR(QString("Not a supported Gamebryo NIF: %1").arg(fileName));
        return false;
    }

    // Index-based dispatch: every block is seekable by its declared size,
    // so unknown blocks are skipped exactly and known ones parse in place.
    QMap<quint32, NifObject*> blocks;
    QStringList externalMeshes;
    auto fail = [&]() {
        qDeleteAll(blocks);
        return false;
    };
    for (quint32 i = 0; i < header.numBlocks; ++i) {
        const QString& type = header.blockTypes.at(header.typeIndex.at(i));
        NifObject* obj = nullptr;
        if (type == QLatin1String("NiNode")) {
            qint64 consumed = 0;
            obj = Gamebryo::parseNode(file, header, i, consumed);
            if (!obj || consumed != static_cast<qint64>(header.blockSize[i]))
                { delete obj; obj = new NifObject(); }
            else
                obj->className = QStringLiteral("NiNode");
        } else if (type == QLatin1String("BSFaceGenNiNode")) {
            qint64 consumed = 0;
            obj = Gamebryo::parseFaceGenNode(file, header, i, consumed);
            if (!obj || consumed != static_cast<qint64>(header.blockSize[i]))
                { delete obj; obj = new NifObject(); }
            // className already "BSFaceGenNiNode" on success.
        } else if (type == QLatin1String("BSGeometry")) {
            qint64 consumed = 0;
            obj = Gamebryo::parseGeometry(file, header, i, externalMeshes, consumed);
            if (!obj || consumed != static_cast<qint64>(header.blockSize[i]))
                { delete obj; obj = new NifObject(); }
            // className already "BSGeometry" on success.
        } else if (type == QLatin1String("BSSkin::Instance")) {
            qint64 consumed = 0;
            obj = Gamebryo::parseSkinInstance(file, header, i, consumed);
            if (!obj || consumed != static_cast<qint64>(header.blockSize[i]))
                { delete obj; obj = new NifObject(); }
        } else if (type == QLatin1String("BSSkin::BoneData")) {
            qint64 consumed = 0;
            obj = Gamebryo::parseSkinBoneData(file, header, i, consumed);
            if (!obj || consumed != static_cast<qint64>(header.blockSize[i]))
                { delete obj; obj = new NifObject(); }
        } else if (type == QLatin1String("SkinAttach")) {
            qint64 consumed = 0;
            obj = Gamebryo::parseSkinAttach(file, header, i, consumed);
            if (!obj || consumed != static_cast<qint64>(header.blockSize[i]))
                { delete obj; obj = new NifObject(); }
        } else {
            obj = new NifObject();
        }
        obj->dataRef = i;
        if (obj->className.isEmpty())
            obj->className = type;
        blocks.insert(i, obj);
    }
    file.close();

    LOG_INFO(QString("Parsed %1 Gamebryo blocks (%2 external meshes)")
                 .arg(blocks.size()).arg(externalMeshes.size()));

    // Resolve external meshes: decode each shell's first resolvable slot
    // into a synthetic data block so extractGeometry picks up real
    // vertices. Unresolvable shells stay vert-less (extractGeometry skips
    // them); the load still succeeds on hierarchy + paths.
    if (!externalMeshes.isEmpty()) {
        MeshArchiveResolver resolver = MeshArchiveResolver::forNif(fileName);
        if (resolver.hasArchives()) {
            int attached = 0;
            for (auto it = blocks.begin(); it != blocks.end(); ++it) {
                auto* shell = dynamic_cast<NifTriShape*>(it.value());
                if (!shell || shell->externalMeshPaths.isEmpty()) continue;
                for (const QString& mp : shell->externalMeshPaths) {
                    const QByteArray mbytes = resolver.meshBytes(mp);
                    if (mbytes.isEmpty()) continue;
                    BsMeshData md;
                    if (!parseBsMeshData(mbytes, md) || md.vertices.isEmpty()) continue;
                    auto* data = new NifTriShapeData();
                    data->className = QStringLiteral("NiTriShapeData");
                    for (const Vector3& v : md.vertices) {
                        NiPoint3 p; p.x = v.x; p.y = v.y; p.z = v.z;
                        data->vertices.append(p);
                    }
                    for (const Vector2& t : md.uvs) {
                        NiPoint2 p; p.u = t.u; p.v = t.v;
                        data->uvs.append(p);
                    }
                    for (const Vector3& n : md.normals) {
                        NiPoint3 p; p.x = n.x; p.y = n.y; p.z = n.z;
                        data->normals.append(p);
                    }
                    for (const Color4& c : md.colors) {
                        NiColorRGBA col; col.r = c.r; col.g = c.g; col.b = c.b; col.a = c.a;
                        data->vertexColors.append(col);
                    }
                    for (quint32 idx : md.triangles)
                        data->indices.append(idx);
                    // Per-vertex skin weights survive on the synthetic block
                    // so the BSSkin triplet link can pair them with the
                    // attach's bone names (indices are attach-local: verified
                    // maxBone == attachBones-1 on every shipped face shape).
                    data->skinWeightsPerVertex = md.weightsPerVertex;
                    for (const BsMeshBoneWeight& w : md.weights) {
                        data->skinBoneIndices.append(w.bone);
                        data->skinBoneWeights.append(w.weightRaw);
                    }
                    const quint32 key = 0x40000000u | it.key();
                    blocks.insert(key, data);
                    shell->refGeometryData = key;
                    ++attached;
                    break;   // first resolvable slot wins
                }
            }
            LOG_INFO(QString("Attached %1 external mesh data blocks").arg(attached));
        }
    }

    auto* rootNode = dynamic_cast<NifNode*>(blocks.value(0));
    if (!rootNode) {
        LOG_ERROR("No NiNode root block in Gamebryo NIF");
        return fail();
    }

    Node* ourRoot = new Node();
    ourRoot->name = QFileInfo(fileName).baseName();
    parser.setRoot(ourRoot);
    parser.setVersion(0x140200);   // display "20.2.0" like the dialect
    parser.setExternalMeshRefs(externalMeshes);

    extractGeometry(blocks, rootNode, ourRoot);

    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (it.value() != rootNode)
            delete it.value();
    }
    delete rootNode;

    LOG_INFO(QString("Loaded Gamebryo hierarchy: %1 nodes, %2 shapes, %3 vertices, %4 external meshes")
                 .arg(ourRoot->children.size())
                 .arg(parser.shapeCount())
                 .arg(parser.totalVertexCount())
                 .arg(externalMeshes.size()));
    return true;
}

NifParser::~NifParser()
{
    delete root;
}

QString NifParser::getVersionString() const
{
    if (m_version == 0) return "Unknown";
    quint8 major = (m_version >> 16) & 0xFF;
    quint8 minor = (m_version >> 8) & 0xFF;
    quint8 patch = m_version & 0xFF;
    return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
}

bool NifParser::load(const QString& fileName)
{
    LOG_INFO(QString("Loading NIF file: %1").arg(fileName));

    // Route by magic: real Gamebryo binaries go to the index-based reader,
    // everything else keeps the dialect path.
    {
        QFile probe(fileName);
        if (probe.open(QIODevice::ReadOnly)) {
            const QByteArray head = probe.read(8);
            probe.close();
            if (head == "Gamebryo") {
                m_externalMeshes.clear();
                if (loadRealNifGamebryo(*this, fileName)) {
                    LOG_INFO("NIF file loaded successfully (Gamebryo 20.2.0.7)");
                    return true;
                }
                LOG_INFO("Gamebryo parse rejected the file; trying dialect path");
            }
        }
    }

    // Try real Bethesda NIF parser first (nifrecord-based)
    if (loadRealNif(*this, fileName)) {
        LOG_INFO(QString("Loaded %1 shapes, %2 vertices from real NIF")
                     .arg(root->shapes.size())
                     .arg(totalVertexCount()));
        LOG_INFO("NIF file loaded successfully (real Bethesda format)");
        return true;
    }

    // Fall back to simplified internal format
    root = nullptr;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open NIF file: %1").arg(fileName));
        return false;
    }

    QByteArray rawData = file.readAll();
    file.close();

    if (rawData.size() < 12) {
        LOG_ERROR("NIF file too small");
        return false;
    }

    QDataStream stream(&rawData, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    if (!parseHeader(stream)) {
        return false;
    }

    quint32 numShapes = 0;
    stream >> numShapes;

    root = new Node();
    root->name = QFileInfo(fileName).baseName();

    for (quint32 s = 0; s < numShapes; ++s) {
        TriShape shape;
        if (!readShape(stream, shape)) {
            LOG_ERROR(QString("Failed to read shape %1").arg(s));
            delete root;
            root = nullptr;
            return false;
        }
        root->shapes.append(shape);
    }

    LOG_INFO(QString("Loaded %1 shapes, %2 vertices")
                 .arg(root->shapes.size())
                 .arg(totalVertexCount()));
    LOG_INFO("NIF file loaded successfully (simplified format)");
    return true;
}

bool NifParser::parseHeader(QDataStream& stream)
{
    quint32 magic = 0;
    stream >> magic;
    if (magic != 0x46494E4E) {
        LOG_ERROR("Not a valid NIF file (invalid magic number)");
        return false;
    }

    quint32 version = 0;
    stream >> version;
    LOG_INFO(QString("NIF version: %1").arg(version));

    quint32 fileNameLength = 0;
    stream >> fileNameLength;
    if (fileNameLength > 1024) {
        LOG_ERROR("NIF file name length unreasonable");
        return false;
    }

    QByteArray fileNameBytes(fileNameLength, 0);
    stream.readRawData(fileNameBytes.data(), fileNameLength);
    Q_UNUSED(fileNameBytes);

    return true;
}

bool NifParser::readShape(QDataStream& stream, TriShape& shape)
{
    quint32 nameLength = 0;
    stream >> nameLength;
    if (nameLength > 1024) return false;
    QByteArray nameBytes(nameLength, 0);
    stream.readRawData(nameBytes.data(), nameLength);
    shape.name = QString::fromLatin1(nameBytes);

    quint32 numVertices = 0;
    stream >> numVertices;
    if (numVertices > 10'000'000) return false;

    for (quint32 v = 0; v < numVertices; ++v) {
        Vector3 vert;
        Vector2 uv;
        Color4 col;
        stream >> vert.x >> vert.y >> vert.z;
        stream >> uv.u >> uv.v;
        stream >> col.r >> col.g >> col.b >> col.a;
        shape.vertices.append(vert);
        shape.uvs.append(uv);
        shape.colors.append(col);
    }

    quint32 numIndices = 0;
    stream >> numIndices;
    if (numIndices > 30'000'000) return false;
    shape.indices.resize(static_cast<int>(numIndices));
    for (quint32 i = 0; i < numIndices; ++i) {
        stream >> shape.indices[i];
    }

    // Material
    stream >> shape.baseColor.r >> shape.baseColor.g
           >> shape.baseColor.b >> shape.baseColor.a;
    quint32 texLen = 0;
    stream >> texLen;
    if (texLen > 0 && texLen < 4096) {
        QByteArray texBytes(texLen, 0);
        stream.readRawData(texBytes.data(), texLen);
        shape.texture = QString::fromUtf8(texBytes);
    }

    return true;
}

bool NifParser::save(const QString& fileName) const
{
    if (!root) {
        LOG_ERROR("Cannot save NIF: no data loaded");
        return false;
    }

    LOG_INFO(QString("Saving NIF file: %1").arg(fileName));

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open NIF file for writing: %1").arg(fileName));
        return false;
    }

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Check if full format is needed (LOD or billboard nodes present)
    bool needsFullFormat = root->isLODNode || root->isBillboardNode;
    if (!needsFullFormat) {
        for (const auto* child : root->children) {
            if (child->isLODNode || child->isBillboardNode) {
                needsFullFormat = true;
                break;
            }
        }
    }

    if (needsFullFormat) {
        // Write header with full format version marker
        stream << static_cast<quint32>(0x46494E4E);
        stream << static_cast<quint32>(0x14020008);

        QByteArray nameBytes = QFileInfo(fileName).fileName().toLatin1();
        stream << static_cast<quint32>(nameBytes.size());
        stream.writeRawData(nameBytes.constData(), nameBytes.size());

        // Write full node tree
        writeNodeTree(stream, root);

        file.write(data);
        file.close();

        LOG_INFO(QString("Saved NIF (full format): %1 shapes").arg(totalVertexCount()));
        return true;
    }

    // Simplified format (shapes only)
    if (!writeHeader(stream, fileName)) {
        file.close();
        return false;
    }

    stream << static_cast<quint32>(root->shapes.size());
    for (const auto& shape : root->shapes) {
        writeShape(stream, shape);
    }

    file.write(data);
    file.close();

    LOG_INFO(QString("Saved NIF: %1 shapes").arg(root->shapes.size()));
    return true;
}

bool NifParser::writeHeader(QDataStream& stream, const QString& fileName) const
{
    stream << static_cast<quint32>(0x46494E4E);
    stream << static_cast<quint32>(0x14020007); // NIF version marker

    QByteArray nameBytes = QFileInfo(fileName).fileName().toLatin1();
    stream << static_cast<quint32>(nameBytes.size());
    stream.writeRawData(nameBytes.constData(), nameBytes.size());
    return true;
}

void NifParser::writeShape(QDataStream& stream, const TriShape& shape) const
{
    QByteArray nameBytes = shape.name.toLatin1();
    stream << static_cast<quint32>(nameBytes.size());
    stream.writeRawData(nameBytes.constData(), nameBytes.size());

    stream << static_cast<quint32>(shape.vertices.size());
    for (int i = 0; i < shape.vertices.size(); ++i) {
        const Vector3& vert = shape.vertices[i];
        Vector2 uv = (i < shape.uvs.size()) ? shape.uvs[i] : Vector2{0.0f, 0.0f};
        Color4 col = (i < shape.colors.size())
                         ? shape.colors[i]
                         : Color4{1.0f, 1.0f, 1.0f, 1.0f};
        stream << vert.x << vert.y << vert.z;
        stream << uv.u << uv.v;
        stream << col.r << col.g << col.b << col.a;
    }

    stream << static_cast<quint32>(shape.indices.size());
    for (quint32 idx : shape.indices) {
        stream << idx;
    }

    // Material
    stream << shape.baseColor.r << shape.baseColor.g
           << shape.baseColor.b << shape.baseColor.a;
    QByteArray texBytes = shape.texture.toUtf8();
    stream << static_cast<quint32>(texBytes.size());
    if (!texBytes.isEmpty()) {
        stream.writeRawData(texBytes.constData(), texBytes.size());
    }
}

void NifParser::writeNodeTree(QDataStream& stream, const Node* node) const
{
    // Node flags
    stream << static_cast<quint8>(node->isLODNode ? 1 : 0);
    stream << static_cast<quint8>(node->isBillboardNode ? 1 : 0);

    if (node->isBillboardNode) {
        stream << static_cast<quint32>(node->billboardMode);
    }

    if (node->isLODNode) {
        quint32 numScreens = static_cast<quint32>(node->lodMinScreens.size());
        stream << numScreens;
        for (quint32 i = 0; i < numScreens; ++i) {
            stream << node->lodMinScreens[i];
            stream << node->lodMaxScreens[i];
        }
    }

    // Shapes
    stream << static_cast<quint32>(node->shapes.size());
    for (const auto& shape : node->shapes) {
        writeShape(stream, shape);
    }

    // Children
    stream << static_cast<quint32>(node->children.size());
    for (const auto* child : node->children) {
        writeNodeTree(stream, child);
    }
}

void NifParser::translateAll(float dx, float dy, float dz)
{
    if (!root) return;
    for (auto& shape : root->shapes) {
        shape.translate(dx, dy, dz);
    }
}

void NifParser::scaleAll(float factor)
{
    if (!root) return;
    for (auto& shape : root->shapes) {
        shape.scale(factor);
    }
}

int NifParser::totalVertexCount() const
{
    if (!root) return 0;
    int count = 0;
    QStack<const Node*> stack;
    stack.push(root);
    while (!stack.isEmpty()) {
        const Node* node = stack.pop();
        for (const auto& shape : node->shapes)
            count += shape.vertices.size();
        for (const Node* child : node->children)
            stack.push(child);
    }
    return count;
}

int NifParser::shapeCount() const
{
    if (!root) return 0;
    int count = 0;
    QStack<const Node*> stack;
    stack.push(root);
    while (!stack.isEmpty()) {
        const Node* node = stack.pop();
        count += node->shapes.size();
        for (const Node* child : node->children)
            stack.push(child);
    }
    return count;
}

// ---------------------------------------------------------------------------
// Animation interpolation
// ---------------------------------------------------------------------------

static void slerpQuat(const QuaternionKeyframe& q0, const QuaternionKeyframe& q1,
                      float t, QuaternionKeyframe& result)
{
    float dot = q0.w * q1.w + q0.x * q1.x + q0.y * q1.y + q0.z * q1.z;

    QuaternionKeyframe q1a = q1;
    if (dot < 0.0f) {
        dot = -dot;
        q1a.w = -q1a.w;
        q1a.x = -q1a.x;
        q1a.y = -q1a.y;
        q1a.z = -q1a.z;
    }

    if (dot > 0.9995f) {
        result.w = q0.w + t * (q1a.w - q0.w);
        result.x = q0.x + t * (q1a.x - q0.x);
        result.y = q0.y + t * (q1a.y - q0.y);
        result.z = q0.z + t * (q1a.z - q0.z);
    } else {
        float theta = std::acos(dot);
        float sinTheta = std::sin(theta);
        float w0 = std::sin((1.0f - t) * theta) / sinTheta;
        float w1 = std::sin(t * theta) / sinTheta;
        result.w = w0 * q0.w + w1 * q1a.w;
        result.x = w0 * q0.x + w1 * q1a.x;
        result.y = w0 * q0.y + w1 * q1a.y;
        result.z = w0 * q0.z + w1 * q1a.z;
    }
}

TransformKeyframe Node::getInterpolatedFrame(float time) const
{
    TransformKeyframe result;
    result.time = time;
    result.translation = {0.0f, 0.0f, 0.0f};
    result.rotation = {1.0f, 0.0f, 0.0f, 0.0f};
    result.scale = {1.0f, 1.0f, 1.0f};

    if (animations.isEmpty()) return result;

    const auto& keyframes = animations[0].keyframes;
    if (keyframes.isEmpty()) return result;
    if (keyframes.size() == 1) return keyframes[0];

    if (time <= keyframes[0].time) return keyframes[0];
    if (time >= keyframes.last().time) return keyframes.last();

    for (int i = 0; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            const auto& k0 = keyframes[i];
            const auto& k1 = keyframes[i + 1];

            float dt = k1.time - k0.time;
            float t = (dt > 0.0f) ? (time - k0.time) / dt : 0.0f;

            result.translation.x = k0.translation.x + t * (k1.translation.x - k0.translation.x);
            result.translation.y = k0.translation.y + t * (k1.translation.y - k0.translation.y);
            result.translation.z = k0.translation.z + t * (k1.translation.z - k0.translation.z);

            slerpQuat(k0.rotation, k1.rotation, t, result.rotation);

            result.scale.x = k0.scale.x + t * (k1.scale.x - k0.scale.x);
            result.scale.y = k0.scale.y + t * (k1.scale.y - k0.scale.y);
            result.scale.z = k0.scale.z + t * (k1.scale.z - k0.scale.z);

            break;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Animation metadata helpers
// ---------------------------------------------------------------------------

static void collectAnimations(const Node* node, QVector<NiKeyframeController>& out)
{
    if (!node) return;
    for (const auto& anim : node->animations) {
        out.append(anim);
    }
    for (const auto* child : node->children) {
        collectAnimations(child, out);
    }
}

static float maxAnimDuration(const Node* node)
{
    if (!node) return 0.0f;
    float maxDur = 0.0f;
    for (const auto& anim : node->animations) {
        for (const auto& kf : anim.keyframes) {
            if (kf.time > maxDur) maxDur = kf.time;
        }
    }
    for (const auto* child : node->children) {
        float childDur = maxAnimDuration(child);
        if (childDur > maxDur) maxDur = childDur;
    }
    return maxDur;
}

static void collectClipNames(const Node* node, QVector<QString>& out)
{
    if (!node) return;
    for (const auto& anim : node->animations) {
        if (!anim.clipName.isEmpty() && !out.contains(anim.clipName)) {
            out.append(anim.clipName);
        }
    }
    for (const auto* child : node->children) {
        collectClipNames(child, out);
    }
}

float NifParser::getAnimationDuration() const
{
    return maxAnimDuration(root);
}

int NifParser::getAnimationClipCount() const
{
    QVector<QString> names;
    collectClipNames(root, names);
    return names.size();
}

QString NifParser::getAnimationClipName(int index) const
{
    QVector<QString> names;
    collectClipNames(root, names);
    if (index >= 0 && index < names.size()) return names[index];
    return QString();
}

QVector<NiKeyframeController> NifParser::getAllAnimationControllers() const
{
    QVector<NiKeyframeController> result;
    collectAnimations(root, result);
    return result;
}

QVector<AnimationClip> NifParser::getAnimClips() const
{
    QVector<AnimationClip> result;
    if (!root) return result;

    QVector<NiKeyframeController> controllers = getAllAnimationControllers();
    QMap<QString, AnimationClip> clipMap;

    for (const auto& ctrl : controllers) {
        QString clipName = ctrl.clipName.isEmpty() ? "Default" : ctrl.clipName;

        if (!clipMap.contains(clipName)) {
            AnimationClip clip;
            clip.name = clipName;
            clip.startTime = 0;
            clip.endTime = 0;
            clipMap[clipName] = clip;
        }

        AnimationClip& clip = clipMap[clipName];

        for (const auto& tk : ctrl.keyframes) {
            clip.keyframes.append(tk);

            if (tk.time > clip.endTime) clip.endTime = tk.time;
        }
    }

    result = clipMap.values();
    return result;
}

} // namespace Nif

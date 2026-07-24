#include "nifparser.hpp"

#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QBuffer>
#include <QDebug>
#include <cmath>
#include <QMap>
#include <QStack>

#include "logger.hpp"
#include "nifrecord.hpp"

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
        } else if (className.startsWith("NiTriShape")) {
            obj = new NifTriShape();
        } else if (className.startsWith("NiTriShapeData") || className.startsWith("NiBasedGeomData")) {
            obj = new NifTriShapeData();
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
        obj->parse(device, version, fileHeader);

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
    for (const auto& shape : root->shapes) {
        count += shape.vertices.size();
    }
    return count;
}

int NifParser::shapeCount() const
{
    return root ? root->shapes.size() : 0;
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

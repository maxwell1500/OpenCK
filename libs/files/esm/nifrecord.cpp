#include "nifrecord.hpp"
#include <QDataStream>
#include <QIODevice>

NiPoint3 NiPoint3::read(QIODevice& device)
{
    NiPoint3 point;
    float values[3];
    device.read(reinterpret_cast<char*>(values), sizeof(values));
    point.x = values[0];
    point.y = values[1];
    point.z = values[2];
    return point;
}

void NiPoint3::write(QIODevice& device) const
{
    float values[3] = {x, y, z};
    device.write(reinterpret_cast<const char*>(values), sizeof(values));
}

NiPoint2 NiPoint2::read(QIODevice& device)
{
    NiPoint2 point;
    float values[2];
    device.read(reinterpret_cast<char*>(values), sizeof(values));
    point.u = values[0];
    point.v = values[1];
    return point;
}

void NiPoint2::write(QIODevice& device) const
{
    float values[2] = {u, v};
    device.write(reinterpret_cast<const char*>(values), sizeof(values));
}

NiPoint4 NiPoint4::read(QIODevice& device)
{
    NiPoint4 point;
    float values[4];
    device.read(reinterpret_cast<char*>(values), sizeof(values));
    point.x = values[0];
    point.y = values[1];
    point.z = values[2];
    point.w = values[3];
    return point;
}

void NiPoint4::write(QIODevice& device) const
{
    float values[4] = {x, y, z, w};
    device.write(reinterpret_cast<const char*>(values), sizeof(values));
}

NiColorRGBA NiColorRGBA::read(QIODevice& device)
{
    NiColorRGBA color;
    float values[4];
    device.read(reinterpret_cast<char*>(values), sizeof(values));
    color.r = values[0];
    color.g = values[1];
    color.b = values[2];
    color.a = values[3];
    return color;
}

void NiColorRGBA::write(QIODevice& device) const
{
    float values[4] = {r, g, b, a};
    device.write(reinterpret_cast<const char*>(values), sizeof(values));
}

NiQuat NiQuat::read(QIODevice& device)
{
    NiQuat quat;
    float values[4];
    device.read(reinterpret_cast<char*>(values), sizeof(values));
    quat.w = values[0];
    quat.x = values[1];
    quat.y = values[2];
    quat.z = values[3];
    return quat;
}

void NiQuat::write(QIODevice& device) const
{
    float values[4] = {w, x, y, z};
    device.write(reinterpret_cast<const char*>(values), sizeof(values));
}

quint32 NifObject::readRef(QIODevice& device)
{
    quint32 ref = 0;
    device.read(reinterpret_cast<char*>(&ref), sizeof(ref));
    return ref;
}

void NifObject::writeRef(QIODevice& device, quint32 ref) const
{
    device.write(reinterpret_cast<const char*>(&ref), sizeof(ref));
}

void NifObject::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    Q_UNUSED(fileHeader)
    
    // Block header (className, dataRef) already read by parseAllBlocks()
    
    QByteArray paddingBytes;
    if (version >= 0x0A000000)
    {
        quint32 paddingLength = 0;
        device.read(reinterpret_cast<char*>(&paddingLength), sizeof(paddingLength));
        paddingBytes.resize(paddingLength);
        device.read(paddingBytes.data(), paddingLength);
    }
    padding = paddingBytes;
}

void NifObject::write(QIODevice& device, quint32 version) const
{
    QByteArray classNameBytes = className.toLatin1();
    quint32 classNameLength = classNameBytes.length();
    device.write(reinterpret_cast<const char*>(&classNameLength), sizeof(classNameLength));
    device.write(classNameBytes);

    writeRef(device, dataRef);

    if (version >= 0x0A000000)
    {
        quint32 paddingLength = padding.length();
        device.write(reinterpret_cast<const char*>(&paddingLength), sizeof(paddingLength));
        device.write(padding);
    }
}

void NifAVObject::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    QByteArray nameBytes;
    quint32 nameLength = 0;
    device.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
    nameBytes.resize(nameLength);
    device.read(nameBytes.data(), nameLength);
    name = QString::fromLatin1(nameBytes);

    refParent = readRef(device);
    refChildren = readRef(device);
    refExtraData = readRef(device);
    refControllers = readRef(device);

    parseChildren(device, version);
}

void NifAVObject::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    QByteArray nameBytes = name.toLatin1();
    quint32 nameLength = nameBytes.length();
    device.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
    device.write(nameBytes);

    writeRef(device, refParent);
    writeRef(device, refChildren);
    writeRef(device, refExtraData);
    writeRef(device, refControllers);

    writeChildren(device, version);
}

void NifAVObject::parseChildren(QIODevice& device, quint32 version)
{
    quint32 childrenCount = 0;
    device.read(reinterpret_cast<char*>(&childrenCount), sizeof(childrenCount));

    children.clear();
    children.resize(childrenCount);
    for (quint32 i = 0; i < childrenCount; i++)
    {
        children[i] = readRef(device);
    }

    quint32 controllersCount = 0;
    device.read(reinterpret_cast<char*>(&controllersCount), sizeof(controllersCount));

    controllers.clear();
    controllers.resize(controllersCount);
    for (quint32 i = 0; i < controllersCount; i++)
    {
        controllers[i] = readRef(device);
    }
}

void NifAVObject::writeChildren(QIODevice& device, quint32 version) const
{
    quint32 childrenCount = children.length();
    device.write(reinterpret_cast<const char*>(&childrenCount), sizeof(childrenCount));
    for (quint32 i = 0; i < childrenCount; i++)
    {
        writeRef(device, children[i]);
    }

    quint32 controllersCount = controllers.length();
    device.write(reinterpret_cast<const char*>(&controllersCount), sizeof(controllersCount));
    for (quint32 i = 0; i < controllersCount; i++)
    {
        writeRef(device, controllers[i]);
    }
}

void NifNode::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifAVObject::parse(device, version, fileHeader);

    keepHierarchy = true;
    hasRotations = true;
    hasTranslations = true;
    hasScales = true;
    hasScaleKeys = false;
    hasBSPNode = false;
    hasBSPGeometry = true;

    if (version >= 0x14000000)
    {
        keepHierarchy = true;
        hasRotations = true;
        hasTranslations = true;
        hasScales = true;
        hasScaleKeys = false;
        hasBSPNode = false;
        hasBSPGeometry = true;
    }

    hasBoundingSphere = true;
    boundingSphereRadius = 0.0f;
    boundingSphereCenter = {};

    if (hasBoundingSphere)
    {
        boundingSphereCenter = NiPoint3::read(device);
        device.read(reinterpret_cast<char*>(&boundingSphereRadius), sizeof(boundingSphereRadius));
    }

    hasBoundingCylinder = false;
    boundingCylinderRadius = 0.0f;
    boundingCylinderLength = 0.0f;
    boundingCylinderAxis = {};
    boundingCylinderHeight = 0.0f;
    collisionLevel = 0.0f;
}

void NifNode::write(QIODevice& device, quint32 version) const
{
    NifAVObject::write(device, version);

    if (version >= 0x14000000)
    {
        // Write version-specific data
    }

    if (hasBoundingSphere)
    {
        boundingSphereCenter.write(device);
        device.write(reinterpret_cast<const char*>(&boundingSphereRadius), sizeof(boundingSphereRadius));
    }
}

void NifTriBasedGeom::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifAVObject::parse(device, version, fileHeader);

    QByteArray geomDataNameBytes;
    quint32 geomDataNameLength = 0;
    device.read(reinterpret_cast<char*>(&geomDataNameLength), sizeof(geomDataNameLength));
    geomDataNameBytes.resize(geomDataNameLength);
    device.read(geomDataNameBytes.data(), geomDataNameLength);
    geometryDataName = QString::fromLatin1(geomDataNameBytes);

    refGeometryData = readRef(device);
}

void NifTriBasedGeom::write(QIODevice& device, quint32 version) const
{
    NifAVObject::write(device, version);

    QByteArray geomDataNameBytes = geometryDataName.toLatin1();
    quint32 geomDataNameLength = geomDataNameBytes.length();
    device.write(reinterpret_cast<const char*>(&geomDataNameLength), sizeof(geomDataNameLength));
    device.write(geomDataNameBytes);

    writeRef(device, refGeometryData);
}

void NifTriShape::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifTriBasedGeom::parse(device, version, fileHeader);

    hasAlpha = true;
    hasMaterial = true;
    hasTexture = true;
    hasNormals = true;
    hasUVs = true;
    hasTangents = true;
    hasVertexColors = false;

    if (version >= 0x14000000)
    {
        hasAlpha = true;
        hasMaterial = true;
        hasTexture = true;
        hasNormals = true;
        hasUVs = true;
        hasTangents = true;
        hasVertexColors = false;
    }
}

void NifTriShape::write(QIODevice& device, quint32 version) const
{
    NifTriBasedGeom::write(device, version);

    if (version >= 0x14000000)
    {
        // Write version-specific data
    }
}

void NifTriShapeData::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    parseVertices(device, version);
    parseUVs(device, version);
    parseNormals(device, version);
    parseTangents(device, version);
    parseVertexColors(device, version);
    parseIndices(device, version);
}

void NifTriShapeData::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    // Write vertices
    quint32 vertexCount = vertices.length();
    device.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
    for (const auto& vertex : vertices)
    {
        vertex.write(device);
    }

    // Write UVs
    quint32 uvCount = uvs.length();
    device.write(reinterpret_cast<const char*>(&uvCount), sizeof(uvCount));
    for (const auto& uv : uvs)
    {
        uv.write(device);
    }

    // Write normals
    quint32 normalCount = normals.length();
    device.write(reinterpret_cast<const char*>(&normalCount), sizeof(normalCount));
    for (const auto& normal : normals)
    {
        normal.write(device);
    }

    // Write tangents
    quint32 tangentCount = tangents.length();
    device.write(reinterpret_cast<const char*>(&tangentCount), sizeof(tangentCount));
    for (const auto& tangent : tangents)
    {
        tangent.write(device);
    }

    // Write vertex colors
    quint32 colorCount = vertexColors.length();
    device.write(reinterpret_cast<const char*>(&colorCount), sizeof(colorCount));
    for (const auto& color : vertexColors)
    {
        color.write(device);
    }

    // Write indices
    quint32 indexCount = indices.length();
    device.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));
    for (quint32 index : indices)
    {
        device.write(reinterpret_cast<const char*>(&index), sizeof(index));
    }
}

void NifTriShapeData::parseVertices(QIODevice& device, quint32 version)
{
    Q_UNUSED(version)

    quint32 vertexCount = 0;
    device.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));

    vertices.clear();
    vertices.resize(vertexCount);
    for (quint32 i = 0; i < vertexCount; i++)
    {
        vertices[i] = NiPoint3::read(device);
    }
}

void NifTriShapeData::parseUVs(QIODevice& device, quint32 version)
{
    Q_UNUSED(version)

    quint32 uvCount = 0;
    device.read(reinterpret_cast<char*>(&uvCount), sizeof(uvCount));

    uvs.clear();
    uvs.resize(uvCount);
    for (quint32 i = 0; i < uvCount; i++)
    {
        uvs[i] = NiPoint2::read(device);
    }
}

void NifTriShapeData::parseNormals(QIODevice& device, quint32 version)
{
    Q_UNUSED(version)

    quint32 normalCount = 0;
    device.read(reinterpret_cast<char*>(&normalCount), sizeof(normalCount));

    normals.clear();
    normals.resize(normalCount);
    for (quint32 i = 0; i < normalCount; i++)
    {
        normals[i] = NiPoint3::read(device);
    }
}

void NifTriShapeData::parseTangents(QIODevice& device, quint32 version)
{
    Q_UNUSED(version)

    quint32 tangentCount = 0;
    device.read(reinterpret_cast<char*>(&tangentCount), sizeof(tangentCount));

    tangents.clear();
    tangents.resize(tangentCount);
    for (quint32 i = 0; i < tangentCount; i++)
    {
        tangents[i] = NiPoint4::read(device);
    }
}

void NifTriShapeData::parseVertexColors(QIODevice& device, quint32 version)
{
    Q_UNUSED(version)

    quint32 colorCount = 0;
    device.read(reinterpret_cast<char*>(&colorCount), sizeof(colorCount));

    vertexColors.clear();
    vertexColors.resize(colorCount);
    for (quint32 i = 0; i < colorCount; i++)
    {
        vertexColors[i] = NiColorRGBA::read(device);
    }
}

void NifTriShapeData::parseIndices(QIODevice& device, quint32 version)
{
    Q_UNUSED(version)

    quint32 indexCount = 0;
    device.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));

    indices.clear();
    indices.resize(indexCount);
    for (quint32 i = 0; i < indexCount; i++)
    {
        device.read(reinterpret_cast<char*>(&indices[i]), sizeof(indices[i]));
    }
}

void NifTexture::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    QByteArray textureNameBytes;
    quint32 textureNameLength = 0;
    device.read(reinterpret_cast<char*>(&textureNameLength), sizeof(textureNameLength));
    textureNameBytes.resize(textureNameLength);
    device.read(textureNameBytes.data(), textureNameLength);
    textureName = QString::fromLatin1(textureNameBytes);

    refTextureData = readRef(device);

    if (version >= 0x14000000)
    {
        QByteArray textureFileBytes;
        quint32 textureFileLength = 0;
        device.read(reinterpret_cast<char*>(&textureFileLength), sizeof(textureFileLength));
        textureFileBytes.resize(textureFileLength);
        device.read(textureFileBytes.data(), textureFileLength);
        textureFile = QString::fromLatin1(textureFileBytes);
    }
}

void NifTexture::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    QByteArray textureNameBytes = textureName.toLatin1();
    quint32 textureNameLength = textureNameBytes.length();
    device.write(reinterpret_cast<const char*>(&textureNameLength), sizeof(textureNameLength));
    device.write(textureNameBytes);

    writeRef(device, refTextureData);

    if (version >= 0x14000000)
    {
        QByteArray textureFileBytes = textureFile.toLatin1();
        quint32 textureFileLength = textureFileBytes.length();
        device.write(reinterpret_cast<const char*>(&textureFileLength), sizeof(textureFileLength));
        device.write(textureFileBytes);
    }
}

void NifMaterial::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    ambientColor = NiColorRGBA::read(device);
    diffuseColor = NiColorRGBA::read(device);
    specularColor = NiColorRGBA::read(device);

    device.read(reinterpret_cast<char*>(&specularExponent), sizeof(specularExponent));
    device.read(reinterpret_cast<char*>(&emission), sizeof(emission));
    device.read(reinterpret_cast<char*>(&reflectivity), sizeof(reflectivity));
    device.read(reinterpret_cast<char*>(&opacity), sizeof(opacity));

    hasDiffuseTexture = false;
    hasSpecularTexture = false;
    hasBumpTexture = false;

    if (version >= 0x14000000)
    {
        hasDiffuseTexture = false;
        hasSpecularTexture = false;
        hasBumpTexture = false;
    }
}

void NifMaterial::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    ambientColor.write(device);
    diffuseColor.write(device);
    specularColor.write(device);

    device.write(reinterpret_cast<const char*>(&specularExponent), sizeof(specularExponent));
    device.write(reinterpret_cast<const char*>(&emission), sizeof(emission));
    device.write(reinterpret_cast<const char*>(&reflectivity), sizeof(reflectivity));
    device.write(reinterpret_cast<const char*>(&opacity), sizeof(opacity));

    if (version >= 0x14000000)
    {
        // Write version-specific data
    }
}

void NifAlphaProperty::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    quint32 alphaTypeValue = 0;
    device.read(reinterpret_cast<char*>(&alphaTypeValue), sizeof(alphaTypeValue));
    alphaType = static_cast<AlphaType>(alphaTypeValue);

    alphaThreshold = 0;

    if (version >= 0x14000000)
    {
        alphaThreshold = 0;
    }
}

void NifAlphaProperty::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    quint32 alphaTypeValue = static_cast<quint32>(alphaType);
    device.write(reinterpret_cast<const char*>(&alphaTypeValue), sizeof(alphaTypeValue));

    device.write(reinterpret_cast<const char*>(&alphaThreshold), sizeof(alphaThreshold));
}

void NifBinaryExtraData::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    QByteArray extraDataNameBytes;
    quint32 extraDataNameLength = 0;
    device.read(reinterpret_cast<char*>(&extraDataNameLength), sizeof(extraDataNameLength));
    extraDataNameBytes.resize(extraDataNameLength);
    device.read(extraDataNameBytes.data(), extraDataNameLength);
    extraDataName = QString::fromLatin1(extraDataNameBytes);

    quint32 dataLength = 0;
    device.read(reinterpret_cast<char*>(&dataLength), sizeof(dataLength));
    data.resize(dataLength);
    device.read(data.data(), dataLength);
}

void NifBinaryExtraData::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    QByteArray extraDataNameBytes = extraDataName.toLatin1();
    quint32 extraDataNameLength = extraDataNameBytes.length();
    device.write(reinterpret_cast<const char*>(&extraDataNameLength), sizeof(extraDataNameLength));
    device.write(extraDataNameBytes);

    quint32 dataLength = data.length();
    device.write(reinterpret_cast<const char*>(&dataLength), sizeof(dataLength));
    device.write(data);
}

void NifStringExtraData::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    QByteArray extraDataNameBytes;
    quint32 extraDataNameLength = 0;
    device.read(reinterpret_cast<char*>(&extraDataNameLength), sizeof(extraDataNameLength));
    extraDataNameBytes.resize(extraDataNameLength);
    device.read(extraDataNameBytes.data(), extraDataNameLength);
    extraDataName = QString::fromLatin1(extraDataNameBytes);

    QByteArray dataBytes;
    quint32 dataLength = 0;
    device.read(reinterpret_cast<char*>(&dataLength), sizeof(dataLength));
    dataBytes.resize(dataLength);
    device.read(dataBytes.data(), dataLength);
    data = QString::fromLatin1(dataBytes);
}

void NifStringExtraData::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    QByteArray extraDataNameBytes = extraDataName.toLatin1();
    quint32 extraDataNameLength = extraDataNameBytes.length();
    device.write(reinterpret_cast<const char*>(&extraDataNameLength), sizeof(extraDataNameLength));
    device.write(extraDataNameBytes);

    QByteArray dataBytes = data.toLatin1();
    quint32 dataLength = dataBytes.length();
    device.write(reinterpret_cast<const char*>(&dataLength), sizeof(dataLength));
    device.write(dataBytes);
}

void NifKeyframeController::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    targetNode = readRef(device);

    device.read(reinterpret_cast<char*>(&flags), sizeof(flags));
    device.read(reinterpret_cast<char*>(&frequency), sizeof(frequency));
    device.read(reinterpret_cast<char*>(&phase), sizeof(phase));
    device.read(reinterpret_cast<char*>(&startTime), sizeof(startTime));
    device.read(reinterpret_cast<char*>(&stopTime), sizeof(stopTime));

    nextController = readRef(device);
    keyframeDataRef = readRef(device);
}

void NifKeyframeController::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    writeRef(device, targetNode);

    device.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
    device.write(reinterpret_cast<const char*>(&frequency), sizeof(frequency));
    device.write(reinterpret_cast<const char*>(&phase), sizeof(phase));
    device.write(reinterpret_cast<const char*>(&startTime), sizeof(startTime));
    device.write(reinterpret_cast<const char*>(&stopTime), sizeof(stopTime));

    writeRef(device, nextController);
    writeRef(device, keyframeDataRef);
}

void NifControllerManager::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    quint32 numControllers = 0;
    device.read(reinterpret_cast<char*>(&numControllers), sizeof(numControllers));
    controllerRefs.clear();
    controllerRefs.resize(numControllers);
    for (quint32 i = 0; i < numControllers; ++i) {
        controllerRefs[i] = readRef(device);
    }

    quint32 numClipNames = 0;
    device.read(reinterpret_cast<char*>(&numClipNames), sizeof(numClipNames));
    clipNames.clear();
    clipNames.resize(numClipNames);
    for (quint32 i = 0; i < numClipNames; ++i) {
        quint32 nameLen = 0;
        device.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        QByteArray nameBytes(nameLen, 0);
        device.read(nameBytes.data(), nameLen);
        clipNames[i] = QString::fromLatin1(nameBytes);
    }
}

void NifControllerManager::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    quint32 numControllers = controllerRefs.length();
    device.write(reinterpret_cast<const char*>(&numControllers), sizeof(numControllers));
    for (quint32 ref : controllerRefs) {
        writeRef(device, ref);
    }

    quint32 numClipNames = clipNames.length();
    device.write(reinterpret_cast<const char*>(&numClipNames), sizeof(numClipNames));
    for (const QString& name : clipNames) {
        QByteArray nameBytes = name.toLatin1();
        quint32 nameLen = nameBytes.length();
        device.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        device.write(nameBytes);
    }
}

void NifKeyframeData::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    device.read(reinterpret_cast<char*>(&numKeys), sizeof(numKeys));
    device.read(reinterpret_cast<char*>(&interpolationType), sizeof(interpolationType));

    keyframes.clear();
    keyframes.resize(numKeys);
    for (quint32 i = 0; i < numKeys; ++i) {
        float time = 0.0f;
        device.read(reinterpret_cast<char*>(&time), sizeof(time));

        float tx = 0.0f, ty = 0.0f, tz = 0.0f;
        device.read(reinterpret_cast<char*>(&tx), sizeof(tx));
        device.read(reinterpret_cast<char*>(&ty), sizeof(ty));
        device.read(reinterpret_cast<char*>(&tz), sizeof(tz));

        float qw = 0.0f, qx = 0.0f, qy = 0.0f, qz = 0.0f;
        device.read(reinterpret_cast<char*>(&qw), sizeof(qw));
        device.read(reinterpret_cast<char*>(&qx), sizeof(qx));
        device.read(reinterpret_cast<char*>(&qy), sizeof(qy));
        device.read(reinterpret_cast<char*>(&qz), sizeof(qz));

        float sx = 0.0f, sy = 0.0f, sz = 0.0f;
        device.read(reinterpret_cast<char*>(&sx), sizeof(sx));
        device.read(reinterpret_cast<char*>(&sy), sizeof(sy));
        device.read(reinterpret_cast<char*>(&sz), sizeof(sz));

        keyframes[i].time = time;
        keyframes[i].translation = {tx, ty, tz};
        keyframes[i].rotation = {time, qw, qx, qy, qz};
        keyframes[i].scale = {sx, sy, sz};
    }
}

void NifKeyframeData::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    device.write(reinterpret_cast<const char*>(&numKeys), sizeof(numKeys));
    device.write(reinterpret_cast<const char*>(&interpolationType), sizeof(interpolationType));

    for (const auto& kf : keyframes) {
        float time = kf.time;
        device.write(reinterpret_cast<const char*>(&time), sizeof(time));

        float tx = kf.translation.x;
        float ty = kf.translation.y;
        float tz = kf.translation.z;
        device.write(reinterpret_cast<const char*>(&tx), sizeof(tx));
        device.write(reinterpret_cast<const char*>(&ty), sizeof(ty));
        device.write(reinterpret_cast<const char*>(&tz), sizeof(tz));

        float qw = kf.rotation.w;
        float qx = kf.rotation.x;
        float qy = kf.rotation.y;
        float qz = kf.rotation.z;
        device.write(reinterpret_cast<const char*>(&qw), sizeof(qw));
        device.write(reinterpret_cast<const char*>(&qx), sizeof(qx));
        device.write(reinterpret_cast<const char*>(&qy), sizeof(qy));
        device.write(reinterpret_cast<const char*>(&qz), sizeof(qz));

        float sx = kf.scale.x;
        float sy = kf.scale.y;
        float sz = kf.scale.z;
        device.write(reinterpret_cast<const char*>(&sx), sizeof(sx));
        device.write(reinterpret_cast<const char*>(&sy), sizeof(sy));
        device.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
    }
}

void NifTransformData::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    device.read(reinterpret_cast<char*>(&numTranslationKeys), sizeof(numTranslationKeys));
    translateKeys.clear();
    translateKeys.resize(numTranslationKeys);
    for (quint32 i = 0; i < numTranslationKeys; ++i) {
        device.read(reinterpret_cast<char*>(&translateKeys[i].time), sizeof(float));
        float v[3];
        device.read(reinterpret_cast<char*>(v), sizeof(v));
        translateKeys[i].value = {v[0], v[1], v[2]};
    }

    device.read(reinterpret_cast<char*>(&numRotationKeys), sizeof(numRotationKeys));
    rotateKeys.clear();
    rotateKeys.resize(numRotationKeys);
    for (quint32 i = 0; i < numRotationKeys; ++i) {
        device.read(reinterpret_cast<char*>(&rotateKeys[i].time), sizeof(float));
        float q[4];
        device.read(reinterpret_cast<char*>(q), sizeof(q));
        rotateKeys[i].w = q[0];
        rotateKeys[i].x = q[1];
        rotateKeys[i].y = q[2];
        rotateKeys[i].z = q[3];
    }

    device.read(reinterpret_cast<char*>(&numScaleKeys), sizeof(numScaleKeys));
    scaleKeys.clear();
    scaleKeys.resize(numScaleKeys);
    for (quint32 i = 0; i < numScaleKeys; ++i) {
        device.read(reinterpret_cast<char*>(&scaleKeys[i].time), sizeof(float));
        float v[3];
        device.read(reinterpret_cast<char*>(v), sizeof(v));
        scaleKeys[i].value = {v[0], v[1], v[2]};
    }
}

void NifTransformData::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    device.write(reinterpret_cast<const char*>(&numTranslationKeys), sizeof(numTranslationKeys));
    for (const auto& k : translateKeys) {
        device.write(reinterpret_cast<const char*>(&k.time), sizeof(float));
        float v[3] = {k.value.x, k.value.y, k.value.z};
        device.write(reinterpret_cast<const char*>(v), sizeof(v));
    }

    device.write(reinterpret_cast<const char*>(&numRotationKeys), sizeof(numRotationKeys));
    for (const auto& k : rotateKeys) {
        device.write(reinterpret_cast<const char*>(&k.time), sizeof(float));
        float q[4] = {k.w, k.x, k.y, k.z};
        device.write(reinterpret_cast<const char*>(q), sizeof(q));
    }

    device.write(reinterpret_cast<const char*>(&numScaleKeys), sizeof(numScaleKeys));
    for (const auto& k : scaleKeys) {
        device.write(reinterpret_cast<const char*>(&k.time), sizeof(float));
        float v[3] = {k.value.x, k.value.y, k.value.z};
        device.write(reinterpret_cast<const char*>(v), sizeof(v));
    }
}

void NifParticleSystem::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    device.read(reinterpret_cast<char*>(&settings.numParticles), sizeof(settings.numParticles));
    device.read(reinterpret_cast<char*>(&settings.numVisibleParticles), sizeof(settings.numVisibleParticles));
    device.read(reinterpret_cast<char*>(&settings.emitterType), sizeof(settings.emitterType));
    device.read(reinterpret_cast<char*>(&settings.rendererType), sizeof(settings.rendererType));
    device.read(reinterpret_cast<char*>(&settings.emissionRate), sizeof(settings.emissionRate));
    device.read(reinterpret_cast<char*>(&settings.lifetime), sizeof(settings.lifetime));
    device.read(reinterpret_cast<char*>(&settings.lifetimeRandom), sizeof(settings.lifetimeRandom));
    device.read(reinterpret_cast<char*>(&settings.speed), sizeof(settings.speed));
    device.read(reinterpret_cast<char*>(&settings.speedRandom), sizeof(settings.speedRandom));
    device.read(reinterpret_cast<char*>(&settings.spread), sizeof(settings.spread));
    device.read(reinterpret_cast<char*>(&settings.gravityStrength), sizeof(settings.gravityStrength));
    device.read(reinterpret_cast<char*>(&settings.startSize), sizeof(settings.startSize));
    device.read(reinterpret_cast<char*>(&settings.startSizeRandom), sizeof(settings.startSizeRandom));
    device.read(reinterpret_cast<char*>(&settings.endSize), sizeof(settings.endSize));
    device.read(reinterpret_cast<char*>(&settings.endSizeRandom), sizeof(settings.endSizeRandom));

    settings.emitterPosition = NiPoint3::read(device);
    settings.emitterDirection = NiPoint3::read(device);

    device.read(reinterpret_cast<char*>(&settings.emitRadius), sizeof(settings.emitRadius));
    device.read(reinterpret_cast<char*>(&settings.emitAngle), sizeof(settings.emitAngle));

    quint32 texPathLen = 0;
    device.read(reinterpret_cast<char*>(&texPathLen), sizeof(texPathLen));
    QByteArray texPathBytes(texPathLen, 0);
    device.read(texPathBytes.data(), texPathLen);
    settings.texturePath = QString::fromLatin1(texPathBytes);

    device.read(reinterpret_cast<char*>(&settings.numColumns), sizeof(settings.numColumns));
    device.read(reinterpret_cast<char*>(&settings.numRows), sizeof(settings.numRows));

    quint8 additiveBlend = 0;
    device.read(reinterpret_cast<char*>(&additiveBlend), sizeof(additiveBlend));
    settings.additiveBlending = (additiveBlend != 0);

    quint8 alphaTestVal = 0;
    device.read(reinterpret_cast<char*>(&alphaTestVal), sizeof(alphaTestVal));
    settings.alphaTest = (alphaTestVal != 0);

    device.read(reinterpret_cast<char*>(&settings.startColorR), sizeof(float));
    device.read(reinterpret_cast<char*>(&settings.startColorG), sizeof(float));
    device.read(reinterpret_cast<char*>(&settings.startColorB), sizeof(float));
    device.read(reinterpret_cast<char*>(&settings.startColorA), sizeof(float));
    device.read(reinterpret_cast<char*>(&settings.endColorR), sizeof(float));
    device.read(reinterpret_cast<char*>(&settings.endColorG), sizeof(float));
    device.read(reinterpret_cast<char*>(&settings.endColorB), sizeof(float));
    device.read(reinterpret_cast<char*>(&settings.endColorA), sizeof(float));
}

void NifParticleSystem::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    device.write(reinterpret_cast<const char*>(&settings.numParticles), sizeof(settings.numParticles));
    device.write(reinterpret_cast<const char*>(&settings.numVisibleParticles), sizeof(settings.numVisibleParticles));
    device.write(reinterpret_cast<const char*>(&settings.emitterType), sizeof(settings.emitterType));
    device.write(reinterpret_cast<const char*>(&settings.rendererType), sizeof(settings.rendererType));
    device.write(reinterpret_cast<const char*>(&settings.emissionRate), sizeof(settings.emissionRate));
    device.write(reinterpret_cast<const char*>(&settings.lifetime), sizeof(settings.lifetime));
    device.write(reinterpret_cast<const char*>(&settings.lifetimeRandom), sizeof(settings.lifetimeRandom));
    device.write(reinterpret_cast<const char*>(&settings.speed), sizeof(settings.speed));
    device.write(reinterpret_cast<const char*>(&settings.speedRandom), sizeof(settings.speedRandom));
    device.write(reinterpret_cast<const char*>(&settings.spread), sizeof(settings.spread));
    device.write(reinterpret_cast<const char*>(&settings.gravityStrength), sizeof(settings.gravityStrength));
    device.write(reinterpret_cast<const char*>(&settings.startSize), sizeof(settings.startSize));
    device.write(reinterpret_cast<const char*>(&settings.startSizeRandom), sizeof(settings.startSizeRandom));
    device.write(reinterpret_cast<const char*>(&settings.endSize), sizeof(settings.endSize));
    device.write(reinterpret_cast<const char*>(&settings.endSizeRandom), sizeof(settings.endSizeRandom));

    settings.emitterPosition.write(device);
    settings.emitterDirection.write(device);

    device.write(reinterpret_cast<const char*>(&settings.emitRadius), sizeof(settings.emitRadius));
    device.write(reinterpret_cast<const char*>(&settings.emitAngle), sizeof(settings.emitAngle));

    QByteArray texPathBytes = settings.texturePath.toLatin1();
    quint32 texPathLen = texPathBytes.length();
    device.write(reinterpret_cast<const char*>(&texPathLen), sizeof(texPathLen));
    device.write(texPathBytes);

    device.write(reinterpret_cast<const char*>(&settings.numColumns), sizeof(settings.numColumns));
    device.write(reinterpret_cast<const char*>(&settings.numRows), sizeof(settings.numRows));

    quint8 additiveBlend = settings.additiveBlending ? 1 : 0;
    device.write(reinterpret_cast<const char*>(&additiveBlend), sizeof(additiveBlend));

    quint8 alphaTestVal = settings.alphaTest ? 1 : 0;
    device.write(reinterpret_cast<const char*>(&alphaTestVal), sizeof(alphaTestVal));

    device.write(reinterpret_cast<const char*>(&settings.startColorR), sizeof(float));
    device.write(reinterpret_cast<const char*>(&settings.startColorG), sizeof(float));
    device.write(reinterpret_cast<const char*>(&settings.startColorB), sizeof(float));
    device.write(reinterpret_cast<const char*>(&settings.startColorA), sizeof(float));
    device.write(reinterpret_cast<const char*>(&settings.endColorR), sizeof(float));
    device.write(reinterpret_cast<const char*>(&settings.endColorG), sizeof(float));
    device.write(reinterpret_cast<const char*>(&settings.endColorB), sizeof(float));
    device.write(reinterpret_cast<const char*>(&settings.endColorA), sizeof(float));
}

void NifPSysEmitter::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    device.read(reinterpret_cast<char*>(&emitterType), sizeof(emitterType));
    center = NiPoint3::read(device);
    extents = NiPoint3::read(device);
    device.read(reinterpret_cast<char*>(&radius), sizeof(radius));
    device.read(reinterpret_cast<char*>(&angle), sizeof(angle));
}

void NifPSysEmitter::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    device.write(reinterpret_cast<const char*>(&emitterType), sizeof(emitterType));
    center.write(device);
    extents.write(device);
    device.write(reinterpret_cast<const char*>(&radius), sizeof(radius));
    device.write(reinterpret_cast<const char*>(&angle), sizeof(angle));
}

void NifPSysModifier::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    quint32 typeVal = 0;
    device.read(reinterpret_cast<char*>(&typeVal), sizeof(typeVal));
    type = static_cast<Type>(typeVal);

    target = readRef(device);

    device.read(reinterpret_cast<char*>(&strength), sizeof(strength));
    device.read(reinterpret_cast<char*>(&damping), sizeof(damping));

    device.read(reinterpret_cast<char*>(&minColorR), sizeof(float));
    device.read(reinterpret_cast<char*>(&minColorG), sizeof(float));
    device.read(reinterpret_cast<char*>(&minColorB), sizeof(float));
    device.read(reinterpret_cast<char*>(&minColorA), sizeof(float));
    device.read(reinterpret_cast<char*>(&maxColorR), sizeof(float));
    device.read(reinterpret_cast<char*>(&maxColorG), sizeof(float));
    device.read(reinterpret_cast<char*>(&maxColorB), sizeof(float));
    device.read(reinterpret_cast<char*>(&maxColorA), sizeof(float));

    device.read(reinterpret_cast<char*>(&minAngle), sizeof(float));
    device.read(reinterpret_cast<char*>(&maxAngle), sizeof(float));
    device.read(reinterpret_cast<char*>(&fadeInTime), sizeof(float));
    device.read(reinterpret_cast<char*>(&fadeOutTime), sizeof(float));
    device.read(reinterpret_cast<char*>(&growTime), sizeof(float));
    device.read(reinterpret_cast<char*>(&shrinkTime), sizeof(float));
}

void NifPSysModifier::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    quint32 typeVal = static_cast<quint32>(type);
    device.write(reinterpret_cast<const char*>(&typeVal), sizeof(typeVal));

    writeRef(device, target);

    device.write(reinterpret_cast<const char*>(&strength), sizeof(strength));
    device.write(reinterpret_cast<const char*>(&damping), sizeof(damping));

    device.write(reinterpret_cast<const char*>(&minColorR), sizeof(float));
    device.write(reinterpret_cast<const char*>(&minColorG), sizeof(float));
    device.write(reinterpret_cast<const char*>(&minColorB), sizeof(float));
    device.write(reinterpret_cast<const char*>(&minColorA), sizeof(float));
    device.write(reinterpret_cast<const char*>(&maxColorR), sizeof(float));
    device.write(reinterpret_cast<const char*>(&maxColorG), sizeof(float));
    device.write(reinterpret_cast<const char*>(&maxColorB), sizeof(float));
    device.write(reinterpret_cast<const char*>(&maxColorA), sizeof(float));

    device.write(reinterpret_cast<const char*>(&minAngle), sizeof(float));
    device.write(reinterpret_cast<const char*>(&maxAngle), sizeof(float));
    device.write(reinterpret_cast<const char*>(&fadeInTime), sizeof(float));
    device.write(reinterpret_cast<const char*>(&fadeOutTime), sizeof(float));
    device.write(reinterpret_cast<const char*>(&growTime), sizeof(float));
    device.write(reinterpret_cast<const char*>(&shrinkTime), sizeof(float));
}

void NifLODNode::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifNode::parse(device, version, fileHeader);

    device.read(reinterpret_cast<char*>(&numLODLevels), sizeof(numLODLevels));

    lodLevels.clear();
    lodLevels.resize(numLODLevels);
    for (quint32 i = 0; i < numLODLevels; ++i) {
        device.read(reinterpret_cast<char*>(&lodLevels[i].minScreenSize), sizeof(float));
        device.read(reinterpret_cast<char*>(&lodLevels[i].maxScreenSize), sizeof(float));
    }
}

void NifLODNode::write(QIODevice& device, quint32 version) const
{
    NifNode::write(device, version);

    quint32 count = static_cast<quint32>(lodLevels.size());
    device.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& level : lodLevels) {
        device.write(reinterpret_cast<const char*>(&level.minScreenSize), sizeof(float));
        device.write(reinterpret_cast<const char*>(&level.maxScreenSize), sizeof(float));
    }
}

void NifBillboardNode::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifNode::parse(device, version, fileHeader);

    device.read(reinterpret_cast<char*>(&mode), sizeof(mode));
}

void NifBillboardNode::write(QIODevice& device, quint32 version) const
{
    NifNode::write(device, version);

    device.write(reinterpret_cast<const char*>(&mode), sizeof(mode));
}

void NifBSLightingShaderProperty::parse(QIODevice& device, quint32 version, const QByteArray& fileHeader)
{
    NifObject::parse(device, version, fileHeader);

    device.read(reinterpret_cast<char*>(&shaderType), sizeof(shaderType));
    device.read(reinterpret_cast<char*>(&shaderFlags1), sizeof(shaderFlags1));
    device.read(reinterpret_cast<char*>(&shaderFlags2), sizeof(shaderFlags2));
    device.read(reinterpret_cast<char*>(&glossiness), sizeof(glossiness));
    device.read(reinterpret_cast<char*>(&specularR), sizeof(float));
    device.read(reinterpret_cast<char*>(&specularG), sizeof(float));
    device.read(reinterpret_cast<char*>(&specularB), sizeof(float));
    device.read(reinterpret_cast<char*>(&alpha), sizeof(alpha));
}

void NifBSLightingShaderProperty::write(QIODevice& device, quint32 version) const
{
    NifObject::write(device, version);

    device.write(reinterpret_cast<const char*>(&shaderType), sizeof(shaderType));
    device.write(reinterpret_cast<const char*>(&shaderFlags1), sizeof(shaderFlags1));
    device.write(reinterpret_cast<const char*>(&shaderFlags2), sizeof(shaderFlags2));
    device.write(reinterpret_cast<const char*>(&glossiness), sizeof(glossiness));
    device.write(reinterpret_cast<const char*>(&specularR), sizeof(float));
    device.write(reinterpret_cast<const char*>(&specularG), sizeof(float));
    device.write(reinterpret_cast<const char*>(&specularB), sizeof(float));
    device.write(reinterpret_cast<const char*>(&alpha), sizeof(alpha));
}

#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

// Decodes a Starfield bhkPhysicsSystem block (the hknp collision data stored
// inside NIF files). The block is: u32 data_length + a TAG0 chunk stream.
// TAG0 (parent) contains SDKV (version string), DATA (system metadata +
// hknpConvexShape + polytope arrays), TYPE (type table, opaque), and INDX
// (ITEM records + PTCH records). Chunk header: 4-byte BE {decorator<<24 |
// size_with_header} followed by the 4-byte FourCC. Layout follows
// kaosnyrb/starfield_collision_mesh_injector hk_encode.py.
struct HknpPhysicsSystem
{
    struct Item
    {
        quint32 typeIdx = 0;
        quint32 dataOff = 0;
        quint32 count = 0;
    };
    struct Patch
    {
        qint32 typeIdx = 0;
        QVector<quint32> offsets;
    };
    struct Chunk
    {
        QByteArray fourcc;
        QByteArray body;
    };

    bool ok = false;
    quint32 dataLength = 0;
    QString sdkvVersion;
    QVector<Chunk> chunks;         // parsed TAG0 sub-chunks
    QVector<Item> items;
    QVector<Patch> patches;
    QByteArray dataBody;           // DATA chunk body
    QByteArray typeBody;           // TYPE chunk body

    // Polytope arrays (from DATA via the hknpConvexShape hkRelArray item
    // indices at +556..+603). Vertex = 3 floats, plane = 4 floats,
    // face = u16 firstIndex + u8 numIndices + u8 minHalfAngle,
    // index = u8, edge = u16 faceIndex + u8 edgeIndex + u8 padding.
    QVector<QVector<float>> vertices;
    QVector<QVector<float>> planes;
    QVector<quint32> faces;        // packed (firstIndex | numIndices<<16 | minHalfAngle<<24)
    QVector<quint8> indices;
    QVector<quint32> faceLinks;    // packed (faceIndex | edgeIndex<<16 | padding<<24)
    QVector<quint32> vertexEdges;  // packed same as faceLinks

    // Parses a bhkPhysicsSystem block (starting at the u32 data_length).
    static HknpPhysicsSystem read(const QByteArray& blockBytes);

private:
    void readPolytopeArrays();
};

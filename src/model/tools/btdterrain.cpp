#include "btdterrain.hpp"

#include <zlib.h>

#include "logger.hpp"

namespace {

quint32 rdU32(const QByteArray& b, qint64 pos)
{
    if (pos < 0 || pos + 4 > b.size()) return 0;
    return static_cast<quint32>(static_cast<quint8>(b.at(static_cast<int>(pos))))
         | (static_cast<quint32>(static_cast<quint8>(b.at(static_cast<int>(pos + 1)))) << 8)
         | (static_cast<quint32>(static_cast<quint8>(b.at(static_cast<int>(pos + 2)))) << 16)
         | (static_cast<quint32>(static_cast<quint8>(b.at(static_cast<int>(pos + 3)))) << 24);
}

qint32 rdI32(const QByteArray& b, qint64 pos)
{
    return static_cast<qint32>(rdU32(b, pos));
}

quint16 rdU16(const QByteArray& b, qint64 pos)
{
    if (pos < 0 || pos + 2 > b.size()) return 0;
    return static_cast<quint16>(static_cast<quint8>(b.at(static_cast<int>(pos))))
         | (static_cast<quint16>(static_cast<quint8>(b.at(static_cast<int>(pos + 1)))) << 8);
}

float rdF32(const QByteArray& b, qint64 pos)
{
    quint32 raw = rdU32(b, pos);
    float v = 0.0f;
    memcpy(&v, &raw, sizeof(v));
    return v;
}

bool has(const QByteArray& b, qint64 pos, qint64 n)
{
    return pos >= 0 && n >= 0 && pos + n <= b.size();
}

quint32 ceilDiv(quint32 a, quint32 b) { return (a + b - 1) / b; }

} // namespace

BtdTerrain BtdTerrain::read(const QByteArray& b)
{
    BtdTerrain t;
    if (b.size() < 44 || b.mid(0, 4) != "BTDB")
        return t;

    t.version = rdU32(b, 4);
    if (t.version != 5 && t.version != 6)
    {
        LOG_WARNING(QString("BtdTerrain: unsupported version %1").arg(t.version));
        return t;
    }

    t.worldHeightMin = rdF32(b, 8);
    t.worldHeightMax = rdF32(b, 12);
    t.resX = rdU32(b, 16);
    t.resY = rdU32(b, 20);
    t.cellMinX = rdI32(b, 24);
    t.cellMinY = rdI32(b, 28);
    t.cellMaxX = rdI32(b, 32);
    t.cellMaxY = rdI32(b, 36);
    t.ltexCount = rdU32(b, 40);

    if (!has(b, 44, static_cast<qint64>(t.ltexCount) * 4))
        return t;
    t.ltexFormIds.reserve(static_cast<int>(t.ltexCount));
    for (quint32 i = 0; i < t.ltexCount; ++i)
        t.ltexFormIds.append(rdU32(b, 44 + static_cast<qint64>(i) * 4));

    const bool isStarfield = !(t.cellMinX | t.cellMinY | t.cellMaxX | t.cellMaxY);
    if (isStarfield)
    {
        t.cellMinX = -static_cast<qint32>(t.resX >> 8);
        t.cellMinY = -static_cast<qint32>(t.resY >> 8);
        t.cellMaxX = t.cellMinX + static_cast<qint32>(t.resX >> 7) - 1;
        t.cellMaxY = t.cellMinY + static_cast<qint32>(t.resY >> 7) - 1;
    }
    t.nCellsX = static_cast<quint32>(t.cellMaxX + 1 - t.cellMinX);
    t.nCellsY = static_cast<quint32>(t.cellMaxY + 1 - t.cellMinY);
    if (t.nCellsX == 0 || t.nCellsY == 0)
        return t;

    qint64 pos = 44 + static_cast<qint64>(t.ltexCount) * 4;
    // Per-cell min/max height map.
    pos += (static_cast<qint64>(t.nCellsY) * t.nCellsX) << 3;
    // Per-quadrant land-texture indices.
    pos += (static_cast<qint64>(t.nCellsY) * t.nCellsX) << 5;
    if (!isStarfield)
    {
        // FO76: ground-cover records + map.
        if (!has(b, pos, 4)) return t;
        const quint32 gcvrCnt = rdU32(b, pos);
        pos += 4 + static_cast<qint64>(gcvrCnt) * 4;
        pos += (static_cast<qint64>(t.nCellsY) * t.nCellsX) << 5;
    }

    // LOD4 height map + land textures (nCellsY*8 x nCellsX*8 u16 each).
    const qint64 lod4Bytes = (static_cast<qint64>(t.nCellsY) * t.nCellsX) << 7;
    if (!has(b, pos, lod4Bytes) || !has(b, pos + lod4Bytes, lod4Bytes))
        return t;
    const int lod4Samples = static_cast<int>(lod4Bytes / 2);
    t.heightMapLOD4.resize(lod4Samples);
    t.landTexturesLOD4.resize(lod4Samples);
    for (int i = 0; i < lod4Samples; ++i)
    {
        t.heightMapLOD4[i] = rdU16(b, pos + static_cast<qint64>(i) * 2);
        t.landTexturesLOD4[i] = rdU16(b, pos + lod4Bytes + static_cast<qint64>(i) * 2);
    }
    pos += lod4Bytes * 2;

    if (!isStarfield)
        pos += (static_cast<qint64>(t.nCellsY) * t.nCellsX) << 7; // vertex color LOD4

    // zlib block tables (LOD3, LOD2, LOD1, LOD0) followed by the compressed
    // block data. Cell 0's LOD0 block is the first entry of the LOD0 table.
    qint64 dataOffs = pos;
    dataOffs += static_cast<qint64>(ceilDiv(t.nCellsX, 8) * ceilDiv(t.nCellsY, 8)) * 8;
    dataOffs += static_cast<qint64>(ceilDiv(t.nCellsX, 4) * ceilDiv(t.nCellsY, 4)) * 8;
    dataOffs += static_cast<qint64>(ceilDiv(t.nCellsX, 2) * ceilDiv(t.nCellsY, 2)) * 8;
    const qint64 lod0Table = dataOffs;
    dataOffs += static_cast<qint64>(t.nCellsX) * t.nCellsY * 8;

    const quint32 relOffs = rdU32(b, lod0Table);
    const quint32 compSize = rdU32(b, lod0Table + 4);
    const qint64 blockStart = dataOffs + relOffs;
    if (compSize > 0 && has(b, blockStart, compSize))
    {
        QByteArray out(65536, '\0');
        uLongf destLen = 65536;
        const int ret = uncompress(
            reinterpret_cast<Bytef*>(out.data()), &destLen,
            reinterpret_cast<const Bytef*>(b.constData() + static_cast<int>(blockStart)),
            compSize);
        if (ret == Z_OK && destLen >= 65536)
        {
            t.cellHeightMap.resize(16384);
            t.cellLandTextures.resize(16384);
            for (int i = 0; i < 16384; ++i)
            {
                t.cellHeightMap[i] = rdU16(out, static_cast<qint64>(i) * 2);
                t.cellLandTextures[i] = rdU16(out, 32768 + static_cast<qint64>(i) * 2);
            }
        }
        else
        {
            LOG_WARNING(QString("BtdTerrain: LOD0 block decompress failed (%1)").arg(ret));
        }
    }

    t.ok = true;
    return t;
}

#include "hknpphysicssystem.hpp"

#include <cstring>

#include "logger.hpp"

namespace {

quint32 beU32(const uchar* p) { return (static_cast<quint32>(p[0]) << 24)
    | (static_cast<quint32>(p[1]) << 16) | (static_cast<quint32>(p[2]) << 8) | p[3]; }

quint32 leU32(const QByteArray& b, qint64 pos)
{
    if (pos < 0 || pos + 4 > b.size()) return 0;
    return static_cast<quint32>(static_cast<quint8>(b.at(static_cast<int>(pos))))
         | (static_cast<quint32>(static_cast<quint8>(b.at(static_cast<int>(pos + 1)))) << 8)
         | (static_cast<quint32>(static_cast<quint8>(b.at(static_cast<int>(pos + 2)))) << 16)
         | (static_cast<quint32>(static_cast<quint8>(b.at(static_cast<int>(pos + 3)))) << 24);
}

quint16 leU16(const QByteArray& b, qint64 pos)
{
    if (pos < 0 || pos + 2 > b.size()) return 0;
    return static_cast<quint16>(static_cast<quint8>(b.at(static_cast<int>(pos))))
         | (static_cast<quint16>(static_cast<quint8>(b.at(static_cast<int>(pos + 1)))) << 8);
}

float leF32(const QByteArray& b, qint64 pos)
{
    const quint32 raw = leU32(b, pos);
    float v = 0.0f;
    memcpy(&v, &raw, sizeof(v));
    return v;
}

void putU32le(QByteArray& b, qint64 pos, quint32 v)
{
    if (pos < 0 || pos + 4 > b.size()) return;
    b[static_cast<int>(pos)] = static_cast<char>(v & 0xFF);
    b[static_cast<int>(pos + 1)] = static_cast<char>((v >> 8) & 0xFF);
    b[static_cast<int>(pos + 2)] = static_cast<char>((v >> 16) & 0xFF);
    b[static_cast<int>(pos + 3)] = static_cast<char>((v >> 24) & 0xFF);
}

void appendU32le(QByteArray& b, quint32 v)
{
    b.append(static_cast<char>(v & 0xFF));
    b.append(static_cast<char>((v >> 8) & 0xFF));
    b.append(static_cast<char>((v >> 16) & 0xFF));
    b.append(static_cast<char>((v >> 24) & 0xFF));
}

void appendU32be(QByteArray& b, quint32 v)
{
    b.append(static_cast<char>((v >> 24) & 0xFF));
    b.append(static_cast<char>((v >> 16) & 0xFF));
    b.append(static_cast<char>((v >> 8) & 0xFF));
    b.append(static_cast<char>(v & 0xFF));
}

void appendF32le(QByteArray& b, float v)
{
    quint32 raw = 0;
    memcpy(&raw, &v, sizeof(raw));
    appendU32le(b, raw);
}

// TAG0-style 8-byte header {decorator<<24 | size_with_header} + fourcc + body.
// Mirrors kaosnyrb hk_encode.py: DATA/ITEM/PTCH are leaves (0x40), TAG0/INDX
// are parents (0x00). read() masks off the decorator either way.
QByteArray packChunk(const QByteArray& fourcc, quint32 decorator, const QByteArray& body)
{
    QByteArray out;
    appendU32be(out, ((decorator & 0xFF) << 24) | ((8u + static_cast<quint32>(body.size())) & 0xFFFFFF));
    out.append(fourcc);
    out.append(body);
    return out;
}

} // namespace

HknpPhysicsSystem HknpPhysicsSystem::read(const QByteArray& block)
{
    HknpPhysicsSystem sys;
    if (block.size() < 8) return sys;

    sys.dataLength = leU32(block, 0);
    // The block payload follows the u32 length as a TAG0 chunk stream.
    qint64 pos = 4;
    // Walk the TAG0 (parent) chunk; its body holds SDKV/DATA/TYPE/INDX.
    if (block.mid(pos + 4, 4) != "TAG0") return sys;
    // TAG0 chunk header: [4-byte BE decorator|size][fourcc 'TAG0'][body]
    const quint32 tag0Hdr = beU32(reinterpret_cast<const uchar*>(block.constData() + pos));
    const quint32 tag0BodySize = (tag0Hdr & 0xFFFFFF) - 8;
    pos += 8;
    const qint64 tag0End = pos + tag0BodySize;
    if (tag0End > block.size()) return sys;

    // Parse sub-chunks inside TAG0.
    while (pos + 8 <= tag0End)
    {
        const quint32 hdr = beU32(reinterpret_cast<const uchar*>(block.constData() + pos));
        const quint32 sizeWithHeader = hdr & 0xFFFFFF;
        const QByteArray fourcc = block.mid(pos + 4, 4);
        pos += 8;
        if (sizeWithHeader < 8 || pos + static_cast<qint64>(sizeWithHeader) - 8 > tag0End)
            break;
        Chunk c;
        c.fourcc = fourcc;
        c.body = block.mid(pos, static_cast<int>(sizeWithHeader) - 8);
        sys.chunks.append(c);
        pos += static_cast<qint64>(sizeWithHeader) - 8;
    }

    for (const Chunk& c : sys.chunks)
    {
        if (c.fourcc == "SDKV") sys.sdkvVersion = QString::fromLatin1(c.body);
        else if (c.fourcc == "DATA") sys.dataBody = c.body;
        else if (c.fourcc == "TYPE") sys.typeBody = c.body;
        else if (c.fourcc == "INDX")
        {
            // INDX = ITEM + PTCH chunks.
            qint64 p = 0;
            while (p + 8 <= c.body.size())
            {
                const quint32 hdr = beU32(reinterpret_cast<const uchar*>(c.body.constData() + p));
                const quint32 sizeWithHeader = hdr & 0xFFFFFF;
                const QByteArray fourcc = c.body.mid(p + 4, 4);
                p += 8;
                if (sizeWithHeader < 8 || p + static_cast<qint64>(sizeWithHeader) - 8 > c.body.size())
                    break;
                const QByteArray body = c.body.mid(p, static_cast<int>(sizeWithHeader) - 8);
                if (fourcc == "ITEM")
                {
                    for (qint64 off = 0; off + 12 <= body.size(); off += 12)
                    {
                        Item it;
                        it.typeIdx = leU32(body, off);
                        it.dataOff = leU32(body, off + 4);
                        it.count = leU32(body, off + 8);
                        sys.items.append(it);
                    }
                }
                else if (fourcc == "PTCH")
                {
                    for (qint64 off = 0; off + 8 <= body.size(); )
                    {
                        Patch patch;
                        patch.typeIdx = static_cast<qint32>(leU32(body, off));
                        const quint32 n = leU32(body, off + 4);
                        off += 8;
                        if (off + static_cast<qint64>(n) * 4 > body.size()) break;
                        for (quint32 k = 0; k < n; ++k)
                        {
                            patch.offsets.append(leU32(body, off));
                            off += 4;
                        }
                        sys.patches.append(patch);
                    }
                }
                p += static_cast<qint64>(sizeWithHeader) - 8;
            }
        }
    }

    if (sys.dataBody.isEmpty() || sys.items.isEmpty())
    {
        LOG_WARNING("HknpPhysicsSystem: missing DATA or ITEM chunk");
        return sys;
    }

    sys.readPolytopeArrays();
    sys.ok = true;
    return sys;
}

QByteArray HknpPhysicsSystem::encode(const QString& sdkvVersion,
                                     const QByteArray& typeBody,
                                     const ConvexShapeData& shape,
                                     const QVector<Patch>& patches)
{
    // DATA body: 608-byte fixed prefix (system metadata + hknpConvexShape,
    // zero-filled -- read() only interprets the hkRelArray fields) with the six
    // hkRelArray item indices at +556..+603, then the polytope arrays laid out
    // 16-byte aligned from offset 608, exactly like hk_encode.py.
    QByteArray dataBody(608, '\0');
    for (int k = 0; k < 6; ++k)
    {
        putU32le(dataBody, 556 + 8 * k, static_cast<quint32>(k));
        putU32le(dataBody, 560 + 8 * k, 0);
    }

    quint32 itemData[6] = { 0, 0, 0, 0, 0, 0 };
    quint32 itemCount[6] = { 0, 0, 0, 0, 0, 0 };

    const auto align16 = [&dataBody]() {
        while (dataBody.size() % 16 != 0) dataBody.append('\0');
    };

    align16();
    itemData[0] = static_cast<quint32>(dataBody.size());
    itemCount[0] = static_cast<quint32>(shape.vertices.size());
    for (const QVector<float>& v : shape.vertices)
        for (float f : v) appendF32le(dataBody, f);

    align16();
    itemData[1] = static_cast<quint32>(dataBody.size());
    itemCount[1] = static_cast<quint32>(shape.planes.size());
    for (const QVector<float>& p : shape.planes)
        for (float f : p) appendF32le(dataBody, f);

    align16();
    itemData[2] = static_cast<quint32>(dataBody.size());
    itemCount[2] = static_cast<quint32>(shape.faces.size());
    for (quint32 f : shape.faces) appendU32le(dataBody, f);

    align16();
    itemData[3] = static_cast<quint32>(dataBody.size());
    itemCount[3] = static_cast<quint32>(shape.indices.size());
    dataBody.append(reinterpret_cast<const char*>(shape.indices.constData()),
                    shape.indices.size());

    align16();
    itemData[4] = static_cast<quint32>(dataBody.size());
    itemCount[4] = static_cast<quint32>(shape.faceLinks.size());
    for (quint32 e : shape.faceLinks) appendU32le(dataBody, e);

    align16();
    itemData[5] = static_cast<quint32>(dataBody.size());
    itemCount[5] = static_cast<quint32>(shape.vertexEdges.size());
    for (quint32 e : shape.vertexEdges) appendU32le(dataBody, e);

    align16();

    QByteArray itemBody;
    for (int k = 0; k < 6; ++k)
    {
        appendU32le(itemBody, shape.itemTypeIdx.value(k));
        appendU32le(itemBody, itemData[k]);
        appendU32le(itemBody, itemCount[k]);
    }

    QByteArray patchBody;
    for (const Patch& p : patches)
    {
        appendU32le(patchBody, static_cast<quint32>(p.typeIdx));
        appendU32le(patchBody, static_cast<quint32>(p.offsets.size()));
        for (quint32 o : p.offsets) appendU32le(patchBody, o);
    }

    const QByteArray sdkvBytes = packChunk("SDKV", 0x00, sdkvVersion.toLatin1());
    const QByteArray dataBytes = packChunk("DATA", 0x40, dataBody);
    const QByteArray typeBytes = packChunk("TYPE", 0x00, typeBody);
    const QByteArray indxBytes = packChunk("INDX", 0x00,
        packChunk("ITEM", 0x40, itemBody) + packChunk("PTCH", 0x40, patchBody));
    const QByteArray tag0Bytes = packChunk("TAG0", 0x00,
        sdkvBytes + dataBytes + typeBytes + indxBytes);

    QByteArray block;
    appendU32le(block, static_cast<quint32>(tag0Bytes.size()));
    block.append(tag0Bytes);
    return block;
}

void HknpPhysicsSystem::readPolytopeArrays()
{
    if (dataBody.size() < 608) return;
    // The hknpConvexShape holds six hkRelArray fields at +556..+603; each is
    // { i32 itemIndex, i32 reserved }. Use the item index to locate the array
    // in the DATA body via the ITEM records.
    struct Rel { quint32 itemIdx; QVector<float>* fout; QVector<quint32>* uout; QVector<quint8>* bout; };
    QVector<Rel> rels = {
        { leU32(dataBody, 556), nullptr, nullptr, nullptr },  // vertices
        { leU32(dataBody, 564), nullptr, nullptr, nullptr },  // planes
        { leU32(dataBody, 572), nullptr, nullptr, nullptr },  // faces
        { leU32(dataBody, 580), nullptr, nullptr, nullptr },  // indices
        { leU32(dataBody, 588), nullptr, nullptr, nullptr },  // faceLinks
        { leU32(dataBody, 596), nullptr, nullptr, nullptr },  // vertexEdges
    };

    auto itemFor = [&](quint32 idx) -> const Item* {
        // ITEM records have no explicit index; the item index is the record's
        // position in the ITEM list.
        if (idx < static_cast<quint32>(items.size())) return &items.at(idx);
        for (const Item& it : items) if (it.typeIdx == idx) return &it;
        return nullptr;
    };

    LOG_DEBUG(QString("HknpPhysicsSystem: rel item indices = %1 %2 %3 %4 %5 %6 (of %7 items)")
        .arg(rels[0].itemIdx).arg(rels[1].itemIdx).arg(rels[2].itemIdx)
        .arg(rels[3].itemIdx).arg(rels[4].itemIdx).arg(rels[5].itemIdx)
        .arg(items.size()));

    // Vertices (12 bytes each) and planes (16 bytes each) are floats.
    if (const Item* it = itemFor(rels[0].itemIdx))
    {
        for (quint32 k = 0; k < it->count; ++k)
        {
            const qint64 off = static_cast<qint64>(it->dataOff) + static_cast<qint64>(k) * 12;
            if (off + 12 > dataBody.size()) break;
            vertices.append({ leF32(dataBody, off), leF32(dataBody, off + 4), leF32(dataBody, off + 8) });
        }
    }
    if (const Item* it = itemFor(rels[1].itemIdx))
    {
        for (quint32 k = 0; k < it->count; ++k)
        {
            const qint64 off = static_cast<qint64>(it->dataOff) + static_cast<qint64>(k) * 16;
            if (off + 16 > dataBody.size()) break;
            planes.append({ leF32(dataBody, off), leF32(dataBody, off + 4),
                            leF32(dataBody, off + 8), leF32(dataBody, off + 12) });
        }
    }
    if (const Item* it = itemFor(rels[2].itemIdx))
    {
        for (quint32 k = 0; k < it->count; ++k)
        {
            const qint64 off = static_cast<qint64>(it->dataOff) + static_cast<qint64>(k) * 4;
            if (off + 4 > dataBody.size()) break;
            faces.append(leU32(dataBody, off));
        }
    }
    if (const Item* it = itemFor(rels[3].itemIdx))
    {
        for (quint32 k = 0; k < it->count; ++k)
        {
            const qint64 off = static_cast<qint64>(it->dataOff) + static_cast<qint64>(k);
            if (off >= dataBody.size()) break;
            indices.append(static_cast<quint8>(dataBody.at(static_cast<int>(off))));
        }
    }
    if (const Item* it = itemFor(rels[4].itemIdx))
    {
        for (quint32 k = 0; k < it->count; ++k)
        {
            const qint64 off = static_cast<qint64>(it->dataOff) + static_cast<qint64>(k) * 4;
            if (off + 4 > dataBody.size()) break;
            faceLinks.append(leU32(dataBody, off));
        }
    }
    if (const Item* it = itemFor(rels[5].itemIdx))
    {
        for (quint32 k = 0; k < it->count; ++k)
        {
            const qint64 off = static_cast<qint64>(it->dataOff) + static_cast<qint64>(k) * 4;
            if (off + 4 > dataBody.size()) break;
            vertexEdges.append(leU32(dataBody, off));
        }
    }

    LOG_DEBUG(QString("HknpPhysicsSystem: %1 verts, %2 planes, %3 faces, %4 indices, %5 items")
        .arg(vertices.size()).arg(planes.size()).arg(faces.size())
        .arg(indices.size()).arg(items.size()));
}

#include "nifblockfile.hpp"
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <QFile>
#include <QSaveFile>

#include <cstring>

#include "logger.hpp"

namespace {

// Little-endian cursor over a byte range. Every read is bounds-checked and
// `ok` latches false, so a layout mismatch surfaces as a rejected block
// instead of a silent misparse.
class Cursor {
public:
    Cursor(const QByteArray& data, int offset = 0)
        : mData(data), mPos(offset) {}

    bool ok() const { return mOk; }
    void invalidate() { mOk = false; }
    int pos() const { return mPos; }
    int remaining() const { return mData.size() - mPos; }
    bool atEnd() const { return mPos >= mData.size(); }

    // Free-text note a walker leaves behind for the failure report. It carries
    // no parsing meaning; it exists so a rejected block can say which field it
    // was in rather than only how far it got.
    QString note;

    quint8 u8()
    {
        if (!take(1)) return 0;
        return static_cast<quint8>(mData.at(mPos - 1));
    }
    quint16 u16()
    {
        if (!take(2)) return 0;
        return static_cast<quint16>(static_cast<quint8>(mData.at(mPos - 2)))
             | static_cast<quint16>(static_cast<quint8>(mData.at(mPos - 1))) << 8;
    }
    quint32 u32()
    {
        if (!take(4)) return 0;
        return static_cast<quint32>(static_cast<quint8>(mData.at(mPos - 4)))
             | static_cast<quint32>(static_cast<quint8>(mData.at(mPos - 3))) << 8
             | static_cast<quint32>(static_cast<quint8>(mData.at(mPos - 2))) << 16
             | static_cast<quint32>(static_cast<quint8>(mData.at(mPos - 1))) << 24;
    }
    float f32()
    {
        const quint32 bits = u32();
        float value = 0.0f;
        memcpy(&value, &bits, 4);
        return value;
    }
    Nif::Vector3 vec3() { const float x = f32(); const float y = f32(); const float z = f32(); return {x, y, z}; }
    Nif::QuaternionKeyframe quat()
    {
        const float time = f32();
        Nif::QuaternionKeyframe q;
        q.time = time;
        q.w = f32();
        q.x = f32();
        q.y = f32();
        q.z = f32();
        return q;
    }
    QByteArray raw(int n)
    {
        if (!take(n)) return QByteArray();
        return mData.mid(mPos - n, n);
    }

private:
    bool take(int n)
    {
        if (!mOk || n < 0 || mPos + n > mData.size()) { mOk = false; return false; }
        mPos += n;
        return true;
    }

    const QByteArray& mData;
    int mPos = 0;
    bool mOk = true;
};

void appendU8(QByteArray& out, quint8 v) { out.append(static_cast<char>(v)); }

void appendU16(QByteArray& out, quint16 v)
{
    out.append(static_cast<char>(v & 0xFF));
    out.append(static_cast<char>((v >> 8) & 0xFF));
}

void appendU32(QByteArray& out, quint32 v)
{
    out.append(static_cast<char>(v & 0xFF));
    out.append(static_cast<char>((v >> 8) & 0xFF));
    out.append(static_cast<char>((v >> 16) & 0xFF));
    out.append(static_cast<char>((v >> 24) & 0xFF));
}

void appendF32(QByteArray& out, float v)
{
    quint32 bits = 0;
    memcpy(&bits, &v, 4);
    appendU32(out, bits);
}

void appendVec3(QByteArray& out, const Nif::Vector3& v)
{
    appendF32(out, v.x);
    appendF32(out, v.y);
    appendF32(out, v.z);
}

void appendQuat(QByteArray& out, const Nif::QuaternionKeyframe& q)
{
    appendF32(out, q.w);
    appendF32(out, q.x);
    appendF32(out, q.y);
    appendF32(out, q.z);
}

// NIF ExportString: u8 length (including the NUL) + that many bytes. The
// encoded form is returned verbatim so a re-save reproduces it byte for byte.
QByteArray readExportString(Cursor& c)
{
    const quint8 len = c.u8();
    if (!c.ok() || len == 0 || len > 250) { c.invalidate(); return QByteArray(); }
    QByteArray encoded;
    encoded.append(static_cast<char>(len));
    encoded.append(c.raw(len));
    return encoded;
}

constexpr int kMaxBlocks = 2000000;
constexpr int kMaxStrings = 200000;
constexpr int kMaxBlockTypes = 1000;
constexpr int kMaxGroups = 10000;
constexpr int kMaxStringLength = 4096;

// Two header fields were added part-way through the Gamebryo line, and their
// absence is not cosmetic: it changes where the block payloads start.
//
// The endian_type byte only exists from 20.0.0.3 on. Earlier files predate the
// field and are always little-endian, so the byte is simply not in the file:
// reading it anyway picks up the low byte of user_version, which for the
// shipped 10.2.0.0 meshes reads back as an "endianness" of 10.
constexpr quint32 kEndianFieldVersion = 0x14000003u;

// The per-block size table arrived in 20.2.0.5. Before it, the file records
// neither block lengths nor anything else that delimits them, so the payload
// region has to be kept as one opaque run. The header string table arrived in
// 20.1.0.1.
//
// Both thresholds are from the NIF format definition (the vendored nifgen
// `Header` struct, which gates `block_size` on version >= 0x14020005 and
// `num_strings`/`max_string_length`/`strings` on version >= 0x14010001), and
// both are confirmed against shipped Oblivion bytes: a 20.0.0.4 file goes
// straight from the block type index table to the group count.
constexpr quint32 kBlockSizeTableVersion = 0x14020005u;
constexpr quint32 kStringTableVersion = 0x14010001u;

// --- Walking a block region that has no size table ---------------------------
//
// A pre-20.2.0.5 file records no block lengths, so the only way to find where
// one block ends and the next begins is to read the block: every field in
// order, including the variable-length arrays. Each walker below therefore has
// to know a block type's real field layout, not merely its fixed size.
//
// Every walker is strict. It either consumes the whole block or reports failure,
// and the walk as a whole is only accepted if it lands exactly on the footer at
// the end of the region. A layout that is wrong for some type therefore
// degrades to the opaque fallback rather than silently shifting every later
// block and mislabelling the file.

using BlockWalker = bool (*)(Cursor&, quint32 version, quint32 bsVersion);

// The oldest container these walkers claim to understand. 10.1.x is the first
// Gamebryo version with a user_version field, and everything below that has a
// different header again (that is the NetImmerse case, handled elsewhere).
constexpr quint32 kWalkVersionFloor = 0x0A000000u;

// Up to and including 10.1.0.106 every non-bhk block is preceded by a u32 that
// must be zero. Confirmed from bytes: a 10.1.0.106 file has a zero word
// immediately before its first block's name, and removing it lines the whole
// block up, where without it the name lands two words early.
constexpr quint32 kBlockDummyVersion = 0x0A01006Au;

bool skipRefs(Cursor& c, quint32 count)
{
    for (quint32 i = 0; i < count; ++i) c.u32();
    return c.ok();
}

// A String is an inline u32-length-prefixed string up to 20.0.0.5 and a
// string-table index from 20.1.0.3 on, which is the same version at which the
// header grows a string table to index into.
bool skipString(Cursor& c, quint32 version)
{
    if (version <= kBlockSizeTableVersion - 1u) {
        const quint32 len = c.u32();
        if (!c.ok() || len > static_cast<quint32>(kMaxStringLength)) return false;
        c.raw(static_cast<int>(len));
        return c.ok();
    }
    c.u32();
    return c.ok();
}

bool walkNiObjectNET(Cursor& c, quint32 version)
{
    if (!skipString(c, version)) return false;
    // legacy_extra_data is four u32s; nothing here is old enough to carry one.
    if (version >= 50331648u && version <= 67240448u) c.u32();  // extra_data
    if (version >= 167772416u) {                                 // num_extra_data_list
        const quint32 n = c.u32();
        c.note = QStringLiteral("extra_data_list=%1").arg(n);
        if (!c.ok() || n > 100000u) return false;
        // Entries are 4 bytes. Widening them to 8 fits one 20.0.0.4 file, whose
        // next block name then reads `03 00 00 00 "BSX"`, but that file is not
        // representative: 8-byte entries across the archive fail every file at
        // this block, and so do a flat extra word on every 20.x node and
        // per-version entry sizes. Four is the only reading that holds up, so
        // the handful of files that still disagree are unexplained rather than
        // accommodated.
        if (!skipRefs(c, n)) return false;
    }
    if (version >= 50331648u) c.u32();                           // controller
    return c.ok();
}

bool walkNiAVObject(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiObjectNET(c, version)) return false;
    // flags narrows to a u16 on the old stream versions.
    if (bsVersion > 26) c.u32();
    else if (version >= 50331648u) c.u16();
    else return false;
    c.raw(12);   // translation
    c.raw(36);   // rotation
    c.f32();     // scale
    if (version <= 67240448u) c.raw(12);                        // velocity
    if (bsVersion <= 34) {                                      // properties
        const quint32 n = c.u32();
        if (!c.ok() || n > 100000u) return false;
        if (!skipRefs(c, n)) return false;
    }
    // Bounding volumes only exist in the 3.x-4.x range, which is not walked.
    if (version >= 167772416u) c.u32();                          // collision object
    return c.ok();
}

// NiExtraData descends from NiObject, not NiObjectNET, so it does not carry a
// controller ref. In the versions walked here the base is just the name: the
// next_extra_data ref only exists up to 0x04020200 and the legacy extra_data
// and num_bytes fields are older still, so all three are absent. Reading the
// ref anyway shifts every following field by four bytes.
bool walkNiExtraData(Cursor& c, quint32 version)
{
    return skipString(c, version);
}

bool walkBSXFlags(Cursor& c, quint32 version, quint32)
{
    if (!walkNiExtraData(c, version)) return false;
    c.u32();  // integer_data
    return c.ok();
}

bool walkNiStringExtraData(Cursor& c, quint32 version, quint32)
{
    if (!walkNiExtraData(c, version)) return false;
    if (!skipString(c, version)) return false;
    return c.ok();
}

bool walkNiBinaryExtraData(Cursor& c, quint32 version, quint32)
{
    if (!walkNiExtraData(c, version)) return false;
    const quint32 len = c.u32();
    if (!c.ok() || len > 64u * 1024u * 1024u) return false;
    c.raw(static_cast<int>(len));
    return c.ok();
}

// NiGeometry adds a bounding sphere and a skin ref to NiAVObject.
bool walkNiGeometry(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiAVObject(c, version, bsVersion)) return false;
    c.raw(12);  // bounding sphere centre
    c.f32();    // bounding sphere radius
    c.u32();    // skin
    return c.ok();
}

// NiKeyframeData: rotations, then the translation and scale channels. The first
// key of each channel carries no time, which is why the counts are not simply
// multiplied out.
void skipKeyChannel(Cursor& c)
{
    const quint32 numKeys = c.u32();
    const quint8 interpolation = c.u8();
    if (!c.ok() || numKeys > 10'000'000u || interpolation > 3) return;
    for (quint32 i = 0; i < numKeys; ++i) {
        if (i > 0) c.f32();
        c.raw(12);  // value
    }
}

bool walkNiKeyframeData(Cursor& c, quint32 version, quint32)
{
    Q_UNUSED(version)
    const quint32 numRotationKeys = c.u32();
    const quint8 rotationType = c.u8();
    if (!c.ok() || numRotationKeys > 10'000'000u || rotationType > 3) return false;
    c.f32();  // order
    for (quint32 i = 0; i < numRotationKeys; ++i) {
        if (i > 0) c.f32();
        c.raw(16);  // quaternion
    }
    skipKeyChannel(c);  // translations
    skipKeyChannel(c);  // scales
    return c.ok();
}

// NiFloatData / NiBoolData / NiPosData / NiColorData all carry a single
// keyframe channel and nothing else.
bool walkKeyGroupData(Cursor& c, quint32, quint32)
{
    skipKeyChannel(c);
    return c.ok();
}

// NiTimeController: the common prefix of every controller, including the
// interpolated ones, which insert the interpolator ref between it and `data`.
bool walkNiTimeController(Cursor& c)
{
    c.u32();    // next_controller
    c.u32();    // flags
    c.f32();    // frequency
    c.f32();    // phase
    c.f32();    // start_time
    c.f32();    // stop_time
    c.u32();    // target
    return c.ok();
}

bool walkKeyframeController(Cursor& c, quint32 version, quint32 bsVersion)
{
    Q_UNUSED(version)
    if (!walkNiTimeController(c)) return false;
    c.u32();  // interpolator
    c.u32();  // data
    return c.ok();
}

bool walkNiTransformController(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkKeyframeController(c, version, bsVersion)) return false;
    c.u32();  // unknown_q_q_speed_integer
    return c.ok();
}

bool walkNiTransformInterpolator(Cursor& c, quint32, quint32)
{
    c.raw(32);  // translation + rotation + scale
    c.u32();    // data
    return c.ok();
}

bool walkNiNode(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiAVObject(c, version, bsVersion)) return false;
    const quint32 numChildren = c.u32();
    if (!c.ok() || numChildren > 100000u) return false;
    if (!skipRefs(c, numChildren)) return false;
    if (bsVersion < 130) {
        const quint32 numEffects = c.u32();
        if (!c.ok() || numEffects > 100000u) return false;
        if (!skipRefs(c, numEffects)) return false;
    }
    return c.ok();
}

// NiGeometryData: the vertex stream, and the per-vertex attribute flags that
// say which optional arrays follow it. Those "has_*" fields are bytes holding
// sentinels rather than booleans - 6, 7 and 15 mean "stored in a compressed
// form" and not "this many" - so they cannot be read as a count.
bool walkNiGeometryData(Cursor& c, quint32 version, quint32 bsVersion)
{
    Q_UNUSED(bsVersion)
    if (version >= 167837810u) c.u32();                      // group_id
    const quint16 numVertices = c.u16();
    if (!c.ok() || numVertices > 1000000u) return false;
    if (version >= 167837696u) { c.u8(); c.u8(); }            // keep/compress flags
    const quint8 hasVertices = c.u8();
    if (!c.ok() || hasVertices > 1) return false;
    if (hasVertices) c.raw(numVertices * 12);                 // vertices, Vector3
    quint16 dataFlags = 0;
    if (version >= 167772416u) dataFlags = c.u16();
    if (!c.ok()) return false;
    const quint8 hasNormals = c.u8();
    if (!c.ok() || hasNormals > 1) return false;
    if (hasNormals) c.raw(numVertices * 12);                 // normals, Vector3
    if (version >= 167837696u && hasNormals && (dataFlags & 4096u) != 0) {
        c.raw(numVertices * 12);                             // tangents
        c.raw(numVertices * 12);                             // bitangents
    }
    c.raw(16);                                                // bounding sphere
    const quint8 hasVertexColors = c.u8();
    if (!c.ok() || hasVertexColors > 1) return false;
    if (hasVertexColors) c.raw(numVertices * 16);             // vertex colours, Color4
    if (hasVertices) c.raw(numVertices * 8 * (dataFlags & 63u));  // uv_sets, TexCoord
    if (version >= 167772416u) c.u16();                      // consistency flags
    if (version >= 335544324u) c.u32();                      // additional data ref
    return c.ok();
}

bool walkNiTriStripsData(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiGeometryData(c, version, bsVersion)) return false;
    const quint16 numTriangles = c.u16();
    const quint16 numStrips = c.u16();
    if (!c.ok() || numStrips > 4096u) return false;
    QVector<quint16> stripLengths;
    stripLengths.reserve(numStrips);
    for (quint16 i = 0; i < numStrips; ++i) stripLengths.append(c.u16());
    if (!c.ok()) return false;
    quint8 hasPoints = 0;
    if (version >= 167772419u) hasPoints = c.u8();
    if (!c.ok() || hasPoints > 1) return false;
    if (hasPoints) {
        for (quint16 s = 0; s < numStrips; ++s) c.raw(stripLengths.at(s) * 2);
    }
    // No triangle indices follow. The triangles are implicit in the strip's
    // point runs - a strip of n points is n-2 triangles - which is why
    // num_triangles equals sum(strip_lengths) - 2 * num_strips. Verified against
    // a real 20.0.0.4 block: the geometry fields plus the strip fields account
    // for all 76,318 bytes of it exactly, with nothing left over. Appending
    // num_triangles * 6 here overran the block by 12,624 bytes.
    return c.ok();
}

// --- Havok collision shapes --------------------------------------------------
//
// A Havok enum is a u32 on the wire, the same width the NIF format definition
// uses for every enum. HavokMaterial carries a leading unknown_int on the
// oldest versions that have it; HavokFilter is a layer enum, a flags enum and a
// u16 group.
constexpr quint32 kHavokMaterialUnknownIntVersion = 0x0A000102u;  // 10.0.1.2

bool skipHavokMaterial(Cursor& c, quint32 version)
{
    if (version <= kHavokMaterialUnknownIntVersion) c.u32();
    c.u32();
    return c.ok();
}

// bhkNiTriStripsShape is the collision shape on the large majority of shipped
// Gamebryo meshes, which makes it by far the highest-value walker here: on its
// own it accounts for 4,297 of the 7,962 files in Oblivion - Meshes.bsa, more
// than every other missing layout combined.
//
// Layout taken from the nifgen `BhkNiTriStripsShape` chain. Its bases -
// BhkShapeCollection, BhkShape, BhkSerializable, BhkRefObject, NiObject - add no
// inline bytes of their own, so the block is exactly the fields below:
//   HavokMaterial material   u32, plus a leading u32 on the oldest versions
//   f32          radius
//   byte         unused_01[20]
//   u32          grow_by
//   Vector4      scale       four floats
//   u32          num_strips_data, then that many refs
//   u32          num_filters, then that many HavokFilters
//
// Verified against a shipped 20.0.0.4 file: the oracle reports io_size 64 for
// the block, and with both arrays empty the fields below sum to exactly 64.
bool walkBhkNiTriStripsShape(Cursor& c, quint32 version, quint32)
{
    if (!skipHavokMaterial(c, version)) return false;
    c.f32();                                   // radius
    c.raw(20);                                 // unused_01
    c.u32();                                   // grow_by
    c.raw(16);                                 // scale (Vector4)
    const quint32 numStrips = c.u32();
    if (!c.ok() || numStrips > 100000u) return false;
    if (!skipRefs(c, numStrips)) return false;
    const quint32 numFilters = c.u32();
    if (!c.ok() || numFilters > 100000u) return false;
    for (quint32 i = 0; i < numFilters; ++i) {
        c.u32();                               // layer
        c.u32();                               // flags
        c.u16();                               // group
    }
    return c.ok();
}

// bhkMoppBvTreeShape carries a Havok MOPP code blob, and is the collision shape
// on most static Gamebryo geometry - 4,281 files in Oblivion - Meshes.bsa, once
// bhkNiTriStripsShape is understood.
//
// From the nifgen `BhkMoppBvTreeShape` / `BhkBvTreeShape` / `HkpMoppCode`
// attribute lists:
//   ref         shape          a ref to the underlying bhkShape
//   byte        unused_01[12]
//   f32         scale
//   u32         mopp_code.data_size
//   Vector4     mopp_code.offset      only from 10.1.0.0
//   enum        mopp_code.build_type only when bs_version > 34
//   byte        mopp_code.data[data_size]
//
// Verified against a shipped 20.0.0.4 file (bs_version 11, so no build_type):
// the oracle reports io_size 25845 for a data_size of 25805, and the fields
// above plus the 4-byte shape ref sum to exactly that.
constexpr quint32 kMoppOffsetVersion = 0x0A010000u;   // 10.1.0.0
constexpr quint32 kMoppBuildTypeBsVersion = 34u;

bool walkBhkMoppBvTreeShape(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!skipRefs(c, 1)) return false;                       // shape
    c.raw(12);                                              // unused_01
    c.f32();                                                // scale
    const quint32 dataSize = c.u32();
    if (!c.ok() || dataSize > 64u * 1024u * 1024u) return false;
    if (version >= kMoppOffsetVersion) c.raw(16);            // mopp_code.offset
    if (bsVersion > kMoppBuildTypeBsVersion) c.u32();        // mopp_code.build_type
    c.raw(static_cast<int>(dataSize));                      // mopp_code.data
    return c.ok();
}

// The four convex shapes all share a base of HavokMaterial + an f32 radius
// (nifgen `bhkConvexShape` -> `bhkSphereRepShape` -> `bhkConvexShapeBase`, of
// which only the first two add inline bytes). What follows is each shape's own
// fields, taken from the corresponding nifgen attribute list.
bool walkBhkSphereShape(Cursor& c, quint32 version, quint32)
{
    if (!skipHavokMaterial(c, version)) return false;
    c.f32();                                   // radius
    return c.ok();
}

bool walkBhkBoxShape(Cursor& c, quint32 version, quint32)
{
    if (!skipHavokMaterial(c, version)) return false;
    c.f32();                                   // radius
    c.raw(8);                                  // unused_01
    c.vec3();                                  // dimensions (half extents)
    c.f32();                                   // unused_float
    return c.ok();
}

bool walkBhkCapsuleShape(Cursor& c, quint32 version, quint32)
{
    if (!skipHavokMaterial(c, version)) return false;
    c.f32();                                   // radius
    c.raw(8);                                  // unused_01
    c.vec3();                                  // first_point
    c.f32();                                   // radius_1
    c.vec3();                                  // second_point
    c.f32();                                   // radius_2
    return c.ok();
}

// A BhkWorldObjCInfoProperty is three u32s: data, size, capacity_and_flags.
bool skipBhkWorldObjCInfoProperty(Cursor& c)
{
    c.raw(12);
    return c.ok();
}

bool walkBhkConvexVerticesShape(Cursor& c, quint32 version, quint32)
{
    if (!skipHavokMaterial(c, version)) return false;
    c.f32();                                   // radius
    if (!skipBhkWorldObjCInfoProperty(c)) return false;   // vertices_property
    if (!skipBhkWorldObjCInfoProperty(c)) return false;   // normals_property
    const quint32 numVertices = c.u32();
    if (!c.ok() || numVertices > 1000000u) return false;
    c.raw(static_cast<int>(numVertices) * 16);            // vertices as Vector4
    const quint32 numNormals = c.u32();
    if (!c.ok() || numNormals > 1000000u) return false;
    c.raw(static_cast<int>(numNormals) * 16);             // normals as Vector4
    return c.ok();
}

const QHash<QString, BlockWalker>& blockWalkers()
{
    // Built imperatively rather than from an initializer list: the values are
    // function pointers, which the list constructor will not take here.
    static const QHash<QString, BlockWalker> kWalkers = [] {
        QHash<QString, BlockWalker> table;
        const auto add = [&table](const char* name, BlockWalker walker) {
            table.insert(QString::fromLatin1(name), walker);
        };
        // Geometry and the object base chain.
        add("NiNode", walkNiNode);
        add("NiTriStrips", walkNiGeometry);
        add("NiTriShape", walkNiGeometry);
        add("NiTriStripsData", walkNiTriStripsData);
        // Havok collision shapes.
        add("bhkNiTriStripsShape", walkBhkNiTriStripsShape);
        add("bhkMoppBvTreeShape", walkBhkMoppBvTreeShape);
        add("bhkSphereShape", walkBhkSphereShape);
        add("bhkBoxShape", walkBhkBoxShape);
        add("bhkCapsuleShape", walkBhkCapsuleShape);
        add("bhkConvexVerticesShape", walkBhkConvexVerticesShape);
        // Extra data.
        add("BSXFlags", walkBSXFlags);
        add("NiStringExtraData", walkNiStringExtraData);
        add("NiBinaryExtraData", walkNiBinaryExtraData);
        // Animation: data, controllers, interpolators.
        add("NiTransformData", walkNiKeyframeData);
        add("NiFloatData", walkKeyGroupData);
        add("NiBoolData", walkKeyGroupData);
        add("NiPosData", walkKeyGroupData);
        add("NiColorData", walkKeyGroupData);
        add("NiTransformController", walkNiTransformController);
        add("NiTransformInterpolator", walkNiTransformInterpolator);
        return table;
    }();
    return kWalkers;
}

} // namespace

bool NifBlockFile::isBethesdaNif(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    const QByteArray head = file.read(40);
    return head.startsWith("Gamebryo File Format");
}

bool NifBlockFile::load(const QString& path)
{
    auto fail = [this, &path](const QString& reason) {
        mLastError = reason;
        LOG_ERROR(QString("NifBlockFile: %1 (%2)").arg(reason, path));
        return false;
    };
    mLastError.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return fail(QStringLiteral("cannot open file"));
    const QByteArray raw = file.readAll();
    file.close();
    if (raw.size() < 64) return fail(QStringLiteral("file too small to be a NIF"));

    // The header gained a trailing u32 after the author string in the
    // Starfield-era headers, and older games (Skyrim 1.5, bsVersion 100) do
    // not have it. Guessing wrong desynchronises every later field, so both
    // layouts are tried and the one that yields a self-consistent file wins.
    QStringList errors;
    for (int variant = 0; variant < 2; ++variant) {
        const bool hasUnknownInt = (variant == 1);
        QString error;
        if (parse(raw, hasUnknownInt, error)) return true;
        errors.append(QStringLiteral("[%1] %2")
                          .arg(hasUnknownInt ? QStringLiteral("with") : QStringLiteral("without"),
                               error));
        reset();
    }
    return fail(errors.join(QStringLiteral("; ")));
}

void NifBlockFile::reset()
{
    mHeaderVersion.clear();
    mHeaderLine.clear();
    mBlockTypes.clear();
    mTypeIndex.clear();
    mBlockSize.clear();
    mStrings.clear();
    mGroupIds.clear();
    mBlocks.clear();
    mBlockRegion.clear();
    mTrailing.clear();
    mWalkError.clear();
}

bool NifBlockFile::parse(const QByteArray& raw, bool hasUnknownInt, QString& error)
{
    auto bad = [&error](const QString& reason) {
        error = reason;
        return false;
    };

    Cursor c(raw);

    // Version line, NUL-free, terminated by '\n'.
    QByteArray magic;
    while (magic.size() < 128) {
        const quint8 ch = c.u8();
        if (!c.ok() || ch == '\n') break;
        magic.append(static_cast<char>(ch));
    }
    // The line is "Gamebryo File Format, Version <major>.<minor>.<patch>.<build>".
    // Matching one exact string was wrong: Skyrim ships 20.2.0.7, Oblivion
    // meshes 20.0.0.4, and some files in the same archive 10.1.0.101 /
    // 10.1.0.106 / 10.2.0.0. Only the shape is stable, so match the shape and
    // let the rest of the parse prove the file is consistent.
    static const QByteArray kMagicPrefix = "Gamebryo File Format, Version ";
    static const QRegularExpression kVersionPattern(
        QStringLiteral("^Gamebryo File Format, Version "
                       "(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,5})$"));
    if (!magic.startsWith(kMagicPrefix))
        return bad(QStringLiteral("unrecognised header line '%1'").arg(QString::fromLatin1(magic.left(60))));
    const QRegularExpressionMatch match = kVersionPattern.match(QString::fromLatin1(magic));
    if (!match.hasMatch())
        return bad(QStringLiteral("unrecognised version line '%1'").arg(QString::fromLatin1(magic.left(60))));
    mHeaderVersion = QStringLiteral("%1.%2.%3.%4")
                          .arg(match.captured(1), match.captured(2),
                               match.captured(3), match.captured(4));
    // Re-emit the line byte for byte rather than rebuilding it from the parsed
    // numbers, so a re-save cannot silently restamp the file's version.
    mHeaderLine = raw.left(c.pos());

    mVersion = c.u32();
    if (mVersion >= kEndianFieldVersion) {
        const quint8 endian = c.u8();
        if (!c.ok() || endian != 1)
            return bad(QStringLiteral("unsupported byte order %1").arg(endian));
    }

    mUserVersion = c.u32();
    const quint32 numBlocks = c.u32();
    mBsVersion = c.u32();
    if (!c.ok() || numBlocks == 0 || numBlocks > kMaxBlocks || mBsVersion == 0)
        return bad(QStringLiteral("implausible header: blocks=%1 bsVersion=%2")
                       .arg(numBlocks).arg(mBsVersion));

    // bs_header holds bs_version (already read) then a run of ExportStrings
    // whose membership depends on bs_version: unknown_int only above 130,
    // max_filepath only from 103. Both variants are tried by the caller, so
    // guessing wrong here only costs one retry.
    mAuthor = readExportString(c);
    mHasUnknownInt = hasUnknownInt;
    if (hasUnknownInt) mUnknownInt = c.u32();
    mExportScript = readExportString(c);
    mMaxFilepath = readExportString(c);
    if (!c.ok()) return bad(QStringLiteral("truncated export strings"));

    const quint16 numTypes = c.u16();
    if (!c.ok() || numTypes == 0 || numTypes > kMaxBlockTypes)
        return bad(QStringLiteral("implausible block type count %1").arg(numTypes));
    mBlockTypes.clear();
    for (quint16 i = 0; i < numTypes; ++i) {
        const quint32 len = c.u32();
        if (!c.ok() || len > 128) return bad(QStringLiteral("bad block type name length"));
        mBlockTypes.append(QString::fromLatin1(c.raw(len)));
    }
    if (!c.ok()) return bad(QStringLiteral("truncated block type table"));

    mTypeIndex.clear();
    mBlockSize.clear();
    mTypeIndex.reserve(numBlocks);
    mBlockSize.reserve(numBlocks);
    for (quint32 i = 0; i < numBlocks; ++i) {
        const quint16 ti = c.u16();
        if (!c.ok() || ti >= numTypes)
            return bad(QStringLiteral("block %1 has out-of-range type index %2").arg(i).arg(ti));
        mTypeIndex.append(ti);
    }
    if (!c.ok()) return bad(QStringLiteral("truncated block type index table"));

    const bool hasSizeTable = mVersion >= kBlockSizeTableVersion;
    if (hasSizeTable) {
        for (quint32 i = 0; i < numBlocks; ++i)
            mBlockSize.append(c.u32());
        if (!c.ok()) return bad(QStringLiteral("truncated block size table"));
    }

    if (mVersion >= kStringTableVersion) {
        const quint32 numStrings = c.u32();
        mMaxStringLen = c.u32();
        if (!c.ok() || numStrings > kMaxStrings || mMaxStringLen > kMaxStringLength)
            return bad(QStringLiteral("implausible string table: %1 strings, max %2")
                           .arg(numStrings).arg(mMaxStringLen));
        mStrings.clear();
        for (quint32 i = 0; i < numStrings; ++i) {
            const quint32 len = c.u32();
            if (!c.ok() || len > mMaxStringLen + 1)
                return bad(QStringLiteral("string %1 has bad length %2 (max %3)")
                               .arg(i).arg(len).arg(mMaxStringLen));
            mStrings.append(QString::fromLatin1(c.raw(len)));
        }
        if (!c.ok()) return bad(QStringLiteral("truncated string table"));
    }

    const quint32 numGroups = c.u32();
    if (!c.ok() || numGroups > kMaxGroups)
        return bad(QStringLiteral("implausible group count %1").arg(numGroups));
    mGroupIds.clear();
    for (quint32 i = 0; i < numGroups; ++i)
        mGroupIds.append(c.u32());
    if (!c.ok()) return bad(QStringLiteral("truncated group table"));

    if (!hasSizeTable) {
        // No size table. Try to recover the block boundaries by walking each
        // block's own fields, the way the reference reader does. Only a walk
        // that lands exactly on the trailing root table is accepted, so an
        // unknown or misread type falls back to one opaque region rather than
        // shifting every later block.
        if (mVersion >= kWalkVersionFloor && splitBlockRegion(raw, c.pos(), mWalkError)) {
            LOG_INFO(QString("NifBlockFile: walked %1 blocks, %2 string(s), "
                             "%3 group(s), bsVersion %4")
                         .arg(mBlocks.size()).arg(mStrings.size())
                         .arg(mGroupIds.size()).arg(mBsVersion));
            return true;
        }
        mBlocks.clear();
        mBlockSize.clear();
        mBlockRegion = raw.mid(c.pos());
        LOG_INFO(QString("NifBlockFile: %1 blocks in one opaque region (%2 bytes), "
                         "%3 group(s), bsVersion %4%5")
                     .arg(numBlocks).arg(mBlockRegion.size())
                     .arg(mGroupIds.size()).arg(mBsVersion)
                     .arg(mWalkError.isEmpty() ? QString()
                                     : QStringLiteral(" (%1)").arg(mWalkError)));
        return true;
    }

    mBlocks.clear();
    mBlocks.reserve(numBlocks);
    for (quint32 i = 0; i < numBlocks; ++i) {
        Block block;
        block.type = mBlockTypes.at(mTypeIndex.at(i));
        block.data = c.raw(static_cast<int>(mBlockSize.at(i)));
        if (!c.ok())
            return bad(QStringLiteral("block %1 (%2) extends past end of file")
                           .arg(i).arg(block.type));
        mBlocks.append(block);
    }

    // Block payloads are concatenated in index order. Some generations append
    // a 4-byte footer per block after them and some do not; whichever the
    // file has is preserved verbatim, so a re-save is byte-identical.
    const qint64 remaining = raw.size() - c.pos();
    mHasFooter = remaining == static_cast<qint64>(numBlocks) * 4;
    for (quint32 i = 0; i < numBlocks; ++i)
        mBlocks[i].footer = mHasFooter ? c.raw(4) : QByteArray(4, '\0');
    mTrailing = raw.mid(c.pos());

    LOG_INFO(QString("NifBlockFile: %1 blocks, %2 strings, %3 group(s), bsVersion %4")
                 .arg(mBlocks.size()).arg(mStrings.size()).arg(mGroupIds.size())
                 .arg(mBsVersion));
    return true;
}

bool NifBlockFile::splitBlockRegion(const QByteArray& raw, int startPos, QString& error)
{
    error.clear();
    Cursor c(raw, startPos);
    const quint32 numBlocks = static_cast<quint32>(mTypeIndex.size());
    const QHash<QString, BlockWalker>& walkers = blockWalkers();

    QVector<Block> blocks;
    QVector<quint32> sizes;
    blocks.reserve(static_cast<int>(numBlocks));
    sizes.reserve(static_cast<int>(numBlocks));

    for (quint32 i = 0; i < numBlocks; ++i) {
        const QString type = mBlockTypes.at(mTypeIndex.at(i));
        if (mVersion <= kBlockDummyVersion && !type.startsWith(QStringLiteral("bhk"))) {
            const quint32 tag = c.u32();
            if (!c.ok() || tag != 0) {
                error = QStringLiteral("block %1 (%2) is not preceded by a zero tag "
                                       "(got 0x%3)")
                            .arg(i).arg(type).arg(tag, 8, 16, QChar('0'));
                return false;
            }
        }
        const int start = c.pos();
        c.note.clear();
        const auto walker = walkers.constFind(type);
        if (walker == walkers.constEnd()) {
            error = QStringLiteral("no payload layout for block type %1").arg(type);
            return false;
        }
        if (!(*walker)(c, mVersion, mBsVersion) || !c.ok()) {
            error = QStringLiteral("block %1 (%2) could not be walked: read %3 byte(s) "
                                   "from offset %4 %5")
                        .arg(i).arg(type).arg(c.pos() - start).arg(start).arg(c.note);
            return false;
        }
        Block block;
        block.type = type;
        block.data = raw.mid(start, c.pos() - start);
        block.footer = QByteArray();
        blocks.append(block);
        sizes.append(static_cast<quint32>(block.data.size()));
    }

    // The region must end with the root table and nothing else. Requiring an
    // exact match is what makes the walk trustworthy: a layout that is subtly
    // wrong lands in the wrong place and is rejected here, not accepted.
    const int footerStart = c.pos();
    const quint32 numRoots = c.u32();
    if (!c.ok() || numRoots > numBlocks) {
        error = QStringLiteral("implausible root count %1 after the last block")
                    .arg(numRoots);
        return false;
    }
    // num_roots has just been consumed, so exactly one u32 per root is left.
    const qint64 rootBytes = 4 * static_cast<qint64>(numRoots);
    if (raw.size() - c.pos() != rootBytes) {
        error = QStringLiteral("walk ended at %1 leaving %2 byte(s), expected %3 "
                               "root ref(s)")
                    .arg(c.pos()).arg(raw.size() - c.pos()).arg(numRoots);
        return false;
    }
    for (quint32 r = 0; r < numRoots; ++r) {
        const quint32 root = c.u32();
        if (!c.ok() || (root != 0xFFFFFFFFu && root >= numBlocks)) {
            error = QStringLiteral("root table entry %1 is out of range").arg(r);
            return false;
        }
    }

    mBlocks = blocks;
    mBlockSize = sizes;
    // The root table is part of the block region and has to be written back, so
    // it is kept from num_roots onwards rather than from after the refs.
    mTrailing = raw.mid(footerStart);
    mHasFooter = false;
    return true;
}

void NifBlockFile::setBlockData(int index, const QByteArray& data)
{
    if (index < 0 || index >= mBlocks.size()) return;
    mBlocks[index].data = data;
    mBlockSize[index] = static_cast<quint32>(data.size());
}

int NifBlockFile::findBlock(const QString& typeName) const
{
    for (int i = 0; i < mBlocks.size(); ++i)
        if (mBlocks.at(i).type == typeName) return i;
    return -1;
}

QList<int> NifBlockFile::findBlocks(const QString& typeName) const
{
    QList<int> found;
    for (int i = 0; i < mBlocks.size(); ++i)
        if (mBlocks.at(i).type == typeName) found.append(i);
    return found;
}

QByteArray NifBlockFile::serialize() const
{
    QByteArray out;
    out.append(mHeaderLine);
    appendU32(out, mVersion);
    if (mVersion >= kEndianFieldVersion) appendU8(out, 1);
    appendU32(out, mUserVersion);
    appendU32(out, static_cast<quint32>(mTypeIndex.size()));
    appendU32(out, mBsVersion);
    out.append(mAuthor);
    if (mHasUnknownInt) appendU32(out, mUnknownInt);
    out.append(mExportScript);
    out.append(mMaxFilepath);

    appendU16(out, static_cast<quint16>(mBlockTypes.size()));
    for (const QString& type : mBlockTypes) {
        const QByteArray bytes = type.toLatin1();
        appendU32(out, static_cast<quint32>(bytes.size()));
        out.append(bytes);
    }
    for (quint16 ti : mTypeIndex) appendU16(out, ti);

    const bool hasSizeTable = mVersion >= kBlockSizeTableVersion;
    if (hasSizeTable) {
        for (quint32 size : mBlockSize) appendU32(out, size);
    }

    if (mVersion >= kStringTableVersion) {
        appendU32(out, static_cast<quint32>(mStrings.size()));
        appendU32(out, mMaxStringLen);
        for (const QString& s : mStrings) {
            const QByteArray bytes = s.toLatin1();
            appendU32(out, static_cast<quint32>(bytes.size()));
            out.append(bytes);
        }
    }

    appendU32(out, static_cast<quint32>(mGroupIds.size()));
    for (quint32 id : mGroupIds) appendU32(out, id);

    if (!hasSizeTable) {
        // Either the region was never split, in which case it is emitted whole,
        // or it was split and the blocks plus the root table reproduce it.
        if (!mBlockRegion.isEmpty()) {
            out.append(mBlockRegion);
            return out;
        }
        for (const Block& block : mBlocks)
            out.append(block.data);
        out.append(mTrailing);
        return out;
    }

    // All block payloads first, then the per-block footer array: the footer
    // is a separate trailing table, not a per-block suffix.
    for (const Block& block : mBlocks)
        out.append(block.data);
    if (mHasFooter) {
        for (const Block& block : mBlocks)
            out.append(block.footer);
    }
    out.append(mTrailing);
    return out;
}

bool NifBlockFile::save(const QString& path) const
{
    const QByteArray bytes = serialize();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("NifBlockFile: cannot open %1 for writing").arg(path));
        return false;
    }
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        LOG_ERROR(QString("NifBlockFile: failed to write %1 (%2)")
                      .arg(path, file.errorString()));
        return false;
    }
    return true;
}

// --- Keyframe codec --------------------------------------------------------

bool NifBlockFile::decodeKeyframeData(const QString& blockType, const QByteArray& data,
                                      QVector<Nif::TransformKeyframe>& out)
{
    out.clear();
    Cursor c(data);

    if (blockType == QLatin1String("NiKeyframeData")
        || blockType == QLatin1String("NiAnimKeyFrameData")) {
        // Skyrim 1.6+/Fallout 4/Starfield: a flat 44-byte-per-key array.
        const quint32 numKeys = c.u32();
        if (!c.ok() || numKeys > 10'000'000u) return false;
        out.reserve(static_cast<int>(numKeys));
        for (quint32 i = 0; i < numKeys; ++i) {
            Nif::TransformKeyframe key;
            key.time = c.f32();
            key.translation = c.vec3();
            key.rotation = c.quat();
            key.rotation.time = key.time;
            key.scale = c.vec3();
            if (!c.ok()) return false;
            out.append(key);
        }
    } else if (blockType == QLatin1String("NiTransformData")
               || blockType == QLatin1String("NiKeyframeControllerData")) {
        // Skyrim LE/Oblivion: three independent channels, and the first key of
        // each channel carries no time (it is implicitly zero).
        const quint32 numTranslation = c.u32();
        if (!c.ok() || numTranslation > 10'000'000u) return false;
        QVector<Nif::Vector3Keyframe> translations;
        translations.reserve(static_cast<int>(numTranslation));
        for (quint32 i = 0; i < numTranslation; ++i) {
            Nif::Vector3Keyframe key;
            key.time = (i == 0) ? 0.0f : c.f32();
            key.value = c.vec3();
            if (!c.ok()) return false;
            translations.append(key);
        }

        const quint32 numRotation = c.u32();
        if (!c.ok() || numRotation > 10'000'000u) return false;
        QVector<Nif::QuaternionKeyframe> rotations;
        rotations.reserve(static_cast<int>(numRotation));
        for (quint32 i = 0; i < numRotation; ++i) {
            Nif::QuaternionKeyframe key;
            key.time = (i == 0) ? 0.0f : c.f32();
            key.w = c.f32();
            key.x = c.f32();
            key.y = c.f32();
            key.z = c.f32();
            if (!c.ok()) return false;
            rotations.append(key);
        }

        const quint32 numScale = c.u32();
        if (!c.ok() || numScale > 10'000'000u) return false;
        QVector<Nif::Vector3Keyframe> scales;
        scales.reserve(static_cast<int>(numScale));
        for (quint32 i = 0; i < numScale; ++i) {
            Nif::Vector3Keyframe key;
            key.time = (i == 0) ? 0.0f : c.f32();
            key.value = c.vec3();
            if (!c.ok()) return false;
            scales.append(key);
        }

        if (translations.isEmpty() && rotations.isEmpty() && scales.isEmpty()) return true;
        const quint32 total = qMax(numTranslation, qMax(numRotation, numScale));
        for (quint32 i = 0; i < total; ++i) {
            Nif::TransformKeyframe key;
            if (i < numTranslation) {
                key.time = translations.at(i).time;
                key.translation = translations.at(i).value;
            } else {
                key.translation = {0.0f, 0.0f, 0.0f};
            }
            if (i < numRotation) {
                key.rotation = rotations.at(i);
                if (i < numTranslation) key.rotation.time = key.time;
            } else {
                key.rotation = {0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
            }
            if (i < numScale) {
                key.scale = scales.at(i).value;
            } else {
                key.scale = {1.0f, 1.0f, 1.0f};
            }
            out.append(key);
        }
    } else {
        return false;
    }

    // Strict: a keyframe block must consume itself exactly. Leftover bytes
    // mean the assumed layout is wrong for this game version.
    if (!c.atEnd()) {
        out.clear();
        return false;
    }
    return true;
}

bool NifBlockFile::isWritableKeyframeType(const QString& blockType)
{
    return blockType == QLatin1String("NiKeyframeData")
        || blockType == QLatin1String("NiAnimKeyFrameData");
}

bool NifBlockFile::encodeKeyframeData(const QString& blockType,
                                      const QVector<Nif::TransformKeyframe>& keyframes,
                                      QByteArray& out)
{
    out.clear();
    if (keyframes.isEmpty()) return false;

    if (blockType == QLatin1String("NiKeyframeData")
        || blockType == QLatin1String("NiAnimKeyFrameData")) {
        appendU32(out, static_cast<quint32>(keyframes.size()));
        for (const Nif::TransformKeyframe& key : keyframes) {
            appendF32(out, key.time);
            appendVec3(out, key.translation);
            appendQuat(out, key.rotation);
            appendVec3(out, key.scale);
        }
        return true;
    }

    if (blockType == QLatin1String("NiTransformData")
        || blockType == QLatin1String("NiKeyframeControllerData")) {
        // The three channels must be monotonic in time; a keyframe list whose
        // channels disagree would not reload as the same animation.
        appendU32(out, static_cast<quint32>(keyframes.size()));
        for (int i = 0; i < keyframes.size(); ++i) {
            if (i > 0) appendF32(out, keyframes.at(i).time);
            appendVec3(out, keyframes.at(i).translation);
        }
        appendU32(out, static_cast<quint32>(keyframes.size()));
        for (int i = 0; i < keyframes.size(); ++i) {
            if (i > 0) appendF32(out, keyframes.at(i).time);
            appendQuat(out, keyframes.at(i).rotation);
        }
        appendU32(out, static_cast<quint32>(keyframes.size()));
        for (int i = 0; i < keyframes.size(); ++i) {
            if (i > 0) appendF32(out, keyframes.at(i).time);
            appendVec3(out, keyframes.at(i).scale);
        }
        return true;
    }

    return false;
}


bool NifBlockFile::isNodeBlockType(const QString& blockType)
{
    // NiObjectNET descendants seen in shipped files. Matching on the type is
    // what stops a data block from being mistaken for a node.
    static const QStringList kNodeTypes = {
        QStringLiteral("NiNode"),
        QStringLiteral("NiLODNode"),
        QStringLiteral("NiBillboardNode"),
        QStringLiteral("NiTriShape"),
        QStringLiteral("NiTriStripshape"),
        QStringLiteral("BSTriShape"),
        QStringLiteral("BSGeometry"),
        QStringLiteral("BSLODTriShape"),
        QStringLiteral("BSFadeNode"),
        QStringLiteral("BSLODNode"),
        QStringLiteral("BSFaceGenNiNode"),
        QStringLiteral("BSParentlessChild"),
        QStringLiteral("BSSearchLightNode"),
        QStringLiteral("BSPortalNode"),
        QStringLiteral("NiCameraNode"),
        QStringLiteral("NiSwitchNode"),
        QStringLiteral("NiLODSwitchNode"),
    };
    return kNodeTypes.contains(blockType);
}

bool NifBlockFile::nodeNetInfo(int index, QString& nameOut, quint32& controllerRefOut) const
{
    nameOut.clear();
    controllerRefOut = 0xFFFFFFFFu;
    if (index < 0 || index >= mBlocks.size()) return false;
    if (!isNodeBlockType(mBlocks.at(index).type)) return false;
    const QByteArray& data = mBlocks.at(index).data;
    Cursor c(data);
    const quint32 nameIndex = c.u32();
    if (!c.ok() || nameIndex >= static_cast<quint32>(mStrings.size())) return false;
    const quint32 numExtra = c.u32();
    if (!c.ok() || numExtra > 100000u) return false;
    for (quint32 i = 0; i < numExtra; ++i) {
        const quint32 ref = c.u32();
        if (!c.ok() || (ref != 0xFFFFFFFFu && ref >= static_cast<quint32>(mBlocks.size())))
            return false;
    }
    const quint32 controller = c.u32();
    if (!c.ok()) return false;
    nameOut = mStrings.at(static_cast<int>(nameIndex));
    controllerRefOut = controller;
    return true;
}

namespace {

// Trailing u32 of a block: the keyframe data / interpolator ref in both the
// 1.5 and 1.6+ controller layouts.
bool trailingRef(const QByteArray& data, quint32& ref)
{
    if (data.size() < 4) return false;
    const int base = data.size() - 4;
    ref = static_cast<quint32>(static_cast<quint8>(data.at(base)))
        | static_cast<quint32>(static_cast<quint8>(data.at(base + 1))) << 8
        | static_cast<quint32>(static_cast<quint8>(data.at(base + 2))) << 16
        | static_cast<quint32>(static_cast<quint8>(data.at(base + 3))) << 24;
    return true;
}

} // namespace

int NifBlockFile::keyframeDataBlockFor(int controllerIndex) const
{
    if (controllerIndex < 0 || controllerIndex >= mBlocks.size()) return -1;
    quint32 ref = 0xFFFFFFFFu;
    if (!trailingRef(mBlocks.at(controllerIndex).data, ref)) return -1;
    if (ref >= static_cast<quint32>(mBlocks.size())) return -1;

    // 1.5 inserts an interpolator between the controller and the data.
    const QString firstType = mBlocks.at(ref).type;
    if (firstType == QLatin1String("NiTransformInterpolator")
        || firstType == QLatin1String("NiQuatKeyframeController")) {
        quint32 inner = 0xFFFFFFFFu;
        if (!trailingRef(mBlocks.at(ref).data, inner)) return -1;
        if (inner >= static_cast<quint32>(mBlocks.size())) return -1;
        ref = inner;
    }

    const QString dataType = mBlocks.at(ref).type;
    if (dataType == QLatin1String("NiKeyframeData")
        || dataType == QLatin1String("NiAnimKeyFrameData")
        || dataType == QLatin1String("NiTransformData"))
        return static_cast<int>(ref);
    return -1;
}

QHash<quint32, QString> NifBlockFile::clipNamesByController() const
{
    QHash<quint32, QString> names;
    const QList<int> sequences = findBlocks(QStringLiteral("NiControllerSequence"));
    for (int index : sequences) {
        QList<QPair<quint32, QString>> entries;
        if (!decodeControllerSequence(block(index).data, entries)) continue;
        for (const auto& entry : entries) {
            if (entry.first < static_cast<quint32>(count())) names.insert(entry.first, entry.second);
        }
    }
    return names;
}

bool NifBlockFile::decodeControllerSequence(const QByteArray& data,
                                            QList<QPair<quint32, QString>>& out)
{
    out.clear();
    Cursor c(data);
    const quint32 numSequences = c.u32();
    if (!c.ok() || numSequences > 100000u) return false;
    for (quint32 i = 0; i < numSequences; ++i) {
        const quint32 controllerRef = c.u32();
        const quint8 nameLen = c.u8();
        if (!c.ok() || nameLen > 250) return false;
        const QString name = QString::fromLatin1(c.raw(nameLen));
        if (!c.ok()) return false;
        out.append(qMakePair(controllerRef, name));
    }
    return c.atEnd();
}

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
// A MaterialData: from 10.0.0.0 to 20.1.0.3 it is a single flag, and only when
// that flag is set does a shader name and an extra-data int follow. From
// 20.1.0.15 it becomes a material-name list, which is not walked here.
constexpr quint32 kMaterialDataFirstVersion = 167772416u;  // 10.0.0.0
constexpr quint32 kMaterialDataLastVersion = 335609859u;    // 20.1.0.3

bool skipMaterialData(Cursor& c, quint32 version)
{
    if (version >= 335675397u) return false;   // material list: unmeasured
    if (version < kMaterialDataFirstVersion
        || version > kMaterialDataLastVersion)
        return c.ok();
    if (c.u8()) {                              // has_shader
        if (!skipString(c, version)) return false;
        c.u32();                               // shader_extra_data
    }
    return c.ok();
}

// NiTriShape and NiTriStrips - both NiTriBasedGeom - carry a data ref, a skin
// ref and a MaterialData.
//
// This previously read a NiGeometry bounding sphere (centre and radius) plus a
// skin ref, which is 20 bytes where the real payload is 9 - eleven bytes too
// many. Because a pre-20.2.0.5 container records no block lengths, every block
// after the first NiTriShape in a file landed eleven bytes early, and the walk
// then failed wherever it happened to notice, blaming whichever type it was
// reading at the time. That is what the several thousand "could not be walked"
// failures were. The bounding sphere only exists for a 20.2.0.7 particle-system
// special case, not for the tri-based geometry.
bool walkNiTriBasedGeom(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiAVObject(c, version, bsVersion)) return false;
    if (!skipRefs(c, 1)) return false;    // data
    if (!skipRefs(c, 1)) return false;    // skin_instance
    return skipMaterialData(c, version);
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
// The prefix on every time-controller-based block: a next-controller ref, a
// flags word, and the four timing floats, then a target ref.
//
// The flags are a u16. Decoded from a shipped 20.0.0.4
// NiMultiTargetTransformController, whose forty bytes are
// `ff ff ff ff | 2c 00 | 00 00 80 3f | 00 00 00 00 | ff ff 7f 7f | ff ff 7f ff |
// 00 00 00 00 | 03 00 | ...` - the 0x2c flags, 1.0 frequency, 0.0 phase and the
// two clamped times only line up if the flags occupy two bytes. As a u32 every
// one of those floats is read two bytes early.
constexpr int kTimeControllerBytes = 26;

bool walkNiTimeController(Cursor& c)
{
    c.u32();    // next_controller
    c.u16();    // flags
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

// A HavokFilter is a single u32 on the wire. nifgen decomposes it into layer,
// flags and group for editing, but its stored size is 4, and reading the three
// parts separately over-reads by two bytes per filter.
bool skipHavokFilter(Cursor& c) { c.u32(); return c.ok(); }

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
        if (!skipHavokFilter(c)) return false;
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

bool walkBhkConvexVerticesShape(Cursor& c, quint32 version, quint32){
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

// --- Havok objects with an entity prefix -------------------------------------
//
// A BhkWorldObjCInfoProperty is three u32s: data, size, capacity_and_flags.

// A BhkWorldObjectCInfo, and the world-object prefix in front of it.
//
// The 20 bytes here were read field by field out of a shipped 20.0.0.4 file
// rather than summed from the attribute list, because two of the enums are
// single bytes. At the offset where this struct starts the bytes are
// `60 af e5 04 | 01 | a2 37 7c | 00 00 00 00 | 00 00 00 00 | 00 00 00 80`:
// unused_01, then broad_phase_type = 0x01 as a *byte*, then three bytes of
// unused_02, then the property's data, size and capacity_and_flags. Reading
// broad_phase_type as a u32 instead consumes the unused bytes and lands four
// bytes off, which is enough to fail every file that carries one.
bool skipHavokWorldObjectInfo(Cursor& c)
{
    c.raw(4);                                  // unused_01
    c.u8();                                    // broad_phase_type
    c.raw(3);                                  // unused_02
    c.raw(12);                                 // property (3 x u32)
    return c.ok();
}

// The prefix every BhkWorldObject, and therefore every BhkEntity, carries.
bool skipHavokWorldObject(Cursor& c, quint32 version)
{
    if (!skipRefs(c, 1)) return false;               // shape
    // unknown_int is only in the file for the oldest versions that have it.
    if (version <= kHavokMaterialUnknownIntVersion) c.u32();
    if (!skipHavokFilter(c)) return false;           // havok_filter
    return skipHavokWorldObjectInfo(c);
}

// A BhkEntityCInfo: a byte of response, a byte of unused, a u16 delay. Verified
// against the same file, where the entity prefix ends in `01 ff ff ff`.
bool skipHavokEntityInfo(Cursor& c)
{
    c.raw(4);
    return c.ok();
}

bool skipHavokEntity(Cursor& c, quint32 version)
{
    if (!skipHavokWorldObject(c, version)) return false;
    return skipHavokEntityInfo(c);
}

// The rigid body info block, whose layout is selected by the bs_version in the
// header rather than the file version.
constexpr quint32 kRigidBodyCInfo2010BsVersion = 83u;
constexpr quint32 kRigidBodyCInfo2014BsVersion = 130u;
constexpr quint32 kRigidBodyFlagsBsVersion = 76u;

// bhkRigidBodyCInfo550660, the layout used through bs_version 34 - which covers
// every Oblivion and Skyrim mesh. 196 bytes at 20.0.0.4, measured: the block's
// rigid_body_info sits at 103040 and the block ends 196 bytes later.
//
// Leading group (unused_01, filter, unused_02) is only in the file from
// 10.1.0.0 on. The tail is fixed: 116 bytes of transform and inertia, eight
// floats, four u32 enums, and 12 unused bytes.
bool skipRigidBodyCInfo550660(Cursor& c, quint32 version)
{
    if (version >= kMoppOffsetVersion) {
        c.raw(4);                              // unused_01
        if (!skipHavokFilter(c)) return false; // havok_filter
        c.raw(4);                              // unused_02
    }
    c.raw(1);                                  // collision_response (a byte)
    c.raw(1);                                  // unused_03
    c.raw(2);                                  // process_contact_callback_delay
    c.raw(4);                                  // unused_04
    c.raw(116);                                // translation, rotation, velocities,
                                              // inertia tensor, centre of mass
    c.raw(32);                                 // mass and seven floats
    c.raw(16);                                 // four u32 enums
    c.raw(12);                                 // unused_05
    return c.ok();
}

// bhkRigidBody, and bhkRigidBodyT with it - the latter adds no fields of its
// own, only a different type name, so it shares this walker.
bool walkBhkRigidBody(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!skipHavokEntity(c, version)) return false;
    if (bsVersion <= kMoppBuildTypeBsVersion) {
        if (!skipRigidBodyCInfo550660(c, version)) return false;
    } else if (bsVersion >= kRigidBodyCInfo2010BsVersion
               && bsVersion != kRigidBodyCInfo2014BsVersion) {
        // BhkRigidBodyCInfo2010 has not been measured against shipped bytes, so
        // decline rather than guess: a wrong length would misread every block
        // after this one. The opaque fallback keeps the file readable.
        return false;
    } else if (bsVersion == kRigidBodyCInfo2014BsVersion) {
        return false;
    }
    const quint32 numConstraints = c.u32();
    if (!c.ok() || numConstraints > 100000u) return false;
    if (!skipRefs(c, numConstraints)) return false;
    if (bsVersion < kRigidBodyFlagsBsVersion) c.u32();    // body_flags
    else c.u16();
    return c.ok();
}

// The bhk collision objects - bhkCollisionObject and its SP/P/Blend/NP
// variants - differ only in the default they give the flags field, so they all
// share one layout and one walker.
//
// Ten bytes, read from a shipped 20.0.0.4 file where the block is
// `00 00 00 00 | 01 00 | 06 00 00 00`: a ref to the target node, the flags, and
// a ref to the body. The flags are a u16, not the u32 the attribute list's type
// name suggests, and the block is two bytes short of the twelve that u32 would
// imply.
bool walkBhkCollisionObject(Cursor& c, quint32, quint32)
{
    if (!skipRefs(c, 1)) return false;          // target
    c.u16();                                   // flags
    if (!skipRefs(c, 1)) return false;          // body
    return c.ok();
}

// --- Property and controller blocks ------------------------------------------

// NiMaterialProperty: the NiObjectNET prefix, then the colours. The 20-byte
// prefix is the name, a zero extra-data count and a null controller, which is
// how every property block starts.
//
// Verified against a shipped 20.0.0.4 file: 20 bytes of prefix, four Color3s at
// 12 bytes each, then glossiness and alpha, for 80 bytes total - the oracle's
// io_size exactly. flags is only in the file for the 3.x-10.0.1.2 range, and
// emissive_mult only once bs_version passes 21, which Oblivion at bs_version 11
// does not reach.
bool walkNiMaterialProperty(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiObjectNET(c, version)) return false;
    if (version >= 50331648u && version <= 167772418u) c.u16();   // flags
    if (bsVersion < 26) {
        c.raw(12);                                              // ambient_color
        c.raw(12);                                              // diffuse_color
    }
    c.raw(12);                                                  // specular_color
    c.raw(12);                                                  // emissive_color
    c.f32();                                                    // glossiness
    c.f32();                                                    // alpha
    if (bsVersion > 21) c.f32();                                // emissive_mult
    return c.ok();
}

// NiStencilProperty. Through 20.0.0.5 it is an enabled byte, a test function, a
// reference and a mask, then three actions and a draw mode. From 20.1.0.3 the
// whole set collapses into a packed flags word plus a reference.
constexpr quint32 kStencilPackedVersion = 335609859u;   // 20.1.0.3

bool walkNiStencilProperty(Cursor& c, quint32 version, quint32)
{
    if (!walkNiObjectNET(c, version)) return false;
    if (version >= kStencilPackedVersion) {
        c.u32();      // flags
        c.u32();      // stencil_ref
        return c.ok();
    }
    if (version <= 167772418u) c.u16();   // flags
    c.u8();                                // stencil_enabled
    c.u32();                               // stencil_function
    c.u32();                               // stencil_ref
    c.u32();                               // stencil_mask
    c.u32();                               // fail_action
    c.u32();                               // z_fail_action
    c.u32();                               // pass_action
    c.u32();                               // draw_mode
    return c.ok();
}

// NiControllerManager: the time-controller prefix, a cumulative byte, the
// sequence refs, and a palette ref.
//
// 43 bytes in a shipped 20.0.0.4 file: 26 of time controller, one byte of
// cumulative, a u32 sequence count, two refs and a palette ref.
bool walkNiControllerManager(Cursor& c, quint32, quint32)
{
    if (!walkNiTimeController(c)) return false;
    c.u8();                                 // cumulative
    const quint32 numSequences = c.u32();
    if (!c.ok() || numSequences > 10000u) return false;
    if (!skipRefs(c, numSequences)) return false;
    if (!skipRefs(c, 1)) return false;       // object_palette
    return c.ok();
}

// NiMultiTargetTransformController: the time-controller prefix, a u16 count of
// extra targets, and that many refs. The count is a u16, not a u32 - the same
// two-byte lesson as the flags, and the same forty-byte file confirms it.
bool walkNiMultiTargetTransformController(Cursor& c, quint32, quint32)
{
    if (!walkNiTimeController(c)) return false;
    const quint16 numExtraTargets = c.u16();
    if (!c.ok()) return false;
    if (!skipRefs(c, numExtraTargets)) return false;
    return c.ok();
}

// NiTextKeyExtraData: a name, a key count, then that many (time, value) pairs.
//
// 63 bytes in a shipped file: four of empty name, a u32 count of three, and
// three keys of 13, 31 and 11 bytes - each a time float, a u32 string length,
// and that many characters.
bool walkNiTextKeyExtraData(Cursor& c, quint32 version, quint32)
{
    if (!walkNiExtraData(c, version)) return false;
    const quint32 numKeys = c.u32();
    if (!c.ok() || numKeys > 100000u) return false;
    for (quint32 i = 0; i < numKeys; ++i) {
        c.f32();                                    // time
        if (!skipString(c, version)) return false;   // value
    }
    return c.ok();
}

// NiStringPalette: a count, the total length of the blob, and the blob itself.
// The blob is several NUL-terminated strings run together, so it is not a list
// of separate strings and must not be split on the NULs.
bool walkNiStringPalette(Cursor& c, quint32, quint32)
{
    c.u32();     // num_strings
    const quint32 length = c.u32();
    if (!c.ok() || length > 1024u * 1024u) return false;
    c.raw(static_cast<int>(length));
    return c.ok();
}

// NiDefaultAVObjectPalette: a scene ref, an object count, and that many names.
// The object refs themselves live in the file's global reference table, so only
// the names are inline.
bool walkNiDefaultAVObjectPalette(Cursor& c, quint32 version, quint32)
{
    if (!skipRefs(c, 1)) return false;    // scene
    const quint32 numObjs = c.u32();
    if (!c.ok() || numObjs > 100000u) return false;
    for (quint32 i = 0; i < numObjs; ++i) {
        if (!skipString(c, version)) return false;
    }
    return c.ok();
}

// BSBound: an extra-data name and a centre/dimensions pair.
bool walkBSBound(Cursor& c, quint32 version, quint32)
{
    if (!walkNiExtraData(c, version)) return false;
    c.vec3();     // center
    c.vec3();     // dimensions
    return c.ok();
}

// BSFurnitureMarker: a name, a position count, and the positions. Through
// bs_version 34 a FurniturePosition is a Vector3 offset, a u16 orientation and
// two bytes; later it becomes a heading, an animation type and entry points,
// which have not been measured here.
constexpr quint32 kFurniturePositionBsVersion = 34u;

bool walkBSFurnitureMarker(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiExtraData(c, version)) return false;
    const quint32 numPositions = c.u32();
    if (!c.ok() || numPositions > 100000u) return false;
    if (bsVersion > kFurniturePositionBsVersion) return false;
    for (quint32 i = 0; i < numPositions; ++i) {
        c.vec3();     // offset
        c.u16();      // orientation
        c.u8();       // position_ref_1
        c.u8();       // position_ref_2
    }
    return c.ok();
}

// bhkConvexTransformShape: a ref to the shape it transforms, a material, a
// radius, eight unused bytes and a 4x4 matrix.
bool walkBhkConvexTransformShape(Cursor& c, quint32 version, quint32)
{
    if (!skipRefs(c, 1)) return false;      // shape
    if (!skipHavokMaterial(c, version)) return false;
    c.f32();                                // radius
    c.raw(8);                               // unused_01
    c.raw(64);                              // transform (Matrix44)
    return c.ok();
}

// bhkListShape: a child-shape array, then a material, two child properties and a
// filter array.
bool walkBhkListShape(Cursor& c, quint32 version, quint32)
{
    const quint32 numSubShapes = c.u32();
    if (!c.ok() || numSubShapes > 100000u) return false;
    if (!skipRefs(c, numSubShapes)) return false;
    if (!skipHavokMaterial(c, version)) return false;
    if (!skipBhkWorldObjCInfoProperty(c)) return false;   // child_shape_property
    if (!skipBhkWorldObjCInfoProperty(c)) return false;   // child_filter_property
    const quint32 numFilters = c.u32();
    if (!c.ok() || numFilters > 100000u) return false;
    for (quint32 i = 0; i < numFilters; ++i) {
        if (!skipHavokFilter(c)) return false;
    }
    return c.ok();
}

// NiZBufferProperty: flags, and a test function from 4.1.0.4 up to 20.0.0.5.
constexpr quint32 kZBufferFunctionFirstVersion = 67174412u;   // 4.1.0.4
constexpr quint32 kZBufferFunctionLastVersion = 335544325u;  // 20.0.0.5

bool walkNiZBufferProperty(Cursor& c, quint32 version, quint32)
{
    if (!walkNiObjectNET(c, version)) return false;
    c.u32();                                              // flags
    if (version >= kZBufferFunctionFirstVersion
        && version <= kZBufferFunctionLastVersion)
        c.u32();                                          // function
    return c.ok();
}

// NiBillboardNode: a NiNode that carries a billboard mode from 10.1.0.0.
bool walkNiBillboardNode(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiNode(c, version, bsVersion)) return false;
    if (version >= 167837696u) c.u32();   // billboard_mode
    return c.ok();
}

// --- Havok constraints and shape phantoms --------------------------------------
//
// Every bhk constraint shares a BhkConstraintCInfo: two entity refs, a priority,
// and a count. Then each adds its own CInfo, which through bs_version 16 is a
// handful of Vector4s and a few floats, and after that reorders into pivots,
// axes and planes with the same total width.
constexpr quint32 kConstraintMotorVersion = 335675399u;   // 20.1.0.15
constexpr quint32 kConstraintOldBsVersion = 16u;

bool skipBhkConstraintCInfo(Cursor& c)
{
    c.u32();     // num_entities
    if (!skipRefs(c, 1)) return false;   // entity_a
    if (!skipRefs(c, 1)) return false;   // entity_b
    c.u32();     // priority
    return c.ok();
}

bool walkBhkLimitedHingeConstraint(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!skipBhkConstraintCInfo(c)) return false;
    if (bsVersion <= kConstraintOldBsVersion) {
        c.raw(16 * 7);        // pivots, axes and perpendicular axes
    } else {
        c.raw(16 * 8);        // the same vectors, reordered
        c.raw(12);            // min_angle, max_angle, max_friction
        if (version >= kConstraintMotorVersion) return false;  // motor: unmeasured
    }
    return c.ok();
}

bool walkBhkRagdollConstraint(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!skipBhkConstraintCInfo(c)) return false;
    if (bsVersion <= kConstraintOldBsVersion) {
        c.raw(16 * 6);        // pivot, plane and twist per entity
    } else {
        c.raw(16 * 8);        // twist, plane, motor and pivot per entity
        c.raw(24);            // six angle and friction floats
        if (version >= kConstraintMotorVersion) return false;  // motor: unmeasured
    }
    return c.ok();
}

bool walkBhkPrismaticConstraint(Cursor& c, quint32 version, quint32)
{
    if (!skipBhkConstraintCInfo(c)) return false;
    if (version <= 335544325u) {
        c.raw(16 * 8);        // pivot, rotation, plane and sliding per entity
    } else {
        c.raw(16 * 8);
        c.raw(12);            // min_distance, max_distance, friction
        if (version >= kConstraintMotorVersion) return false;  // motor: unmeasured
    }
    return c.ok();
}

// The shape phantoms differ only in the type name: eight unused bytes and a
// 4x4 transform. They need no references of their own, so the matrix is the
// whole payload.
bool walkBhkSimpleShapePhantom(Cursor& c, quint32, quint32)
{
    c.raw(8);      // unused_01
    c.raw(64);     // transform (Matrix44)
    return c.ok();
}

// NiVertexColorProperty: a flags word, and through 20.0.0.5 a vertex mode and a
// lighting mode. From 20.1.0.3 the two modes pack into the flags word.
//
// The flags are a u16 and the two modes are u32, which is 22 bytes for a
// 10.2.0.0 file: twelve of NiObjectNET, then two, four and four. Reading the
// flags as a u32 makes the block two bytes long and shifts everything after it.
bool walkNiVertexColorProperty(Cursor& c, quint32 version, quint32)
{
    if (!walkNiObjectNET(c, version)) return false;
    c.u16();                                              // flags
    if (version <= 335544325u) {
        c.u32();                                          // vertex_mode
        c.u32();                                          // lighting_mode
    }
    return c.ok();
}

// NiAlphaProperty: a flags word and a one-byte threshold. The threshold is a
// byte, not a float - a shipped 10.2.0.0 file stores 127 in it, which as a
// float would be a denormal rather than a threshold.
constexpr quint32 kAlphaPropertyOldVersion = 33751040u;    // 2.0.0.4
constexpr quint32 kAlphaPropertyStarfield = 335741185u;    // 20.2.0.9
constexpr quint32 kAlphaPropertyStarfieldEnd = 335741186u;

bool walkNiAlphaProperty(Cursor& c, quint32 version, quint32)
{
    if (!walkNiObjectNET(c, version)) return false;
    c.u16();                                              // flags
    c.u8();                                               // threshold
    if (version <= kAlphaPropertyOldVersion) {
        c.u16();                                          // unknown_short_1
        c.u32();                                          // unknown_int_2
    } else if (version >= kAlphaPropertyStarfield
               && version <= kAlphaPropertyStarfieldEnd) {
        c.u16();                                          // unknown_short_1
    }
    return c.ok();
}

// NiSpecularProperty is just a flags word; earlier versions put the specular
// colour and strength beside it, and from 20.1.0.3 the version lives inside the
// flags word instead.
bool walkNiSpecularProperty(Cursor& c, quint32 version, quint32)
{
    if (!walkNiObjectNET(c, version)) return false;
    c.u16();                                              // flags
    if (version < 335609859u) c.u32();                    // colour and strength
    return c.ok();
}

// A TexDesc: a ref to the source texture, two clamp/filter modes, a uv set, and
// a flag saying whether a texture transform follows.
//
// The uv set is a u32, not the ushort the field is declared as - the oracle
// reports TexDesc as 21 bytes with its fields starting at a known offset, and a
// u16 there leaves every block two bytes short. Reading it as a u16 is what made
// 151 files stop mid-block on this. The two modes are likewise u32 enums.
// Through 10.3.0.1 a pair of PlayStation2 fields sits between the uv set and the
// transform flag.
constexpr quint32 kTexDescPackedVersion = 335609859u;   // 20.1.0.3
constexpr quint32 kTexDescAnisotropyVersion = 335872004u;
constexpr quint32 kTexDescTransformVersion = 167837696u;  // 10.1.0.0
constexpr quint32 kTexDescLegacyEnd = 335544325u;        // 20.0.0.5
constexpr quint32 kTexDescPs2End = 168034305u;           // 10.3.0.1

bool skipTexDesc(Cursor& c, quint32 version)
{
    if (version <= 50397184u) {
        if (!skipRefs(c, 1)) return false;   // image
    } else {
        if (!skipRefs(c, 1)) return false;   // source
    }
    if (version <= kTexDescLegacyEnd) {
        c.u32();                              // clamp_mode
        c.u32();                              // filter_mode
    }
    if (version >= kTexDescPackedVersion) c.u32();          // flags
    if (version >= kTexDescAnisotropyVersion) c.u32();     // max_anisotropy
    if (version <= kTexDescLegacyEnd) c.u32();              // uv_set
    if (version <= kTexDescPs2End) {
        c.u16();                              // ps_2_l
        c.u16();                              // ps_2_k
    }
    if (version >= kTexDescTransformVersion) {
        if (c.u8()) {                          // has_texture_transform
            c.raw(8);      // translation
            c.raw(8);      // scale
            c.f32();       // rotation
            c.u32();       // transform_method
            c.raw(8);      // center
        }
    }
    return c.ok();
}

// NiTexturingProperty: an apply mode and a slot count, then per-slot flags each
// guarding a TexDesc, then the bump-map extras and the shader texture array.
//
// The slot order is base, dark, detail, gloss, glow, bump, then either the
// normal/parallax pair or four decals depending on version, and each flag is
// only written when the slot exists in the count. Reading a flag for a slot past
// the count is the difference between this landing on the next block and landing
// two bytes before it.
constexpr quint32 kTexNormalVersion = 335675397u;   // 20.1.0.15
constexpr quint32 kTexApplyModeFirstVersion = 50528269u;
constexpr quint32 kTexApplyModeLastVersion = 335609857u;

bool walkNiTexturingProperty(Cursor& c, quint32 version, quint32)
{
    if (!walkNiObjectNET(c, version)) return false;
    if (version <= 167772418u) c.u16();                              // flags
    else if (version >= kTexDescPackedVersion) c.u32();               // flags
    quint32 count = 0;
    if (version >= kTexApplyModeFirstVersion && version <= kTexApplyModeLastVersion) {
        c.u32();                                                      // apply_mode
        count = c.u32();                                              // texture_count
    } else {
        count = c.u32();
    }
    if (!c.ok() || count > 32u) return false;

    // Each of these reads a flag and, when it is set, the TexDesc behind it.
    const auto slot = [&c, version](quint32 index, bool bumpExtras) {
        if (!c.u8()) return true;                 // flag clear: nothing to read
        if (bumpExtras) {
            if (!skipTexDesc(c, version)) return false;
            c.f32();                              // bump_map_luma_scale
            c.f32();                              // bump_map_luma_offset
            c.raw(16);                            // bump_map_matrix
        }
        return skipTexDesc(c, version);
    };

    for (quint32 i = 0; i < count; ++i) {
        if (!slot(i, i == 5)) return false;
    }
    if (version >= 167772416u) {
        const quint32 numShaderTextures = c.u32();
        if (!c.ok() || numShaderTextures > 1000u) return false;
        if (!skipRefs(c, numShaderTextures)) return false;
    }
    return c.ok();
}

// A FormatPrefs is three four-byte enums: a pixel layout, a mipmap choice and
// an alpha format. nifgen models all three as boolean-like types, but the block
// is 12 bytes wide, not 3.
bool skipFormatPrefs(Cursor& c) { c.raw(12); return c.ok(); }

// NiSourceTexture: an external/internal flag, a file name, an optional pixel
// data ref, the format preferences, an is-static byte and a direct-render flag.
//
// The format preferences and the is-static byte are read whether or not the
// texture is external. Reading them only for internal textures - which the
// version-gated field list suggests, because each carries an `and
// use_external == 0` - leaves the block 13 bytes short, and this type is on
// 1,882 of the archive's files.
//
// Checked against two shipped files, one 20.0.0.4 and one 10.2.0.0, both 81
// bytes: 12 of NiObjectNET, the flag, a 50-byte file-name string, a 4-byte ref,
// 12 of format prefs, and two single bytes.
constexpr quint32 kSourceTextureRefVersion = 167772420u;   // 10.0.0.4
constexpr quint32 kSourceTextureNameVersion = 167837696u;  // 10.1.0.0
constexpr quint32 kSourceTextureDirectRender = 167837799u;
constexpr quint32 kSourceTexturePersist = 335675396u;      // 20.1.0.15

bool walkNiSourceTexture(Cursor& c, quint32 version, quint32)
{
    if (!walkNiObjectNET(c, version)) return false;
    const quint8 useExternal = c.u8();
    if (version <= 167772419u && useExternal == 0) c.u8();   // use_internal
    if (!skipString(c, version)) return false;                // file_name
    if (version >= kSourceTextureRefVersion) {
        if (!skipRefs(c, 1)) return false;                    // pixel_data
    }
    if (!skipFormatPrefs(c)) return false;                    // format_prefs
    c.u8();                                                   // is_static
    if (version >= kSourceTextureDirectRender) c.u8();        // direct_render
    if (version >= kSourceTexturePersist) {
        c.u8();                                               // persist_render_data
    }
    return c.ok();
}

// NiSkinInstance. Its base is NiObject, not NiObjectNET, so there is no name
// and no controller - reading either would shift every field by four bytes.
//
// The bones are references to NiNode blocks, not inline bone data. nifgen
// declares the array as BoneData, but the array resolves to the named NiNodes it
// points at, and a shipped 20.0.0.4 file measures 28 bytes with three bones:
// four refs, a count, and three more refs. Reading 70 bytes of transform per bone
// instead walks straight past the end of the block.
constexpr quint32 kSkinInstancePartitionVersion = 167837797u;   // 10.1.0.1

bool walkNiSkinInstance(Cursor& c, quint32 version, quint32)
{
    if (!skipRefs(c, 1)) return false;    // data
    if (version < kSkinInstancePartitionVersion) return c.ok();
    if (!skipRefs(c, 1)) return false;    // skin_partition
    if (!skipRefs(c, 1)) return false;    // skeleton_root
    const quint32 numBones = c.u32();
    if (!c.ok() || numBones > 10000u) return false;
    if (!skipRefs(c, numBones)) return false;   // bones
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
        add("NiTriStrips", walkNiTriBasedGeom);
        add("NiTriShape", walkNiTriBasedGeom);
        add("NiTriStripsData", walkNiTriStripsData);
        // Havok collision shapes.
        add("bhkNiTriStripsShape", walkBhkNiTriStripsShape);
        add("bhkMoppBvTreeShape", walkBhkMoppBvTreeShape);
        add("bhkSphereShape", walkBhkSphereShape);
        add("bhkBoxShape", walkBhkBoxShape);
        add("bhkCapsuleShape", walkBhkCapsuleShape);
        add("bhkConvexVerticesShape", walkBhkConvexVerticesShape);
        add("bhkRigidBody", walkBhkRigidBody);
        add("bhkRigidBodyT", walkBhkRigidBody);
        add("bhkCollisionObject", walkBhkCollisionObject);
        add("bhkSPCollisionObject", walkBhkCollisionObject);
        add("bhkPCollisionObject", walkBhkCollisionObject);
        add("bhkBlendCollisionObject", walkBhkCollisionObject);
        add("bhkNPCollisionObject", walkBhkCollisionObject);
        // Properties and controllers.
        add("NiMaterialProperty", walkNiMaterialProperty);
        add("NiStencilProperty", walkNiStencilProperty);
        add("NiControllerManager", walkNiControllerManager);
        add("NiMultiTargetTransformController", walkNiMultiTargetTransformController);
        add("NiTextKeyExtraData", walkNiTextKeyExtraData);
        add("NiStringPalette", walkNiStringPalette);
        add("NiDefaultAVObjectPalette", walkNiDefaultAVObjectPalette);
        // Extra data.
        add("BSBound", walkBSBound);
        add("BSFurnitureMarker", walkBSFurnitureMarker);
        add("bhkConvexTransformShape", walkBhkConvexTransformShape);
        add("bhkListShape", walkBhkListShape);
        add("NiZBufferProperty", walkNiZBufferProperty);
        add("NiBillboardNode", walkNiBillboardNode);
        // Havok constraints and shape phantoms.
        add("bhkLimitedHingeConstraint", walkBhkLimitedHingeConstraint);
        add("bhkRagdollConstraint", walkBhkRagdollConstraint);
        add("bhkPrismaticConstraint", walkBhkPrismaticConstraint);
        add("bhkSimpleShapePhantom", walkBhkSimpleShapePhantom);
        add("bhkSimpleShapeBall", walkBhkSimpleShapePhantom);
        add("bhkSimpleShapeCylinder", walkBhkSimpleShapePhantom);
        add("bhkSimpleShapeCapsule", walkBhkSimpleShapePhantom);
        add("NiVertexColorProperty", walkNiVertexColorProperty);
        add("NiAlphaProperty", walkNiAlphaProperty);
        add("NiSpecularProperty", walkNiSpecularProperty);
        add("NiTriShapeData", walkNiGeometryData);
        add("NiTexturingProperty", walkNiTexturingProperty);
        add("NiSourceTexture", walkNiSourceTexture);
        add("NiSkinInstance", walkNiSkinInstance);
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

    const int headerTailStart = c.pos();
    const quint32 numGroups = c.u32();
    if (!c.ok() || numGroups > kMaxGroups)
        return bad(QStringLiteral("implausible group count %1").arg(numGroups));
    mGroupIds.clear();
    for (quint32 i = 0; i < numGroups; ++i)
        mGroupIds.append(c.u32());
    if (!c.ok()) return bad(QStringLiteral("truncated group table"));
    mHeaderTail = raw.mid(headerTailStart, c.pos() - headerTailStart);

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
    mWalkedOffsets.clear();
    mWalkedTypes.clear();
    mWalkedOffsets.reserve(static_cast<int>(numBlocks));
    mWalkedTypes.reserve(static_cast<int>(numBlocks));
    blocks.reserve(static_cast<int>(numBlocks));
    sizes.reserve(static_cast<int>(numBlocks));

    for (quint32 i = 0; i < numBlocks; ++i) {
        const QString type = mBlockTypes.at(mTypeIndex.at(i));
        // A block's bytes start *before* the zero tag, not after it. The tag is
        // four bytes of the file, so leaving it out makes the re-serialized
        // block four bytes short. That is invisible while the region is opaque
        // - it is copied through whole - and becomes a whole-file mismatch the
        // moment the region splits. A 9-block 10.1.0.106 file loses 36 bytes.
        const int blockStart = c.pos();
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
        block.data = raw.mid(blockStart, c.pos() - blockStart);
        block.footer = QByteArray();
        blocks.append(block);
        sizes.append(static_cast<quint32>(block.data.size()));
        mWalkedOffsets.append(blockStart);
        mWalkedTypes.append(type);
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

    // The block-reference count, its references, the group count and the group
    // ids are re-emitted from the bytes that were read, not rebuilt from
    // mGroupIds: the reference table is not modelled, and rebuilding drops four
    // bytes on every file whose tables are empty.
    out.append(mHeaderTail);

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

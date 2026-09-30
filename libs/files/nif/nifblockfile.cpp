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
        // The bounds check is done in 64-bit on purpose. A payload that claims a
        // huge length - which a misparsed field on a file this code does not yet
        // understand will happily do - overflows mPos + n, wraps negative, passes
        // the comparison, and then asks QByteArray::mid for a multi-gigabyte
        // slice. That aborts the process rather than rejecting the block, so a
        // wrong layout anywhere turned into a crash instead of a failed walk.
        if (!mOk || n < 0) { mOk = false; return false; }
        if (static_cast<qint64>(mPos) + n > static_cast<qint64>(mData.size())) {
            mOk = false;
            return false;
        }
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

// The 20.x form of NiTransformData, where a rotation type of 4 means the block
// stores three separate KeyGroups - one per axis - rather than a quaternion
// channel.
//
// The groups are self-describing: each carries its own key count and its own
// interpolation, and the key width follows from the interpolation, because a
// linear key has only a time and a value while a quadratic one also carries
// forward and backward tangents. That is why the widths cannot be derived from
// the value type alone.
//
// Checked against a shipped 20.0.0.4 block of 1,084 bytes: a u32 count and a u32
// rotation type, three axis groups of nine quadratic float keys at 16 bytes
// (time, value, forward, backward), a translation group of 38 linear Vector3
// keys at 16 bytes (time, value), and an empty scale group. That is
// 8 + 3*152 + 616 + 4 = 1,084 exactly - which is what fixes both the group layout
// and the key widths.
//
// A KeyGroup with no keys holds only its count: the interpolation and the key
// array are both absent rather than empty, so an empty group costs four bytes and
// not twelve.
// The KeyType values as they appear on disk: 1 is linear, 2 quadratic, 3 cubic,
// and 0 is the unset value. They start at 1, not 0, which is worth stating
// because reading 0 as "linear" silently drops the tangents from every
// non-linear group.
constexpr quint32 kKeyTypeLinear = 1u;
constexpr quint32 kKeyTypeQuadratic = 2u;
constexpr quint32 kKeyTypeTbc = 3u;
constexpr quint32 kKeyTypeXyzRotation = 4u;
constexpr quint32 kKeyTypeConst = 5u;
constexpr quint32 kKeyTypeMax = 5u;
constexpr quint32 kRotationTypeXyz = 4u;

// A key is a time, the value, and - for anything other than linear - a forward
// and a backward tangent. The time is the part that is easy to miss: a linear
// Vector3 key is 16 bytes, not the 12 its value type suggests.
bool skipKeyframeGroup(Cursor& c, quint32 valueBytes)
{
    const quint32 numKeys = c.u32();
    if (!c.ok() || numKeys > 10000000u) return false;
    if (numKeys == 0) return true;              // no interpolation, no keys
    const quint32 interpolation = c.u32();
    if (!c.ok() || interpolation > kKeyTypeMax) return false;
    // A quadratic key carries a forward and a backward tangent, each as wide as
    // the value; a TBC key carries a tension, bias and continuity triple instead,
    // which is twelve bytes whatever the value is.
    quint32 keyBytes = 4u + valueBytes;
    if (interpolation == kKeyTypeQuadratic) keyBytes += valueBytes * 2u;
    else if (interpolation == kKeyTypeTbc) keyBytes += 12u;
    c.raw(static_cast<int>(static_cast<quint64>(numKeys) * keyBytes));
    return c.ok();
}

// The single-channel data blocks are KeyGroups, and unlike every other structure
// here they carry no version gate at all - so the older reader that used to sit
// for them, a one-byte interpolation and a fixed twelve-byte value, was wrong in
// every version rather than only in the new ones. It is seven bytes short on a
// 24-byte NiFloatData, because a quadratic float key is sixteen bytes and carries
// two tangents, and the interpolation is a u32.
//
// The four differ only in the width of the value the keys interpolate.
bool walkFloatData(Cursor& c, quint32, quint32) { return skipKeyframeGroup(c, 4); }
bool walkBoolData(Cursor& c, quint32, quint32) { return skipKeyframeGroup(c, 1); }
bool walkPosData(Cursor& c, quint32, quint32) { return skipKeyframeGroup(c, 12); }
bool walkColorData(Cursor& c, quint32, quint32) { return skipKeyframeGroup(c, 16); }

// numRotationKeys has already been read. hasRotationType says whether the caller
// has already consumed the rotation type; it has not when the count is zero,
// because a block with no rotation keys has no type in the file at all and the
// translation group starts four bytes earlier.
//
// Verified against two shipped 20.0.0.4 blocks: 1,084 bytes with a rotation key,
// three axis groups of nine quadratic float keys, 38 linear translation keys and
// no scale keys; and 1,936 bytes with no rotation keys, 120 linear translation
// keys and no scale keys.
bool walkNiKeyframeData(Cursor& c, quint32 version, quint32)
{
    const quint32 numRotationKeys = c.u32();
    if (!c.ok() || numRotationKeys > 10'000'000u) return false;
    if (numRotationKeys == 0) {
        // No rotation keys, so no rotation type in the file, and nothing to
        // inspect before the translation group.
        if (!skipKeyframeGroup(c, 12)) return false;   // translations
        if (!skipKeyframeGroup(c, 4)) return false;    // scales
        return c.ok();
    }
    // The rotation type is a u32 in every version - reading one byte of it
    // leaves the quaternion form three bytes short. A type of 4 means three
    // per-axis float groups; anything else is a run of quaternion keys.
    const quint32 rotationType = c.u32();               // KeyType
    if (!c.ok()) return false;
    if (rotationType != kRotationTypeXyz) {
        // A time, the quaternion, and - for the cubic channel alone - a TBC
        // tangent triple. The time is absent only for the narrow 10.1.0.0 range.
        int keyBytes = 16;                              // quaternion
        if (version <= 167837696u || version >= 167837802u) keyBytes += 4;  // time
        if (rotationType == 3u) keyBytes += 12;         // tbc
        c.raw(static_cast<int>(static_cast<quint64>(numRotationKeys) * keyBytes));
        if (!c.ok()) return false;
    } else {
        if (version <= 167837696u) c.f32();             // order
        for (int axis = 0; axis < 3; ++axis) {
            if (!skipKeyframeGroup(c, 4)) return false; // xyz rotations
        }
    }
    if (!skipKeyframeGroup(c, 12)) return false;   // translations
    if (!skipKeyframeGroup(c, 4)) return false;    // scales
    return c.ok();
}

// NiFloatData / NiBoolData / NiPosData / NiColorData all carry a single
// keyframe channel and nothing else.
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

// A keyframe controller is the time-controller prefix and *one* of two refs: the
// interpolator from 10.1.0.2 on, or the data it used to point at before that.
// They are mutually exclusive, and reading both puts every one of these blocks
// four bytes out - which is what a 30-byte NiTransformController in a 10.2.0.0
// file showed, against the 38 this used to read.
constexpr quint32 kInterpolatorRefVersion = 167837800u;   // 10.1.0.2
constexpr quint32 kControllerDataEnd = 167837799u;         // 10.1.0.0

bool walkKeyframeController(Cursor& c, quint32 version, quint32 bsVersion)
{
    Q_UNUSED(bsVersion)
    if (!walkNiTimeController(c)) return false;
    // NiInterpController carries a manager-controlled flag for a few versions
    // between the data ref and the interpolator ref.
    if (version >= 167837800u && version <= 167837804u) c.u8();   // manager_controlled
    if (version >= kInterpolatorRefVersion) {
        c.u32();  // interpolator
    } else if (version <= kControllerDataEnd) {
        c.u32();  // data
    }
    return c.ok();
}

bool walkNiTransformController(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkKeyframeController(c, version, bsVersion)) return false;
    // A one-version field: only 20.1.0.15 carries it.
    if (version == 335676695u) c.u32();  // unknown_q_q_speed_integer
    return c.ok();
}

bool walkNiTransformInterpolator(Cursor& c, quint32 version, quint32)
{
    c.raw(32);  // translation + rotation + scale
    // The quaternion transform carries three validity bytes in the oldest shape.
    if (version <= 167837805u) c.raw(3);   // trs_valid
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

// The triangle list and per-vertex match groups that NiTriShapeData adds on top
// of the plain geometry data. Skinned meshes carry both, and a mesh with a few
// thousand vertices is tens of kilobytes short without them.
constexpr quint32 kTriShapeHasTrianglesVersion = 167837696u;   // 10.1.0.0
constexpr quint32 kTriShapeMatchGroupsVersion = 50397184u;      // 3.0.0.8

// A MatchGroup is a vertex count and that many vertex indices, both u16, and the
// group count that precedes them is a u16 as well.
//
// The reference definition says Uint, and reading four bytes there is what made
// the array look one group short on every file: the count lands correctly either
// way, but four bytes consumes the first two bytes of the array as well, so the
// parse starts two bytes late and comes up one group long. Two files settle it -
// 2,351 groups over 5,836 bytes and 1,967 over 8,512 both land exactly on their
// block boundary with a u16 count and no adjustment.
bool skipMatchGroups(Cursor& c, quint32 numGroups)
{
    for (quint32 g = 0; g < numGroups; ++g) {
        const quint32 count = c.u16();
        if (!c.ok() || count > 10000u) return false;
        c.raw(static_cast<int>(count * 2u));      // vertex indices
        if (!c.ok()) return false;
    }
    return c.ok();
}

// The triangle header is seven bytes, and its first two fields are u16s: a
// shipped skinned mesh stores 3,922 triangles as `52 0f` and 11,766 triangle
// points as `f6 2d`, followed by two bytes and a has-triangles flag, with the
// triangle data starting on the eighth. Reading the counts as u32 puts the block
// tens of kilobytes out - 29,369 on a mesh with 2,351 vertices.
bool walkNiTriShapeData(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiGeometryData(c, version, bsVersion)) return false;
    const quint32 numTriangles = c.u16();
    c.u16();                                     // num_triangle_points
    c.raw(2);                                    // two fields not yet named
    if (!c.ok() || numTriangles > 10000000u) return false;
    if (version >= kTriShapeHasTrianglesVersion) {
        if (c.u8()) {                            // has_triangles
            c.raw(static_cast<int>(numTriangles * 6u));
        }
    } else {
        c.raw(static_cast<int>(numTriangles * 6u));
    }
    if (!c.ok()) return false;
    if (version < kTriShapeMatchGroupsVersion) return c.ok();
    const quint32 numMatchGroups = c.u16();
    if (!c.ok() || numMatchGroups > 10000000u) return false;
    return skipMatchGroups(c, numMatchGroups);
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

// A blend collision object is the collision object plus the heir and velocity
// gains, and on the older stream versions two more floats.
bool walkBhkBlendCollisionObject(Cursor& c, quint32, quint32 bsVersion)
{
    if (!walkBhkCollisionObject(c, 0, 0)) return false;
    c.f32();    // heir_gain
    c.f32();    // vel_gain
    if (bsVersion < 9u) {
        c.f32();  // unknown_float_1
        c.f32();  // unknown_float_2
    }
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

// NiStringPalette: a byte length, the blob, and the same length again.
//
// The blob is several NUL-terminated strings run together, so it is not a list of
// separate strings and must not be split on the NULs - doing that would need a
// per-string length that is not there. The trailing repeat of the length is part
// of the 96 bytes of a shipped 20.0.0.4 palette: 4 + 88 + 4.
bool walkNiStringPalette(Cursor& c, quint32, quint32)
{
    const quint32 length = c.u32();
    if (!c.ok() || length > 1024u * 1024u) return false;
    c.raw(static_cast<int>(length));
    c.u32();     // the length again
    return c.ok();
}

// NiDefaultAVObjectPalette: a scene ref, an object count, and that many entries.
// Each entry is a length-prefixed name *followed by* its ref - the ref trails the
// name rather than leading it, which is the opposite of every other struct here
// and is what makes the block 95 bytes rather than 91.
//
// Checked: 8 bytes of header, then 26, 35 and 26 for three names of 18, 27 and
// 18 characters. The names are the ones the controller sequences and the
// transform controllers are indexed by.
bool walkNiDefaultAVObjectPalette(Cursor& c, quint32 version, quint32)
{
    if (!skipRefs(c, 1)) return false;    // scene
    const quint32 numObjs = c.u32();
    if (!c.ok() || numObjs > 100000u) return false;
    for (quint32 i = 0; i < numObjs; ++i) {
        if (!skipString(c, version)) return false;
        if (!skipRefs(c, 1)) return false;    // the entry's own ref
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
    c.u16();                                              // flags
    if (version >= kZBufferFunctionFirstVersion
        && version <= kZBufferFunctionLastVersion)
        c.u32();                                          // function
    return c.ok();
}

// NiBillboardNode: a NiNode that carries a billboard mode from 10.1.0.0.
bool walkNiBillboardNode(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiNode(c, version, bsVersion)) return false;
    // The mode is a two-byte enum, not a u32. Reading four bytes here overruns
    // the block by two and shifts every block after it, which surfaces as a
    // nonsense name length in the next block rather than as a billboard error.
    if (version >= 167837696u) c.u16();   // billboard_mode
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

// The three constraint CInfos share a shape: a run of Vector4s, then three
// floats, then an optional motor. The floats are read unconditionally in the
// reference implementation - the version gates in its generated attribute list
// describe a filtered view, not the stream - so they belong to every version,
// and omitting them on the older branch leaves the block twelve bytes short.
// That is what a 156-byte prismatic constraint in a 20.0.0.4 file showed, against
// the 144 this used to read.
bool walkBhkLimitedHingeConstraint(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!skipBhkConstraintCInfo(c)) return false;
    if (bsVersion <= kConstraintOldBsVersion) {
        c.raw(16 * 7);        // pivots, axes and perpendicular axes
    } else {
        c.raw(16 * 8);        // the same vectors, reordered
    }
    c.raw(12);                // min_angle, max_angle, max_friction
    if (version >= kConstraintMotorVersion && bsVersion > kConstraintOldBsVersion) return false;
    return c.ok();
}

bool walkBhkRagdollConstraint(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!skipBhkConstraintCInfo(c)) return false;
    if (bsVersion <= kConstraintOldBsVersion) {
        c.raw(16 * 6);        // pivot, plane and twist per entity
    } else {
        c.raw(16 * 8);        // twist, plane, motor and pivot per entity
    }
    // All six angle and friction floats, on both layouts. The reference reads
    // them unconditionally; the older branch was missing the three plane and
    // cone angles, which left the block twelve bytes short.
    c.raw(24);
    if (version >= kConstraintMotorVersion && bsVersion > kConstraintOldBsVersion) return false;
    return c.ok();
}

bool walkBhkPrismaticConstraint(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!skipBhkConstraintCInfo(c)) return false;
    c.raw(16 * 8);        // pivot, rotation, plane and sliding per entity
    c.raw(12);            // min_distance, max_distance, friction
    if (version >= kConstraintMotorVersion && bsVersion > kConstraintOldBsVersion) return false;
    return c.ok();
}

// The shape phantoms differ only in the type name: the world-object header, eight
// unused bytes and a 4x4 transform. They need no references of their own, so the
// matrix is the whole payload.
//
// The world-object header is twenty-eight bytes: a shape ref, an unknown int, the
// Havok filter and the world-object info. The reference gates the last three behind
// an older version, but they are present in 20.0.0.4 files - a 100-byte phantom
// against the 72 this used to read is what showed it.
bool walkBhkSimpleShapePhantom(Cursor& c, quint32, quint32)
{
    c.u32();    // shape
    c.u32();    // unknown_int
    c.u32();    // havok_filter
    c.u32();    // broad_phase_type
    c.raw(12);  // property: data, size, capacity_and_flags
    c.raw(8);   // unused_01
    c.raw(64);  // transform (Matrix44)
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

// NiSpecularProperty is just a flags word after the NiObjectNET prefix - there is
// no colour and strength beside it, contrary to what the older NIF layout says.
// A shipped 20.0.0.4 block is 14 bytes, which is twelve of prefix and two of
// flags; adding the four bytes of colour puts everything after it four out.
bool walkNiSpecularProperty(Cursor& c, quint32 version, quint32)
{
    if (!walkNiObjectNET(c, version)) return false;
    c.u16();      // flags
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
    // The bump map adds a luma scale, a luma offset and a 2x2 matrix, and from
    // 20.1.0.0 the parallax slot adds its offset.
    for (quint32 i = 0; i < count; ++i) {
        if (!c.u8()) continue;                 // flag clear: nothing to read
        if (!skipTexDesc(c, version)) return false;
        if (i == 5) {
            c.f32();                           // bump_map_luma_scale
            c.f32();                           // bump_map_luma_offset
            c.raw(16);                         // bump_map_matrix
        } else if (version >= 335675397u && i == 7) {
            c.f32();                           // parallax_offset
        }
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

// A ControlledBlock, as found inside a NiControllerSequence.
//
// 33 bytes, and the width is odd, so this does not divide into the four-byte refs
// the field list implies - the tail is a mix of unaligned shorts and i32s. Read
// out of a shipped 20.0.0.4 sequence whose three controlled blocks start at
// +16, +49 and +82 from the block start; the node_name_offset values in it (0, 41
// and 60) are what pin the middle of the layout down.
constexpr int kControlledBlockBytes = 33;

// A controlled block names the node and controller it drives. The five string
// slots are inline sized strings up to 20.0.0.5, palette offsets from 10.1.0.5
// through 20.1.0.3, and fixed strings after that; the node name joins them from
// 10.1.0.0 through 10.1.0.2. The blend interpolator and its index only exist for
// 10.1.0.0 through 10.1.0.2.
bool skipControlledBlock(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (version <= 167837799u) {
        if (!skipString(c, version)) return false;              // target_name
    }
    if (version >= 167837802u) {
        if (!skipRefs(c, 1)) return false;                      // interpolator
    }
    if (version <= 335872000u) {
        if (!skipRefs(c, 1)) return false;                      // controller
    }
    if (version >= 167837800u && version <= 167837806u) {
        if (!skipRefs(c, 1)) return false;                      // blend_interpolator
        c.u16();                                                // blend_index
    }
    if (version >= 167837802u && bsVersion > 0u) c.u8();        // priority
    if (version >= 167837800u && version <= 167837809u) {
        for (int i = 0; i < 5; ++i) {
            if (!skipString(c, version)) return false;          // the five strings
        }
    }
    if (version >= 167903232u && version <= 335609856u) {
        if (!skipRefs(c, 1)) return false;                      // string_palette
        c.raw(20);                                              // five string offsets
    }
    if (version >= 335609857u) {
        for (int i = 0; i < 5; ++i) {
            if (!skipString(c, version)) return false;          // the five strings
        }
    }
    return c.ok();
}

// NiControllerSequence: a name, a controlled-block count, the blocks themselves,
// then the playback fields. The first of those is unversioned in the field list
// but only exists from 10.1.0.2, and a phase float only between 10.1.0.2 and
// 10.3.0.1.
//
// Verified against a 20.0.0.4 sequence of 169 bytes: an eight-byte name, a u32
// count, a u32 grow-by, three 33-byte blocks, 28 bytes of weight/text-keys/
// cycle/frequency/start/stop/manager, an 18-byte name and a palette ref.
constexpr quint32 kSequencePlaybackVersion = 167837802u;   // 10.1.0.2
constexpr quint32 kSequencePhaseEnd = 168034305u;          // 10.3.0.1
constexpr quint32 kSequencePaletteFirst = 167837809u;
constexpr quint32 kSequencePaletteLast = 335609856u;       // 20.1.0.3

bool walkNiControllerSequence(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!skipString(c, version)) return false;                  // name
    const quint32 numControlled = c.u32();
    if (!c.ok() || numControlled > 10000u) return false;
    c.u32();                                                   // array_grow_by
    for (quint32 i = 0; i < numControlled; ++i) {
        if (!skipControlledBlock(c, version, bsVersion)) return false;
    }
    if (version < kSequencePlaybackVersion) return c.ok();
    c.f32();                                                    // weight
    if (!skipRefs(c, 1)) return false;                          // text_keys
    c.u32();                                                    // cycle_type
    c.f32();                                                    // frequency
    if (version <= kSequencePhaseEnd) c.f32();                  // phase
    c.f32();                                                    // start_time
    c.f32();                                                    // stop_time
    if (version == 167837802u) c.u8();                          // play_backwards
    if (!skipRefs(c, 1)) return false;                          // manager
    if (!skipString(c, version)) return false;                  // accum_root_name
    if (version >= 335740936u) c.u32();                         // accum_flags
    if (version >= kSequencePaletteFirst && version <= kSequencePaletteLast) {
        if (!skipRefs(c, 1)) return false;                      // string_palette
    }
    if (version >= 335675399u && bsVersion >= 24u && bsVersion <= 28u) {
        if (!skipRefs(c, 1)) return false;                      // anim_notes
    }
    if (version >= 335675399u && bsVersion > 28u) {
        const quint32 numAnimNoteArrays = c.u16();
        if (!c.ok() || numAnimNoteArrays > 10000u) return false;
        if (!skipRefs(c, numAnimNoteArrays)) return false;      // anim_note_arrays
    }
    return c.ok();
}

// The key-based interpolators. Their base is NiInterpolator, which is NiObject
// with nothing inline at all, so the whole block is the value and a ref to the
// data that drives it - eight bytes for a float, sixteen for a point, five for a
// bool. Reading a name or a controller here, as though it were NiObjectNET, would
// put every one of them out by eight.
bool walkNiFloatInterpolator(Cursor& c, quint32, quint32)
{
    c.f32();                    // value
    if (!skipRefs(c, 1)) return false;   // data
    return c.ok();
}

bool walkNiPoint3Interpolator(Cursor& c, quint32, quint32)
{
    c.vec3();                   // value
    if (!skipRefs(c, 1)) return false;   // data
    return c.ok();
}

bool walkNiBoolInterpolator(Cursor& c, quint32, quint32)
{
    c.u8();                     // value
    if (!skipRefs(c, 1)) return false;   // data
    return c.ok();
}

// The single-interpolator controllers: the time-controller prefix, a ref to the
// interpolator, and whatever the specific controller adds.
//
// 30 bytes for an NiAlphaController (26 of time controller and a ref) and 32 for
// an NiMaterialColorController, whose target colour is a two-byte enum and not
// the Color4 the older layout implies. The extra data ref that used to sit beside
// them is only in the file up to 10.1.0.0, which 20.0.0.4 is past.
constexpr quint32 kInterpControllerDataEnd = 167837799u;   // 10.1.0.0

bool walkSingleInterpController(Cursor& c, quint32 version, quint32)
{
    if (!walkNiTimeController(c)) return false;
    // NiInterpController's manager-controlled flag, for a few 10.1.0.0 versions.
    if (version >= 167837800u && version <= 167837804u) c.u8();
    // Either the interpolator or the data it replaced, never both.
    if (version >= kInterpolatorRefVersion) {
        if (!skipRefs(c, 1)) return false;   // interpolator
    } else if (version <= kInterpControllerDataEnd) {
        if (!skipRefs(c, 1)) return false;   // data
    }
    return c.ok();
}

bool walkNiAlphaController(Cursor& c, quint32 version, quint32 bsVersion)
{
    return walkSingleInterpController(c, version, bsVersion);
}

bool walkNiMaterialColorController(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkSingleInterpController(c, version, bsVersion)) return false;
    c.u16();      // target colour enum
    return c.ok();
}

// NiGeomMorpherController: the interp-controller prefix, then the morph data and
// its interpolators. 89 bytes in a shipped 20.0.0.4 file with six interpolators,
// which fixes the morpher flags at two bytes - not the four an enum would
// suggest.
constexpr quint32 kGeomMorpherFlagsVersion = 167772418u;   // 10.0.1.2
constexpr quint32 kGeomMorpherInterpsVersion = 167837802u;  // 10.1.0.2
constexpr quint32 kGeomMorpherUnknownEnd = 335544325u;    // 20.0.0.5
constexpr quint32 kGeomMorpherUnknownFirst = 167903232u;  // 10.2.0.0

bool walkNiGeomMorpherController(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiTimeController(c)) return false;
    // NiInterpController's manager-controlled flag, for a few 10.1.0.0 versions.
    if (version >= 167837800u && version <= 167837804u) c.u8();
    if (version >= kGeomMorpherFlagsVersion) c.u16();     // morpher_flags
    if (!skipRefs(c, 1)) return false;                    // data
    if (version >= 67108866u) c.u8();                     // always_update
    quint32 numInterpolators = 0;
    if (version >= kGeomMorpherInterpsVersion) {
        numInterpolators = c.u32();
        if (!c.ok() || numInterpolators > 10000u) return false;
        if (version <= kGeomMorpherUnknownEnd) {
            if (!skipRefs(c, numInterpolators)) return false;
        }
    }
    if (version >= kGeomMorpherUnknownFirst && version <= kGeomMorpherUnknownEnd
        && bsVersion > 9) {
        const quint32 numUnknown = c.u32();
        if (!c.ok() || numUnknown > 10000u) return false;
        c.raw(static_cast<int>(numUnknown * 4u));
    }
    return c.ok();
}

// NiBlendInterpolator and its four subclasses. From 10.1.0.2 the array size is a
// byte and from 10.1.0.3 a flags byte appears ahead of it; when the
// manager-controlled bit is set the whole priority block is absent, which is why a
// NiBlendTransformInterpolator can be six bytes.
//
// 10.2.0.0, measured: flags 1, array size 2, weight threshold 0.0 - and no
// transform value, because the value field only exists in the older versions.
constexpr quint32 kBlendFlagsVersion = 167837808u;   // 10.1.0.3
constexpr quint32 kBlendArraySizeVersion = 167837806u;

// A blend interpolator changes shape three times. Up to 10.0.1.5 the array size
// is a ushort with a grow-by, the items carry an int priority and the trailing
// scalars are ushorts and ints; 10.0.1.6 and .7 narrow those; 10.1.0.0 on drops
// the grow-by for a flags byte and folds the item array behind a
// single-interpolator flag. The derived type appends the blended value, but only
// the quaternion transform gates that on the version, so it disappears with
// 10.1.0.0 along with the array shape.
bool walkNiBlendInterpolator(Cursor& c, quint32 version, quint32, int modernValue,
                             int oldValue)
{
    const int valueLen = (version <= 167837805u) ? oldValue : modernValue;
    if (version >= 167837808u) {
        const quint8 flags = c.u8();
        const quint8 arraySize = c.u8();
        c.f32();                                  // weight_threshold
        if ((flags & 1u) == 0) {
            c.u8();    // interp_count
            c.u8();    // single_index
            c.u8();    // high_priority
            c.u8();    // next_high_priority
            c.f32();   // single_time
            c.f32();   // high_weights_sum
            c.f32();   // next_high_weights_sum
            c.f32();   // high_ease_spinner
            c.raw(static_cast<int>(arraySize) * 17);   // interp_array_items
        }
        if (valueLen > 0) c.raw(valueLen);
        return c.ok();
    }
    quint32 arraySize = 0;
    if (version <= 167837805u) {
        arraySize = c.u16();                      // array_size
        c.u16();                                  // array_grow_by
    } else {
        arraySize = c.u8();                       // array_size
    }
    if (!c.ok() || arraySize > 100000u) return false;
    const int itemBytes = (version <= 167837805u) ? 20 : 17;
    c.raw(static_cast<int>(arraySize) * itemBytes);   // interp_array_items
    c.u8();                                       // manager_controlled
    c.f32();                                      // weight_threshold
    c.u8();                                       // only_use_highest_weight
    if (version <= 167837805u) {
        c.u16();  // interp_count
        c.u16();  // single_index
    } else {
        c.u8();   // interp_count
        c.u8();   // single_index
    }
    if (version >= 167837804u) {
        c.u32();  // single_interpolator
        c.f32();  // single_time
    }
    if (version <= 167837805u) {
        c.u32();  // high_priority (int)
        c.u32();  // next_high_priority
    } else {
        c.u8();   // high_priority (sbyte)
        c.u8();   // next_high_priority
    }
    if (valueLen > 0) c.raw(valueLen);
    return c.ok();
}

// The particle-system modifiers. Their base is NiPSysModifier: a name, an order,
// a particle count and a dead flag - nine bytes after the name.
//
// Pinned by four blocks that differ only in name length and their own fields:
// a 37-byte NiPSysPositionModifier with a 24-character name and no fields of its
// own, and a 60-byte NiPSysSpawnModifier with a 21-character name and 26 bytes
// of fields, both leave exactly nine.
bool walkNiPSysModifier(Cursor& c, quint32 version)
{
    if (!skipString(c, version)) return false;    // name
    c.u32();                                      // order
    c.u32();                                      // num_particles
    c.u8();                                       // is_dead
    return c.ok();
}

bool walkNiPSysUpdateCtlr(Cursor& c, quint32, quint32)
{
    return walkNiTimeController(c);
}

bool walkNiPSysPositionModifier(Cursor& c, quint32 version, quint32)
{
    return walkNiPSysModifier(c, version);
}

bool walkNiPSysColorModifier(Cursor& c, quint32 version, quint32)
{
    if (!walkNiPSysModifier(c, version)) return false;
    return skipRefs(c, 1);                        // data
}

bool walkNiPSysBoundUpdateModifier(Cursor& c, quint32 version, quint32)
{
    if (!walkNiPSysModifier(c, version)) return false;
    c.u16();                                      // update_skip
    return c.ok();
}

bool walkNiPSysSpawnModifier(Cursor& c, quint32 version, quint32)
{
    if (!walkNiPSysModifier(c, version)) return false;
    c.u16();                                      // num_spawn_generations
    c.f32();                                       // percentage_spawned
    c.u16();                                       // min_num_to_spawn
    c.u16();                                       // max_num_to_spawn
    c.f32();                                       // spawn_speed_variation
    c.f32();                                       // spawn_dir_variation
    c.f32();                                       // life_span
    c.f32();                                       // life_span_variation
    return c.ok();
}

bool walkNiPSysGrowFadeModifier(Cursor& c, quint32 version, quint32)
{
    if (!walkNiPSysModifier(c, version)) return false;
    c.f32();                                       // grow_time
    c.u16();                                       // grow_generation
    c.f32();                                       // fade_time
    c.u16();                                       // fade_generation
    // The base scale is a 20.2.0.4 field, not part of the older versions.
    if (version >= 335675399u) c.f32();           // base_scale
    return c.ok();
}

bool walkNiPSysRotationModifier(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiPSysModifier(c, version)) return false;
    c.f32();                                       // rotation_speed
    if (version >= 335544322u) c.f32();            // rotation_speed_variation
    // The unknown vector and byte are Starfield-format fields.
    if (bsVersion == 155u) { c.raw(16); c.u8(); }  // unknown_vector, unknown_byte
    if (version >= 335544322u) {
        c.f32();                                   // rotation_angle
        c.f32();                                   // rotation_angle_variation
        c.u8();                                    // random_rot_speed_sign
    }
    c.u8();                                        // random_axis
    c.raw(12);                                     // axis
    return c.ok();
}

bool walkNiPSysAgeDeathModifier(Cursor& c, quint32 version, quint32)
{
    if (!walkNiPSysModifier(c, version)) return false;
    c.u8();                                        // spawn_on_death
    return skipRefs(c, 1);                         // spawn_modifier
}

// NiMorphData: a morph-target count, a vertex count, a relative-targets flag,
// and then one Morph per target.
//
// A Morph in these versions is a frame name and one Vector3 per vertex - the
// legacy weight is not present, because it is gated on a bs_version below ten and
// the shipped 20.0.0.4 file that pins this down has eleven. Confirmed exactly on
// that file: nine bytes of header, six morphs, names totalling 64 characters,
// and 6 * 839 vectors of twelve bytes each makes 60,505 - the block size the
// reference reader reports to the byte.
//
// Below 10.1.0.2 a Morph carries key frames instead of a frame name, and those
// keys have not been measured, so this declines rather than guess at them.
constexpr quint32 kMorphFrameNameVersion = 167837802u;   // 10.1.0.2
constexpr quint32 kMorphLegacyWeightFirst = 167837800u;
constexpr quint32 kMorphLegacyWeightLast = 335609858u;   // 20.1.0.2

bool walkNiMorphData(Cursor& c, quint32 version, quint32 bsVersion)
{
    const quint32 numMorphs = c.u32();
    const quint32 numVertices = c.u32();
    c.u8();                                          // relative_targets
    if (!c.ok() || numMorphs > 100000u || numVertices > 10000000u) return false;
    if (version < kMorphFrameNameVersion) return false;
    for (quint32 i = 0; i < numMorphs; ++i) {
        if (!skipString(c, version)) return false;    // frame_name
        if (version >= kMorphLegacyWeightFirst && version <= kMorphLegacyWeightLast
            && bsVersion < 10) {
            c.f32();                                  // legacy_weight
        }
        c.raw(static_cast<int>(static_cast<quint64>(numVertices) * 12u));
        if (!c.ok()) return false;
    }
    return c.ok();
}

// NiSkinData: the skin transform, a bone count, a weights flag, and then one
// inline BoneData per bone.
//
// A BoneData here is not the same thing as the bone refs inside NiSkinInstance -
// this one is a full transform, a bounding sphere, a vertex count and that many
// vertex weights, each a u16 index and an f32 weight. Checked against a shipped
// 20.0.0.4 block of 17,777 bytes with 26 bones: 57 bytes of header, 26 bodies of
// 70 bytes, and 15,900 bytes of six-byte weights - which divides exactly, where
// an eight-byte weight does not.
constexpr quint32 kSkinDataPartitionFirst = 67108866u;
constexpr quint32 kSkinDataPartitionLast = 167837696u;   // 10.1.0.0
constexpr quint32 kSkinDataWeightVersion = 67240192u;    // 4.0.0.1
constexpr int kBoneVertDataBytes = 6;

bool walkNiSkinData(Cursor& c, quint32 version, quint32)
{
    c.raw(52);                                 // skin_transform (NiTransform)
    const quint32 numBones = c.u32();
    if (!c.ok() || numBones > 10000u) return false;
    if (version >= kSkinDataPartitionFirst && version <= kSkinDataPartitionLast) {
        if (!skipRefs(c, 1)) return false;     // skin_partition
    }
    bool hasVertexWeights = true;
    if (version >= kSkinDataWeightVersion) hasVertexWeights = c.u8() != 0;
    for (quint32 i = 0; i < numBones; ++i) {
        c.raw(52);                             // BoneData.skin_transform
        c.raw(16);                             // BoneData.bounding_sphere
        const quint32 numVerts = c.u16();
        if (!c.ok() || numVerts > 100000u) return false;
        if (version <= kSkinDataWeightVersion || hasVertexWeights) {
            c.raw(static_cast<int>(static_cast<quint64>(numVerts) * kBoneVertDataBytes));
            if (!c.ok()) return false;
        }
    }
    return c.ok();
}

// A length-prefixed array of u16s, which is how every array in these blocks is
// stored: a u32 count and then the elements.
bool skipU16Array(Cursor& c)
{
    const quint32 n = c.u32();
    if (!c.ok() || n > 10000000u) return false;
    c.raw(static_cast<int>(static_cast<quint64>(n) * 2u));
    return c.ok();
}

// One SkinPartition entry. A partition splits a skinned mesh into pieces small
// enough for hardware skinning, and records which vertices and bones belong to
// each.
//
// Checked field by field against the reference definition rather than the older
// NIF documentation, which does not describe the version-gated flags at all: from
// 10.1.0.0 the vertex map and the vertex weights are each preceded by a byte that
// says whether they are present, so an empty partition costs three bytes for
// those flags and nothing else.
//
// Strip-based partitions are a distinct layout - a variable two-dimensional array
// whose row lengths come from a separate list that only the Starfield variant
// stores - and have not been measured, so they decline rather than be guessed at.
constexpr quint32 kSkinPartitionFlagsVersion = 167837696u;   // 10.1.0.0
constexpr quint8 kSkinPartitionHalfFloatWeights = 15u;

bool walkSkinPartitionEntry(Cursor& c, quint32 version)
{
    const quint32 numVertices = c.u16();
    const quint32 numTriangles = c.u16();
    const quint32 numBones = c.u16();
    const quint32 numStrips = c.u16();
    const quint32 numWeightsPerVertex = c.u16();
    if (!c.ok() || numBones > 10000u || numVertices > 100000u) return false;
    // The arrays carry no length of their own: the counts that precede them are
    // the lengths, and reading a length as well double-counts four bytes and
    // fails on the first block that has one.
    c.raw(static_cast<int>(static_cast<quint64>(numBones) * 2u));      // bones
    if (!c.ok()) return false;
    quint8 hasWeights = 0;
    if (version >= kSkinPartitionFlagsVersion) {
        if (c.u8()) {                          // has_vertex_map
            c.raw(static_cast<int>(static_cast<quint64>(numVertices) * 2u));
            if (!c.ok()) return false;
        }
        hasWeights = c.u8();
        if (hasWeights > 0 && hasWeights != kSkinPartitionHalfFloatWeights) {
            const quint64 n = static_cast<quint64>(numVertices) * numWeightsPerVertex;
            if (n > 100000000u) return false;
            c.raw(static_cast<int>(n * 4u));   // float weights
            if (!c.ok()) return false;
        }
    }
    if (numStrips > 0) {
        // The strip form has no has-faces byte and no has-bone-indices byte, and
        // the bone indices follow the strips unconditionally after two bytes.
        // Reading the has-faces byte here - where the field list puts it, ahead of
        // the strips - shifts the strip length by one byte and it comes out as 263
        // instead of 1,999. A shipped 67,466-byte strip partition is exact with
        // this shape and only this shape: 21,164 + 29,788 + 16,510.
        for (quint32 s = 0; s < numStrips; ++s) {
            const quint32 len = c.u16();
            if (!c.ok() || len > 100000u) return false;
            c.raw(static_cast<int>(static_cast<quint64>(len) * 2u));
            if (!c.ok()) return false;
        }
        c.raw(2);
        const quint64 n = static_cast<quint64>(numVertices) * numWeightsPerVertex;
        if (n > 100000000u) return false;
        c.raw(static_cast<int>(n));
        return c.ok();
    }
    const quint8 hasFaces = c.u8();
    if (!c.ok()) return false;
    if (!hasFaces) return c.ok();
    if (numTriangles > 10000000u) return false;
    c.raw(static_cast<int>(static_cast<quint64>(numTriangles) * 6u));  // Triangle
    if (!c.ok()) return false;
    if (c.u8()) {                              // has_bone_indices
        const quint64 n = static_cast<quint64>(numVertices) * numWeightsPerVertex;
        if (n > 100000000u) return false;
        c.raw(static_cast<int>(n));
        if (!c.ok()) return false;
    }
    return c.ok();
}

bool walkNiSkinPartition(Cursor& c, quint32 version, quint32)
{
    const quint32 numPartitions = c.u32();
    if (!c.ok() || numPartitions > 100000u) return false;
    for (quint32 i = 0; i < numPartitions; ++i) {
        if (!walkSkinPartitionEntry(c, version)) return false;
    }
    return c.ok();
}

// The particle emitter controllers. Their base is NiPSysModifierCtlr: the
// single-interpolator controller prefix plus the name of the modifier they drive.
//
// Fixed by three measured blocks: a 48-byte base, a 52-byte float controller that
// adds a data ref, and a 56-byte NiPSysEmitterCtlr that adds a data ref and a
// visibility interpolator. 48 = 26 of time controller, a ref, and an 18-byte name.
constexpr quint32 kPSysModifierCtlrDataEnd = 167837799u;   // 10.1.0.0

bool walkNiPSysModifierCtlr(Cursor& c, quint32 version, quint32)
{
    if (!walkNiTimeController(c)) return false;
    if (!skipRefs(c, 1)) return false;          // interpolator
    if (version <= kPSysModifierCtlrDataEnd) {
        if (!skipRefs(c, 1)) return false;      // data, older versions only
    }
    if (!skipString(c, version)) return false;  // modifier_name
    return c.ok();
}

// The float modifier controllers add a data ref, but only through 10.1.0.0.
// From 10.1.0.2 the emitter controller carries a visibility interpolator
// instead. They are mutually exclusive, exactly like the keyframe controller's
// interpolator-or-data pair, and reading both puts every one of these blocks
// four bytes out.
bool walkNiPSysModifierFloatCtlr(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkNiPSysModifierCtlr(c, version, bs)) return false;
    if (version <= kPSysModifierCtlrDataEnd) {
        if (!skipRefs(c, 1)) return false;      // data
    }
    return c.ok();
}

bool walkNiPSysEmitterCtlr(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkNiPSysModifierFloatCtlr(c, version, bs)) return false;
    if (version >= kInterpolatorRefVersion) {
        if (!skipRefs(c, 1)) return false;      // visibility_interpolator
    }
    return c.ok();
}

// A dynamic effect is the AV-object prefix plus a switch state and the list of
// nodes it is switched on for. The affected-node list appears in two disjoint
// version windows - it was present early, dropped, and came back - so this is a
// range test rather than a single cut.
constexpr quint32 kSwitchStateVersion = 167837802u;   // 10.1.0.4
constexpr quint32 kAffectedNodesMin = 167837696u;      // 10.1.0.1
constexpr quint32 kAffectedNodesMax = 67108866u;       // 4.0.1.0

bool walkNiDynamicEffect(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiAVObject(c, version, bsVersion)) return false;
    if (version >= kSwitchStateVersion && bsVersion < 130) c.u8();  // switch_state
    const bool hasAffected = (version <= kAffectedNodesMax)
                          || (version >= kAffectedNodesMin && bsVersion < 130);
    if (hasAffected) {
        const quint32 n = c.u32();                     // num_affected_nodes
        if (!c.ok() || n > 100000u) return false;
        if (!skipRefs(c, n)) return false;             // affected_nodes
    }
    return c.ok();
}

// The light base: dimmer and three colours. Everything under it adds only its
// own optical parameters, so the four families share this prefix.
bool walkNiLight(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiDynamicEffect(c, version, bsVersion)) return false;
    c.f32();    // dimmer
    c.raw(12);  // ambient_color
    c.raw(12);  // diffuse_color
    c.raw(12);  // specular_color
    return c.ok();
}

bool walkNiAmbientLight(Cursor& c, quint32 version, quint32 bsVersion)
{
    return walkNiLight(c, version, bsVersion);
}

// A directional light is the light base and nothing more. The direction, shadow
// centre and shadow plane that the format is often described as carrying are not
// in the reference definition, and reading them overruns the block by twenty-
// eight bytes - which surfaced as a failure in whatever block followed.
bool walkNiDirectionalLight(Cursor& c, quint32 version, quint32 bsVersion)
{
    return walkNiLight(c, version, bsVersion);
}

bool walkNiPointLight(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiLight(c, version, bsVersion)) return false;
    c.f32();    // constant_attenuation
    c.f32();    // linear_attenuation
    c.f32();    // quadratic_attenuation
    return c.ok();
}

bool walkNiSpotLight(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiPointLight(c, version, bsVersion)) return false;
    c.f32();    // outer_spot_angle
    // Inner angle and exponent were added together in 20.2.0.4.
    if (version >= 335675397u) {
        c.f32();  // inner_spot_angle
        c.f32();  // exponent
    }
    return c.ok();
}

// A bone LOD controller. Its __init__ reads the counts here and the arrays in
// hand, so the prefix is the three counts followed by the node groups; each group
// is a bone count and that many node refs.
//
// The two shape-group lists are a 4.0.2.0 addition and only exist when the stream
// version is 0, which no Oblivion or Skyrim mesh uses, so they are not walked.
// A SkinInfoSet is a name, a data ref and a bone-index list per entry, and
// guessing its width is worse than refusing the block outright.
bool walkNiBSBoneLODController(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiTimeController(c)) return false;
    if (version >= 67240448u && bsVersion == 0) return false;   // shape groups
    c.u32();  // lod
    const quint32 numLods = c.u32();
    if (!c.ok() || numLods > 1000u) return false;
    const quint32 numGroups = c.u32();
    if (!c.ok() || numGroups > 100000u) return false;
    for (quint32 g = 0; g < numGroups; ++g) {
        const quint32 numBones = c.u32();
        if (!c.ok() || numBones > 100000u) return false;
        if (!skipRefs(c, numBones)) return false;
    }
    return c.ok();
}

// A visibility controller is a single-interpolator controller and nothing more:
// the boolean interpolator controller adds no field of its own, so the byte that
// was being read here put the block one byte long and shifted everything after.
bool walkNiVisController(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkSingleInterpController(c, version, bsVersion)) return false;
    if (version <= kControllerDataEnd) {
        if (!skipRefs(c, 1)) return false;  // data
    }
    return c.ok();
}

// The particle system is the tri-based geometry prefix plus a world-space flag
// and the modifier list. The bs >= 83 and bs >= 100 fields in the reference are
// Skyrim-and-later additions; no Oblivion mesh carries them.
bool walkNiParticleSystem(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiTriBasedGeom(c, version, bsVersion)) return false;
    if (version >= 167837696u) {
        c.u8();                                   // world_space
        const quint32 n = c.u32();                // num_modifiers
        if (!c.ok() || n > 100000u) return false;
        if (!skipRefs(c, n)) return false;
    }
    return c.ok();
}

// A property is the object-net prefix and nothing else; each subclass then adds
// its own small payload.
bool walkNiProperty(Cursor& c, quint32 version, quint32)
{
    return walkNiObjectNET(c, version);
}
bool walkNiFogProperty(Cursor& c, quint32 version, quint32)
{
    if (!walkNiProperty(c, version, 0)) return false;
    c.u16();    // flags
    c.f32();    // fog_depth
    c.raw(12);  // fog_color
    return c.ok();
}

bool walkNiWireframeProperty(Cursor& c, quint32 version, quint32)
{
    if (!walkNiProperty(c, version, 0)) return false;
    c.u16();    // flags
    return c.ok();
}

// A blend controller is the time-controller prefix and a key count.
bool walkBhkBlendController(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiTimeController(c)) return false;
    c.u32();    // keys
    return c.ok();
}

// A transform shape wraps another shape in a 4x4 matrix.
bool walkBhkTransformShape(Cursor& c, quint32 version, quint32)
{
    if (!skipHavokMaterial(c, version)) return false;
    if (!skipRefs(c, 1)) return false;   // shape
    if (!skipHavokMaterial(c, version)) return false;  // material
    c.f32();                             // radius
    c.raw(64);                           // transform
    return c.ok();
}

// A path interpolator steers along a curve: flags, banking parameters and the
// two data refs that hold the path and the percent channel.
bool walkNiPathInterpolator(Cursor& c, quint32 version, quint32)
{
    c.u16();    // flags
    c.u32();    // bank_dir
    c.f32();    // max_bank_angle
    c.f32();    // smoothing
    c.u16();    // follow_axis
    if (!skipRefs(c, 1)) return false;   // path_data
    if (!skipRefs(c, 1)) return false;   // percent_data
    return c.ok();
}

// A texture transform controller is a float single-interpolator controller
// plus a shader flag, a texture slot and an operation selector.
bool walkNiTextureTransformController(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkSingleInterpController(c, version, bsVersion)) return false;
    c.u8();    // shader_map
    c.u32();   // texture_slot
    c.u32();   // operation
    return c.ok();
}

// A particle system's data. Its geometry-data prefix carries no additional-data
// ref - the ref the other geometry blocks have is absent here, which is easy to
// miss because the two are otherwise identical. The particle arrays that follow
// are sized by the vertex count, not by the active count, so a system with no
// active particles still carries a full set of per-vertex entries.
bool walkNiPSysData(Cursor& c, quint32 version, quint32 bsVersion)
{
    Q_UNUSED(bsVersion);
    if (version >= 167837810u) c.u32();                  // group_id
    const quint32 numVertices = c.u16();
    if (!c.ok() || numVertices > 1000000u) return false;
    if (version >= 167837696u) { c.u8(); c.u8(); }        // keep/compress flags
    const quint8 hasVertices = c.u8();
    if (!c.ok() || hasVertices > 1) return false;
    if (hasVertices) c.raw(static_cast<int>(numVertices) * 12);
    quint16 dataFlags = 0;
    if (version >= 167772416u) dataFlags = c.u16();
    if (!c.ok()) return false;
    const quint8 hasNormals = c.u8();
    if (!c.ok() || hasNormals > 1) return false;
    if (hasNormals) c.raw(static_cast<int>(numVertices) * 12);
    if (version >= 167837696u && hasNormals && (dataFlags & 4096u) != 0) {
        c.raw(static_cast<int>(numVertices) * 12);
        c.raw(static_cast<int>(numVertices) * 12);
    }
    c.raw(16);                                            // bounding sphere
    const quint8 hasVertexColors = c.u8();
    if (!c.ok() || hasVertexColors > 1) return false;
    if (hasVertexColors) c.raw(static_cast<int>(numVertices) * 16);
    if (hasVertices) c.raw(static_cast<int>(numVertices) * 8 * (dataFlags & 63u));
    if (version >= 167772416u) c.u16();                  // consistency flags
    // The additional geometry data ref appears with 20.0.0.4.
    if (version >= 335544324u) c.u32();                  // additional_data
    // NiParticlesData.
    const quint8 hasRadii = c.u8();
    if (!c.ok() || hasRadii > 1) return false;
    if (hasRadii) c.raw(static_cast<int>(numVertices) * 4);
    const quint32 numActive = c.u16();
    if (!c.ok() || numActive > 1000000u) return false;
    const quint8 hasSizes = c.u8();
    if (!c.ok() || hasSizes > 1) return false;
    if (hasSizes) c.raw(static_cast<int>(numVertices) * 4);
    const quint8 hasRotations = c.u8();
    if (!c.ok() || hasRotations > 1) return false;
    if (hasRotations) c.raw(static_cast<int>(numVertices) * 16);
    // The rotation angles and axes also appear with 20.0.0.4.
    if (version >= 335544324u) {
        const quint8 hasRotationAngles = c.u8();
        if (!c.ok() || hasRotationAngles > 1) return false;
        if (hasRotationAngles) c.raw(static_cast<int>(numVertices) * 4);
        const quint8 hasRotationAxes = c.u8();
        if (!c.ok() || hasRotationAxes > 1) return false;
        if (hasRotationAxes) c.raw(static_cast<int>(numVertices) * 12);
    }
    // NiPSysData: one particle info per vertex. The rotation axis is a
    // 10.0.1.1-and-older field, so the record is 40 bytes there and 28 after.
    const int particleInfoSize = (version <= 168034305u) ? 40 : 28;
    c.raw(static_cast<int>(numVertices) * particleInfoSize);
    if (bsVersion == 155u) c.raw(12);                           // unknown_vector
    if (version == 335676423u) c.u8();                          // unknown byte
    if (version >= 335544322u) {
        const quint8 hasRotationSpeeds = c.u8();
        if (!c.ok() || hasRotationSpeeds > 1) return false;
        if (hasRotationSpeeds) c.raw(static_cast<int>(numVertices) * 4);
    }
    c.u16();                                              // num_added_particles
    c.u16();                                              // added_particles_base
    if (version == 335676423u) c.u8();                    // unknown byte
    return c.ok();
}

// An emitter is a particle modifier - so it carries the modifier's name, order,
// target and active flag - followed by its optical parameters. The colour is a
// Color4; only the radius variation is a 10.1.0.5 field, the life span and its
// variation are always there.
bool walkNiPSysEmitter(Cursor& c, quint32 version, quint32)
{
    if (!walkNiPSysModifier(c, version)) return false;
    c.f32();    // speed
    c.f32();    // speed_variation
    c.f32();    // declination
    c.f32();    // declination_variation
    c.f32();    // planar_angle
    c.f32();    // planar_angle_variation
    c.raw(16);  // initial_color
    c.f32();    // initial_radius
    if (version >= 168034305u) c.f32();  // radius_variation
    c.f32();    // life_span
    c.f32();    // life_span_variation
    if (version == 335676423u) c.raw(8);  // unknown_q_q_speed_floats
    return c.ok();
}

// A volume emitter adds the object it emits from.
bool walkNiPSysVolumeEmitter(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkNiPSysEmitter(c, version, bs)) return false;
    if (version >= 167837696u) c.u32();   // emitter_object
    return c.ok();
}

// The volume emitters are the volume emitter plus their own dimensions.
bool walkNiPSysBoxEmitter(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkNiPSysVolumeEmitter(c, version, bs)) return false;
    c.f32();    // width
    c.f32();    // height
    c.f32();    // depth
    return c.ok();
}

bool walkNiPSysSphereEmitter(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkNiPSysVolumeEmitter(c, version, bs)) return false;
    c.f32();    // radius
    return c.ok();
}

bool walkNiPSysCylinderEmitter(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkNiPSysVolumeEmitter(c, version, bs)) return false;
    c.f32();    // radius
    c.f32();    // height
    return c.ok();
}

// A boolean modifier controller is the modifier controller plus the byte it
// interpolates, and on the older versions a data ref.
bool walkNiPSysModifierActiveCtlr(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkNiPSysModifierCtlr(c, version, bs)) return false;
    c.u8();    // bool_value
    if (version <= kPSysModifierCtlrDataEnd) {
        if (!skipRefs(c, 1)) return false;   // data
    }
    return c.ok();
}

// A reset-on-loop controller is the time-controller prefix and nothing else.
bool walkNiPSysResetOnLoopCtlr(Cursor& c, quint32, quint32)
{
    return walkNiTimeController(c);
}

// A flip controller is a float single-interpolator controller plus a texture
// slot; the accumulation time, delta and source count are older-version fields.
bool walkNiFlipController(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkSingleInterpController(c, version, bs)) return false;
    c.u32();    // texture_slot
    return c.ok();
}

// A boolean timeline interpolator is the boolean interpolator prefix and a byte.
bool walkNiBoolTimelineInterpolator(Cursor& c, quint32, quint32)
{
    c.u8();    // bool_value
    return c.ok();
}

// A stiff spring constraint is the constraint CInfo plus two pivots and a length.
bool walkBhkStiffSpringConstraint(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!skipBhkConstraintCInfo(c)) return false;
    c.raw(16);   // pivot_a
    c.raw(16);   // pivot_b
    c.f32();     // length
    return c.ok();
}

// A malleable constraint wraps one of the other constraint types. It carries the
// base CInfo, a type selector, a second CInfo, the wrapped type's own CInfo, and
// through 20.0.0.5 a tau and damping.
bool walkBhkMalleableConstraint(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!skipBhkConstraintCInfo(c)) return false;   // BhkConstraint base
    const quint32 type = c.u32();                   // type
    if (!skipBhkConstraintCInfo(c)) return false;   // constraint_info
    switch (type) {
    case 0:                                         // ball_and_socket
        c.raw(32);
        break;
    case 1:                                         // hinge
    case 2:                                         // limited_hinge
        if (bsVersion <= kConstraintOldBsVersion) c.raw(16 * 7);
        else c.raw(16 * 8 + 12);
        break;
    case 6:                                         // prismatic
        c.raw(16 * 8 + 12);
        break;
    case 7:                                         // ragdoll
        if (bsVersion <= kConstraintOldBsVersion) c.raw(16 * 6 + 12);
        else c.raw(16 * 8 + 24);
        break;
    case 8:                                         // stiff_spring
        c.raw(36);
        break;
    default:
        return false;
    }
    if (version <= 335544325u) { c.f32(); c.f32(); }  // tau, damping
    return c.ok();
}

// A mesh particle system is the particle system and nothing more.
bool walkNiMeshParticleSystem(Cursor& c, quint32 version, quint32 bsVersion)
{
    return walkNiParticleSystem(c, version, bsVersion);
}

// A mesh emitter is the emitter plus a mesh count, two emission enums and an
// axis.
bool walkNiPSysMeshEmitter(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkNiPSysEmitter(c, version, bs)) return false;
    const quint32 numEmitterMeshes = c.u32();
    if (!c.ok() || numEmitterMeshes > 100000u) return false;
    c.raw(static_cast<int>(numEmitterMeshes) * 4);  // emitter_meshes
    c.u32();    // initial_velocity_type
    c.u32();    // emission_type
    c.raw(12);  // emission_axis
    return c.ok();
}

// A multi-sphere shape is the sphere-rep base, a world-object property, a sphere
// count and that many center-and-radius records.
bool walkBhkMultiSphereShape(Cursor& c, quint32 version, quint32)
{
    if (!skipHavokMaterial(c, version)) return false;
    c.raw(12);                                    // shape_property
    const quint32 numSpheres = c.u32();
    if (!c.ok() || numSpheres > 100000u) return false;
    c.raw(static_cast<int>(numSpheres) * 16);    // centre and radius per sphere
    return c.ok();
}

// A dither property is the property base and a two-byte flags word.
bool walkNiDitherProperty(Cursor& c, quint32 version, quint32)
{
    if (!walkNiProperty(c, version, 0)) return false;
    c.u16();    // flags
    return c.ok();
}

// A gravity modifier is the particle modifier's name plus the gravity parameters.
// The world-aligned flag is a newer-stream addition.
bool walkNiPSysGravityModifier(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiPSysModifier(c, version)) return false;
    c.u32();    // gravity_object
    c.raw(12);  // gravity_axis
    c.f32();    // decay
    c.f32();    // strength
    c.u32();    // force_type
    c.f32();    // turbulence
    c.f32();    // turbulence_scale
    if (bsVersion > kConstraintOldBsVersion) c.u8();  // world_aligned
    return c.ok();
}

// A mesh particle data is the particle data plus the pool size, fill flag,
// generation count and mesh ref, all from 10.1.0.0 on.
bool walkNiMeshPSysData(Cursor& c, quint32 version, quint32 bsVersion)
{
    if (!walkNiPSysData(c, version, bsVersion)) return false;
    if (version >= 167903232u) {
        c.u32();    // default_pool_size
        c.u8();     // fill_pools_on_load
        c.u32();    // num_generations
        c.u32();    // particle_meshes
    }
    return c.ok();
}

// A drag modifier is the particle modifier's name plus the drag parameters.
bool walkNiPSysDragModifier(Cursor& c, quint32 version, quint32)
{
    if (!walkNiPSysModifier(c, version)) return false;
    c.u32();    // drag_object
    c.raw(12);  // drag_axis
    c.f32();    // percentage
    c.f32();    // range
    c.f32();    // range_falloff
    return c.ok();
}

// A collider manager is a particle modifier carrying the head of its collider
// chain.
bool walkNiPSysColliderManager(Cursor& c, quint32 version, quint32)
{
    if (!walkNiPSysModifier(c, version)) return false;
    return skipRefs(c, 1);    // collider
}

// A particle collider is the bounce and spawn flags, the spawn modifier, its
// parent manager, the next collider in the chain and the collider object.
bool walkNiPSysCollider(Cursor& c, quint32, quint32)
{
    c.f32();    // bounce
    c.u8();     // spawn_on_collide
    c.u8();     // die_on_collide
    if (!skipRefs(c, 1)) return false;   // spawn_modifier
    c.u32();    // parent
    if (!skipRefs(c, 1)) return false;   // next_collider
    c.u32();    // collider_object
    return c.ok();
}

bool walkNiPSysPlanarCollider(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkNiPSysCollider(c, version, bs)) return false;
    c.f32();    // width
    c.f32();    // height
    c.raw(12);  // x_axis
    c.raw(12);  // y_axis
    return c.ok();
}

bool walkNiPSysSphericalCollider(Cursor& c, quint32 version, quint32 bs)
{
    if (!walkNiPSysCollider(c, version, bs)) return false;
    c.f32();    // radius
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
        add("bhkBlendCollisionObject", walkBhkBlendCollisionObject);
        add("bhkSPCollisionObject", walkBhkCollisionObject);
        add("bhkPCollisionObject", walkBhkCollisionObject);
        add("bhkNPCollisionObject", walkBhkCollisionObject);
        // Properties and controllers.
        add("NiMaterialProperty", walkNiMaterialProperty);
        add("NiStencilProperty", walkNiStencilProperty);
        add("NiControllerManager", walkNiControllerManager);
        add("NiMultiTargetTransformController", walkNiMultiTargetTransformController);
        add("NiTextKeyExtraData", walkNiTextKeyExtraData);
        add("NiStringPalette", walkNiStringPalette);
        add("NiDefaultAVObjectPalette", walkNiDefaultAVObjectPalette);
        add("NiControllerSequence", walkNiControllerSequence);
        // Key-based interpolators.
        add("NiFloatInterpolator", walkNiFloatInterpolator);
        add("NiPoint3Interpolator", walkNiPoint3Interpolator);
        add("NiBoolInterpolator", walkNiBoolInterpolator);
        // Controllers and blend interpolators.
        add("NiAlphaController", walkNiAlphaController);
        add("NiMaterialColorController", walkNiMaterialColorController);
        add("NiGeomMorpherController", walkNiGeomMorpherController);
        add("NiBlendTransformInterpolator", [](Cursor& c, quint32 v, quint32 b) {
            return walkNiBlendInterpolator(c, v, b, 0, 35);
        });
        add("NiBlendFloatInterpolator", [](Cursor& c, quint32 v, quint32 b) {
            return walkNiBlendInterpolator(c, v, b, 4, 4);
        });
        add("NiBlendPoint3Interpolator", [](Cursor& c, quint32 v, quint32 b) {
            return walkNiBlendInterpolator(c, v, b, 12, 12);
        });
        add("NiBlendBoolInterpolator", [](Cursor& c, quint32 v, quint32 b) {
            return walkNiBlendInterpolator(c, v, b, 1, 1);
        });
        // Particle system.
        add("NiPSysUpdateCtlr", walkNiPSysUpdateCtlr);
        add("NiPSysPositionModifier", walkNiPSysPositionModifier);
        add("NiPSysColorModifier", walkNiPSysColorModifier);
        add("NiPSysBoundUpdateModifier", walkNiPSysBoundUpdateModifier);
        add("NiPSysSpawnModifier", walkNiPSysSpawnModifier);
        add("NiPSysGrowFadeModifier", walkNiPSysGrowFadeModifier);
        add("NiPSysRotationModifier", walkNiPSysRotationModifier);
        add("NiPSysAgeDeathModifier", walkNiPSysAgeDeathModifier);
        add("NiMorphData", walkNiMorphData);
        // Lights.
        add("NiAmbientLight", walkNiAmbientLight);
        add("NiDirectionalLight", walkNiDirectionalLight);
        add("NiPointLight", walkNiPointLight);
        add("NiSpotLight", walkNiSpotLight);
        add("NiVisController", walkNiVisController);
        add("NiBoneLODController", walkNiBSBoneLODController);
        add("NiBSBoneLODController", walkNiBSBoneLODController);
        // Geometry and particles.
        add("NiGeometry", walkNiTriBasedGeom);
        add("NiParticles", walkNiTriBasedGeom);
        add("NiParticleSystem", walkNiParticleSystem);
        add("NiPSysData", walkNiPSysData);
        // Properties.
        add("NiFogProperty", walkNiFogProperty);
        add("NiWireframeProperty", walkNiWireframeProperty);
        // Havok.
        add("bhkBlendController", walkBhkBlendController);
        add("bhkTransformShape", walkBhkTransformShape);
        // Interpolators.
        add("NiPathInterpolator", walkNiPathInterpolator);
        add("NiPSysBoxEmitter", walkNiPSysBoxEmitter);
        add("NiPSysSphereEmitter", walkNiPSysSphereEmitter);
        add("NiPSysCylinderEmitter", walkNiPSysCylinderEmitter);
        add("NiPSysModifierActiveCtlr", walkNiPSysModifierActiveCtlr);
        add("NiPSysResetOnLoopCtlr", walkNiPSysResetOnLoopCtlr);
        add("NiFlipController", walkNiFlipController);
        add("NiBoolTimelineInterpolator", walkNiBoolTimelineInterpolator);
        add("bhkHingeConstraint", walkBhkLimitedHingeConstraint);
        add("bhkStiffSpringConstraint", walkBhkStiffSpringConstraint);
        add("bhkMalleableConstraint", walkBhkMalleableConstraint);
        add("NiMeshParticleSystem", walkNiMeshParticleSystem);
        add("NiPSysMeshEmitter", walkNiPSysMeshEmitter);
        add("bhkMultiSphereShape", walkBhkMultiSphereShape);
        add("NiDitherProperty", walkNiDitherProperty);
        add("NiPSysGravityModifier", walkNiPSysGravityModifier);
        add("NiPSysColliderManager", walkNiPSysColliderManager);
        add("NiPSysPlanarCollider", walkNiPSysPlanarCollider);
        add("NiPSysSphericalCollider", walkNiPSysSphericalCollider);
        add("NiPSysGrowFadeModifier", walkNiPSysGrowFadeModifier);
        add("NiMeshPSysData", walkNiMeshPSysData);
        add("NiPSysDragModifier", walkNiPSysDragModifier);
        // Particle emitter controllers.
        add("NiPSysEmitterCtlr", walkNiPSysEmitterCtlr);
        add("NiPSysModifierFloatCtlr", walkNiPSysModifierFloatCtlr);
        add("NiPSysEmitterDeclinationCtlr", walkNiPSysModifierFloatCtlr);
        add("NiPSysEmitterDeclinationVarCtlr", walkNiPSysModifierFloatCtlr);
        add("NiPSysEmitterInitialRadiusCtlr", walkNiPSysModifierFloatCtlr);
        add("NiPSysEmitterLifeSpanCtlr", walkNiPSysModifierFloatCtlr);
        add("NiPSysEmitterSpeedCtlr", walkNiPSysModifierFloatCtlr);
        add("NiPSysGravityStrengthCtlr", walkNiPSysModifierFloatCtlr);
        add("NiPSysInitialRotAngleCtlr", walkNiPSysModifierFloatCtlr);
        add("NiPSysInitialRotSpeedCtlr", walkNiPSysModifierFloatCtlr);
        add("NiPSysInitialRotSpeedVarCtlr", walkNiPSysModifierFloatCtlr);
        add("NiSkinData", walkNiSkinData);
        add("NiSkinPartition", walkNiSkinPartition);
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
        add("NiTriShapeData", walkNiTriShapeData);
        add("NiTexturingProperty", walkNiTexturingProperty);
        add("NiSourceTexture", walkNiSourceTexture);
        add("NiSkinInstance", walkNiSkinInstance);
        // Extra data.
        add("BSXFlags", walkBSXFlags);
        add("NiStringExtraData", walkNiStringExtraData);
        add("NiBinaryExtraData", walkNiBinaryExtraData);
        // Animation: data, controllers, interpolators.
        add("NiTransformData", walkNiKeyframeData);
        add("NiFloatData", walkFloatData);
        add("NiBoolData", walkBoolData);
        add("NiPosData", walkPosData);
        add("NiColorData", walkColorData);
        add("NiTransformController", walkNiTransformController);
        add("NiTextureTransformController", walkNiTextureTransformController);
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

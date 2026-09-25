#include "nifblockfile.hpp"

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
    mBlockTypes.clear();
    mTypeIndex.clear();
    mBlockSize.clear();
    mStrings.clear();
    mGroupIds.clear();
    mBlocks.clear();
    mTrailing.clear();
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
    if (magic != "Gamebryo File Format, Version 20.2.0.7")
        return bad(QStringLiteral("unsupported version line '%1'").arg(QString::fromLatin1(magic.left(60))));

    mVersion = c.u32();
    const quint8 endian = c.u8();
    if (!c.ok() || endian != 1)
        return bad(QStringLiteral("unsupported byte order %1").arg(endian));

    mUserVersion = c.u32();
    const quint32 numBlocks = c.u32();
    mBsVersion = c.u32();
    if (!c.ok() || numBlocks == 0 || numBlocks > kMaxBlocks || mBsVersion == 0)
        return bad(QStringLiteral("implausible header: blocks=%1 bsVersion=%2")
                       .arg(numBlocks).arg(mBsVersion));

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
    for (quint32 i = 0; i < numBlocks; ++i)
        mBlockSize.append(c.u32());
    if (!c.ok()) return bad(QStringLiteral("truncated block size table"));

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

    const quint32 numGroups = c.u32();
    if (!c.ok() || numGroups > kMaxGroups)
        return bad(QStringLiteral("implausible group count %1").arg(numGroups));
    mGroupIds.clear();
    for (quint32 i = 0; i < numGroups; ++i)
        mGroupIds.append(c.u32());
    if (!c.ok()) return bad(QStringLiteral("truncated group table"));

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
    out.append("Gamebryo File Format, Version 20.2.0.7\n");
    appendU32(out, mVersion);
    appendU8(out, 1);
    appendU32(out, mUserVersion);
    appendU32(out, static_cast<quint32>(mBlocks.size()));
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
    for (quint32 size : mBlockSize) appendU32(out, size);

    appendU32(out, static_cast<quint32>(mStrings.size()));
    appendU32(out, mMaxStringLen);
    for (const QString& s : mStrings) {
        const QByteArray bytes = s.toLatin1();
        appendU32(out, static_cast<quint32>(bytes.size()));
        out.append(bytes);
    }

    appendU32(out, static_cast<quint32>(mGroupIds.size()));
    for (quint32 id : mGroupIds) appendU32(out, id);

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

#ifndef NIFBLOCKFILE_HPP
#define NIFBLOCKFILE_HPP

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

#include "nifparser.hpp"

// Block-level reader/writer for the Bethesda "Gamebryo File Format,
// Version 20.2.0.7" container used by every shipped NIF.
//
// The parser in nifparser.cpp builds a render-ready Node tree and throws the
// rest of the file away, which makes a lossless save impossible: any block it
// does not model (NiTexture, BSLightingShaderProperty, SkinAttach, ...) would
// be dropped, and animation edits could only be written back as the internal
// dialect. NifBlockFile instead keeps the header, the string table and every
// block payload byte-for-byte, so a save only has to replace the blocks it
// actually changed. Unknown blocks are never re-encoded, so they cannot drift.
class NifBlockFile
{
public:
    struct Block {
        QString type;      // resolved class name (blockTypes[typeIndex[i]])
        QByteArray data;   // exact payload, blockSize[i] bytes
        QByteArray footer; // trailing u32 block footer, 4 bytes or empty
    };

    // True when the file starts with the Bethesda version line.
    static bool isBethesdaNif(const QString& path);

    bool load(const QString& path);

    // Human-readable reason the last load() returned false.
    QString lastError() const { return mLastError; }

    // Why the last load() could not split the block region, when it could not.
    // Empty when the region was split, or when the file has a size table. This
    // is the signal for which block type still needs a payload layout, so it
    // names the type rather than reporting a position.
    QString lastWalkError() const { return mWalkError; }

    int count() const { return mBlocks.size(); }
    const Block& block(int index) const { return mBlocks.at(index); }
    void setBlockData(int index, const QByteArray& data);

    // Some packed BA2 assets carry "stub" NIFs: a lone BSWeakReferenceNode
    // node whose payload points at real geometry shipped elsewhere, never
    // inline. Sampling those as if they were real geometry is what made an
    // earlier survey conclude no vertices existed. Returns true when the
    // block list contains a BSWeakReferenceNode type and no inline
    // geometry-bearing type (BSGeometry/NiTriSh*/BSFaceGen/skin blocks).
    bool allWeakReferenceStub() const {
        if (mBlocks.isEmpty()) return false;
        bool sawWeak = false;
        for (const Block& b : mBlocks) {
            const QString& t = b.type;
            if (t == QLatin1String("BSWeakReferenceNode")) { sawWeak = true; continue; }
            if (t.contains(QLatin1String("Geometry"), Qt::CaseInsensitive)
                || t.contains(QLatin1String("TriSh"), Qt::CaseInsensitive)
                || t.contains(QLatin1String("TriStrip"), Qt::CaseInsensitive)
                || t.contains(QLatin1String("FaceGen"), Qt::CaseInsensitive)
                || t.contains(QLatin1String("Skin"), Qt::CaseInsensitive))
                return false;
        }
        return sawWeak;
    }

    // First block whose resolved type equals typeName, or -1.
    int findBlock(const QString& typeName) const;

    // All block indices whose resolved type equals typeName.
    QList<int> findBlocks(const QString& typeName) const;

    quint32 bsVersion() const { return mBsVersion; }
    quint32 version() const { return mVersion; }
    /// Dotted version from the header line, e.g. "20.0.0.4" or "20.2.0.7".
    QString headerVersion() const { return mHeaderVersion; }
    int stringCount() const { return mStrings.size(); }

    /// True when the file is the pre-Gamebryo "NetImmerse File Format"
    /// container. Its header is read (version, block count, and the type table
    /// when the generation has one) but its blocks are not: the container records
    /// no block lengths, so the payload region is kept whole and re-emitted
    /// verbatim. count() is therefore 0 for these files even though the header
    /// declares blocks.
    bool isNetImmerse() const { return mIsNetImmerse; }

    // False for the pre-20.2.0.5 container, which carries no per-block size
    // table. Those files load and re-save byte for byte, but the block region
    // cannot be split into individual blocks: the file simply does not record
    // where one block ends and the next begins, so only a per-block-type payload
    // parser could recover that, and there is not one here yet. Callers that
    // want individual blocks must check this before asking for them.
    bool hasIndividualBlocks() const { return mBlockRegion.isEmpty(); }

    struct KeyGroup {
        quint32 count = 0;
        quint32 interpolation = 0; // 0 if count==0, else 1=linear, 2=quadratic, 3=TBC
        QVector<float> times;
        QVector<float> values;   // count * valueWidth floats
        QVector<float> tangents; // quadratic: count*2*valueWidth, TBC: count*3
    };

    struct NiTransformDataRaw {
        bool valid = false;
        quint32 numRotationKeys = 0;
        quint32 rotationType = 0; // only present if numRotationKeys > 0
        QVector<KeyGroup> rotationGroups; // 3 for XYZ, 1 for quaternion
        KeyGroup translation;             // Vector3 values
        KeyGroup scale;                   // scalar values
    };

    // A keyframe controller's animation as one flat channel list. `nodeName`
    // is the node the controller animates, and `clipName` is the name a
    // NiControllerSequence gives the controller when it has one. `keyframes`
    // follows the writer's flat convention: one entry at the sorted union of
    // every channel's times, each channel sampled at its own nearest earlier
    // key.
    struct AnimationChannelSource {
        QString clipName;
        QString nodeName;
        QVector<Nif::TransformKeyframe> keyframes;
        // The original channel-preserving payload this source was flattened
        // from. Present for NiTransformData; other layouts set hasRaw false.
        bool hasRaw = false;
        NiTransformDataRaw raw;
    };

    // The flat keyframe list a NiTransformData channel set becomes. XYZ
    // rotation blocks set hasEuler; quaternion blocks set rotation.
    static QVector<Nif::TransformKeyframe> flattenNiTransformData(
        const NiTransformDataRaw& raw);

    // Every keyframe controller whose data block resolves and decodes.
    QVector<AnimationChannelSource> animationChannels() const;

    // How many blocks the header declares. This is known even for a container
    // with no size table, where count() is 0 because the payloads could not be
    // split: declaredBlockCount() is the number of blocks in the file,
    // count() is how many can be handed out individually.
    int declaredBlockCount() const { return mTypeIndex.size(); }

    // Resolved class name of declared block i. Valid whether or not the
    // container could be split, so it still answers "what is in this file".
    QString declaredBlockType(int index) const
    {
        if (index < 0 || index >= mTypeIndex.size()) return QString();
        return mBlockTypes.value(mTypeIndex.at(index));
    }

    // How far the split got, even when it did not finish.
    //
    // A container with no size table has to be walked block by block, and a
    // wrong payload length for any one of them makes every later block land in
    // the wrong place. When that happens lastWalkError() names the block the
    // walk gave up on, which is *not* the block that is wrong: by then the
    // position is already lost, so the error usually names whatever type it
    // happened to be reading when it read payload as a name. These give the
    // offsets of the blocks that did land correctly, so the first block whose
    // offset disagrees with the reference reader is the actual culprit and can
    // be found by comparison rather than guessed at.
    int walkedBlockCount() const { return mWalkedOffsets.size(); }
    int walkedBlockOffset(int index) const
    {
        if (index < 0 || index >= mWalkedOffsets.size()) return -1;
        return mWalkedOffsets.at(index);
    }
    QString walkedBlockType(int index) const
    {
        if (index < 0 || index >= mWalkedTypes.size()) return QString();
        return mWalkedTypes.at(index);
    }

    // Clip name -> owning controller block, gathered from every
    // NiControllerSequence. A clip name only exists here, so this is what ties
    // a controller to a named animation in the editor.
    QHash<quint32, QString> clipNamesByController() const;

    QByteArray serialize() const;
    bool save(const QString& path) const;

    // --- Keyframe codec -----------------------------------------------------
    // Real Bethesda layouts, not the internal dialect used by nifrecord.cpp.
    // The two data blocks differ per game generation, and both are decoded
    // strictly: trailing bytes mean the layout assumption is wrong and the
    // block is rejected rather than half-parsed.
    static bool decodeKeyframeData(const QString& blockType, const QByteArray& data,
                                   QVector<Nif::TransformKeyframe>& out);
    static bool encodeKeyframeData(const QString& blockType,
                                   const QVector<Nif::TransformKeyframe>& keyframes,
                                   QByteArray& out);

    // Channel-preserving representation for NiTransformData (Skyrim 1.5 /
    // Oblivion). The flat TransformKeyframe cannot represent per-channel key
    // counts, interpolation types, tangents, or independent times, so this
    // struct stores the raw block structure exactly as it appears on disk.
    static bool decodeNiTransformData(const QByteArray& data, quint32 version,
                                      NiTransformDataRaw& out);
    static bool encodeNiTransformData(const NiTransformDataRaw& raw, quint32 version,
                                      QByteArray& out);

    // Keyframe data layouts whose byte encoding is confirmed. NiKeyframeData
    // (Skyrim 1.6+, Fallout 4, Starfield) is a flat 44-byte-per-key array and
    // is verified by round-tripping shipped files. NiTransformData (Skyrim
    // 1.5 / Oblivion) uses the channel-preserving representation above and
    // is verified by round-tripping all 4,412 blocks in the test corpus.
    static bool isWritableKeyframeType(const QString& blockType);

    // NiKeyframeController fields are deliberately not decoded here: their
    // layout changed between game generations, and the target reference is not
    // at a fixed offset in the 1.5 form (it reads back as a null ref on every
    // shipped 1.5 file). The node -> controller direction is used instead.

    // NiObjectNET prefix: name index, extra-data list, controller ref. This is
    // the reliable node -> controller direction, because the fields inside a
    // controller block differ between game generations while this prefix does
    // not. Returns false if the block is not a node type or the prefix is
    // malformed — checking the type matters, since a data block's first word
    // can otherwise look like a valid name index.
    bool nodeNetInfo(int index, QString& nameOut, quint32& controllerRefOut) const;

    // Block types that derive from NiObjectNET and therefore carry the prefix.
    static bool isNodeBlockType(const QString& blockType);

    // The node name a controller's target ref points at, found by search because
    // the target field moves between game generations.
    QString controllerTargetName(int controllerIndex) const;

    // Resolve the keyframe data block a controller drives. Both the 1.5
    // (controller -> interpolator -> data) and 1.6+ (controller -> data)
    // chains are handled: the ref is the trailing u32 of the block, and the
    // result is only accepted when it is a known keyframe data type.
    int keyframeDataBlockFor(int controllerIndex) const;

    // NiControllerSequence: (controller ref, animated node name) pairs. The
    // entries are ControlledBlock records and every field of one is
    // version-conditional, so this reads the header version rather than
    // assuming a shape. A pre-20.1.0.1 container has no header string table, so
    // the node name is an offset into the NiStringPalette block the entry
    // points at; the name is empty when that lookup does not resolve, and
    // callers able to work from a node name and a controller ref alone should
    // not require it.
    bool decodeControllerSequence(const QByteArray& data,
                                  QList<QPair<quint32, QString>>& out) const;

    // The palette string at a byte position inside an NiStringPalette block. A
    // pre-20.1.0.1 container stores its palette as a NUL-terminated run inside
    // one length-prefixed blob, and a ControlledBlock's name offset is a
    // position in that blob. Empty when the block is absent or the position
    // does not land on one.
    QString stringAtPaletteOffset(quint32 paletteRef, quint32 offset) const;

    // NiTextKeyExtraData is the NIF's event/keyframe marker list attached to a
    // NiControllerSequence. The first string in the block is the NiExtraData
    // name; each key is (time, event name).
    struct TextKey {
        float time = 0.0f;
        QString text;
    };
    bool decodeTextKeys(const QByteArray& data, QString& extraNameOut,
                        QVector<TextKey>& out) const;
    QByteArray encodeTextKeys(const QString& extraName,
                              const QVector<TextKey>& keys) const;
    int textKeysBlockForClip(const QString& clipName) const;
    QVector<TextKey> textKeysForClip(const QString& clipName) const;
    bool setTextKeysForClip(const QString& clipName,
                            const QVector<TextKey>& keys);

void reset();
    bool parse(const QByteArray& raw, bool hasUnknownInt, QString& error);
    bool parseNetImmerse(const QByteArray& raw, QString& error);
    // Recover individual block boundaries in a container that has no size table,
    // by walking each block's fields. Fails unless every block type is
    // understood and the walk lands exactly on the trailing root table, so a
    // partial or wrong result is never used.
    bool splitBlockRegion(const QByteArray& raw, int startPos, QString& error);

    quint32 mVersion = 0x14020007;
    quint32 mUserVersion = 0;
    quint32 mBsVersion = 0;
    /// The dotted version from the header line, e.g. "20.0.0.7". Recorded
    /// so a caller can tell which game's file it is looking at.
    QString mHeaderVersion;
    /// The version line including its trailing newline, kept verbatim. The
    /// serializer must not rewrite it: the line is part of the file's bytes and
    /// a NIF that is re-saved as a different version stops being the same file.
    QByteArray mHeaderLine;
    quint32 mUnknownInt = 0;
    QByteArray mAuthor;
    QByteArray mExportScript;
    QByteArray mMaxFilepath;
    QStringList mBlockTypes;
    QVector<quint16> mTypeIndex;
    QVector<quint32> mBlockSize;
    QStringList mStrings;
    quint32 mMaxStringLen = 0;
    QVector<quint32> mGroupIds;
    /// The header bytes between the end of the last table we model (the string
    /// table, or the block-size table when there is no string table) and the
    /// first block payload, kept verbatim.
    ///
    /// This span holds the block-reference count and its references along with
    /// the group count and ids. The reference table is not modelled at all, and
    /// reconstructing the group table from mGroupIds loses four bytes for a file
    /// whose tables are empty - which is most of them. That does not corrupt the
    /// file so much as make it differ from the original by exactly that much,
    /// and only once the block region splits: while the region is opaque it is
    /// copied through whole, so the header mistake round-trips by coincidence.
    QByteArray mHeaderTail;
    QVector<Block> mBlocks;
    /// Offsets and types of the blocks the split walked successfully, kept even
    /// when the split then failed, so the divergence can be located.
    QVector<int> mWalkedOffsets;
    QStringList mWalkedTypes;
    /// Everything from the first block payload to end of file, for containers
    /// that carry no size table and therefore cannot be split. Empty otherwise.
    QByteArray mBlockRegion;
    bool mHasFooter = false;
    bool mHasUnknownInt = false;
    bool mIsNetImmerse = false;
    /// The complete header of a container whose field set this class does not
    /// model field-by-field (NetImmerse), kept verbatim. Emitting it unchanged
    /// is what makes such a file round-trip byte for byte; rebuilding it from
    /// parsed pieces would drop whichever conditional field this build does not
    /// know about, which is exactly the failure mode the Gamebryo path had.
    QByteArray mContainerHeader;
    QByteArray mTrailing;  // bytes after the block footer, preserved verbatim
    QString mLastError;
    QString mWalkError;
};

#endif // NIFBLOCKFILE_HPP

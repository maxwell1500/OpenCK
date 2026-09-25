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

    int count() const { return mBlocks.size(); }
    const Block& block(int index) const { return mBlocks.at(index); }
    void setBlockData(int index, const QByteArray& data);

    // First block whose resolved type equals typeName, or -1.
    int findBlock(const QString& typeName) const;

    // All block indices whose resolved type equals typeName.
    QList<int> findBlocks(const QString& typeName) const;

    quint32 bsVersion() const { return mBsVersion; }
    int stringCount() const { return mStrings.size(); }

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

    // Keyframe data layouts whose byte encoding is confirmed. NiKeyframeData
    // (Skyrim 1.6+, Fallout 4, Starfield) is a flat 44-byte-per-key array and
    // is verified by round-tripping shipped files. NiTransformData (Skyrim
    // 1.5 / Oblivion) is a different, three-channel layout whose encoding is
    // not yet confirmed, so writing it is refused rather than guessed.
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

    // Resolve the keyframe data block a controller drives. Both the 1.5
    // (controller -> interpolator -> data) and 1.6+ (controller -> data)
    // chains are handled: the ref is the trailing u32 of the block, and the
    // result is only accepted when it is a known keyframe data type.
    int keyframeDataBlockFor(int controllerIndex) const;

    // NiControllerSequence: (controller ref, clip name) pairs. This is the
    // only place a clip name is stored, so it is what ties a controller to a
    // named animation in the editor.
    static bool decodeControllerSequence(const QByteArray& data,
                                         QVector<QPair<quint32, QString>>& out);

private:
    void reset();
    bool parse(const QByteArray& raw, bool hasUnknownInt, QString& error);

    quint32 mVersion = 0x14020007;
    quint32 mUserVersion = 0;
    quint32 mBsVersion = 0;
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
    QVector<Block> mBlocks;
    bool mHasFooter = false;
    bool mHasUnknownInt = false;
    QByteArray mTrailing;  // bytes after the block footer, preserved verbatim
    QString mLastError;
};

#endif // NIFBLOCKFILE_HPP

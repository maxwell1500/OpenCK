#ifndef LIPFILE_HPP
#define LIPFILE_HPP

#include <QByteArray>
#include <QString>
#include <QVector>

#include "fuzparser.hpp"

// Decode-only support for the Skyrim/Fallout `.lip` payload that travels
// inside a `.fuz` voice container. `FuzParser` hands the lip half over as raw
// bytes; this parses it into the 16 FaceGen viseme curves the game plays.
//
// The format is the Bethesda/Fonix FaceFX lip format used by Oblivion,
// Skyrim, SSE and Fallout 3/4 alike: a header followed by an RLE payload that
// expands to curve slots grouped 33 to a frame. Frame N occupies slots
// [N*33, N*33+33).
//
// VERIFIED by the shipped archive: the header, the RLE payload, the 33 slots
// per frame, the payload start offset, and that every slot carries a
// normalised weight curve (curves do dip slightly below zero; the worst
// shipped line reaches -0.066).
//
// NOT VERIFIED: which slots are which. Slots 6..21 are conventionally read as
// the 16 FaceGen visemes in kVisemeNames order, but nothing on a stock
// developer machine corroborates it — no Bethesda binary or data file shipped
// to modders contains those names, and the curves themselves give no signal:
// across 7,932 frames of real voice data, no 16-wide slot window stands out
// from its neighbours, dominance falls off monotonically with slot index, and
// slots 6..21 are the *least* active group rather than a distinct set (slot 0
// alone is active on 26.6% of frames, 2.5x the busiest slot in the claimed
// window). Treat kVisemeNames as a display convention, not a fact, and never
// let it gate behaviour; value(row, slot) covers all 33 slots.
//
// The header is 24 bytes, but the payload does not always start right after it:
// five of the lines in `Skyrim - Voices_en0.bsa` carry one extra record-header
// byte, so the start offset is searched for rather than assumed. `gridSize` is
// derived from `frames` (33 slots and 28 bytes of padding per frame), which
// every shipped file agrees to the byte, so a header that disagrees with itself
// is rejected instead of guessed around.
//
// The RLE stream stops once the remaining curves are silent, so a payload may
// be shorter than the grid it fills; the absent tail is zero-filled and its size
// is reported by missingSlots() rather than silently absorbed. Consumers that
// draw a weight must clamp: shipped curves occasionally dip slightly below
// zero, the worst observed being -0.066.
//
// This is a **reader**. Nothing in OpenCK rewrites a lip payload: the editor
// keeps the original bytes verbatim, so an edited voice still round-trips
// exactly the way it was written by the generator.
class LipFile
{
public:
    static constexpr int kSlotsPerFrame = 33;
    // The conventional FaceGen window. UNVERIFIED — see the class comment.
    static constexpr int kVisemeBase = 6;
    static constexpr int kVisemeCount = 16;
    static constexpr int kHeaderSize = 24;
    static constexpr float kFps = 30.0f;

    // Conventional FaceGen names for the window above. UNVERIFIED — see the
    // class comment. Present for display only.
    static const char* const kVisemeNames[kVisemeCount];

    struct Header {
        quint32 version = 0;
        quint32 gridSize = 0;
        quint32 numCurves = 0;
        quint16 frames = 0;
        quint16 const14 = 0;
        qint32 first = 0;
        quint16 const20 = 0;
        quint16 u22 = 0;
    };

    LipFile() = default;

    // Decodes `data` (the lip half of a `.fuz`, or a standalone `.lip`).
    // Returns false and fills `error` when the payload cannot be decoded;
    // the caller keeps the original bytes in that case.
    static bool decode(const QByteArray& data, LipFile& out, QString* error = nullptr);

    // Decodes a `.fuz` container, splitting and decoding in one step.
    static bool decodeFuz(const QByteArray& fuzBytes, LipFile& out, QString* error = nullptr,
                          QString* audioFourCC = nullptr);

    const Header& header() const { return mHeader; }

    // One row of 33 curve slots per declared frame. The header's `frames` is
    // authoritative and `gridSize` is derived from it, not independent: see
    // gridSizeMatchesFrames().
    int rows() const { return mRows; }
    int slotAt(int row, int slot) const;
    float value(int row, int slot) const;

    // The conventional 16-viseme window for one frame, in kVisemeNames order.
    // Returns 16 zeros when the frame is out of range. Display only: the
    // window itself is unverified, so use value(row, slot) when it matters.
    QVarLengthArray<float, kVisemeCount> visemesAt(int row) const;

    // Seconds, or NaN when the frame is out of range.
    float frameTime(int row) const;

    // True when the header's own timing fields could be trusted. When false,
    // frameTime() still returns a usable value — the format's native 30 fps
    // with no pre-roll — so the editor draws something rather than nothing.
    bool hasTiming() const { return mHasTiming; }

    // Offset the RLE payload was found at (the header is sometimes followed
    // by extra record-header bytes), and how many floats it decoded to.
    int payloadOffset() const { return mPayloadOffset; }
    int payloadFloatCount() const { return mPayloadFloatCount; }

    // Declared slots the payload did not carry. The RLE stream stops when the
    // remaining curves are silent, so a small deficit is normal; a large one
    // means the shipped payload is truncated and the tail of the line plays
    // silent.
    int missingSlots() const { return mMissingSlots; }
    bool payloadIsComplete() const { return mMissingSlots == 0; }

    // Largest absolute viseme value across every frame. Useful for a summary
    // without walking the whole grid.
    float peakViseme(int visemeIndex) const;

    QString errorString() const { return mError; }

private:
    bool decodeInternal(const QByteArray& data);
    void resolveTiming(const QByteArray& data);

    Header mHeader;
    QVector<float> mGrid;
    int mRows = 0;
    bool mHasTiming = false;
    int mMissingSlots = 0;
    int mPayloadOffset = 0;
    int mPayloadFloatCount = 0;
    QString mError;
};

#endif // LIPFILE_HPP

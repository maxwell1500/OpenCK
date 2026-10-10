#include "lipfile.hpp"

#include <limits>

#include <QVarLengthArray>

#include "../log/logger.hpp"

namespace {

constexpr int kMaxExtraHeaderBytes = 8;
constexpr int kMaxDecodedBytes = 16 * 1024 * 1024;

// Every file in the shipped voice archive sizes its grid as exactly
// gridSize == frames * 132 + 28: 33 float slots and 28 bytes of leading
// padding per frame. Verified byte-for-byte on every real line, so agreement
// between the two header fields is a check on the header, not the payload.
constexpr int kGridSizeBias = 28;

// Trusting the header's timing fields is a two-out-of-three vote: `const20`
// has to say so and the pre-roll has to look like a pre-roll. Without both,
// frameTime() uses no pre-roll rather than whatever the field happens to hold.
constexpr quint16 kConst20TimingValid = 16;
constexpr int kMinPreRollFrames = -120;
constexpr int kMaxPreRollFrames = 0;

quint16 readU16(const QByteArray& data, int pos)
{
    if (pos < 0 || pos + 2 > data.size()) return 0;
    return static_cast<quint16>(static_cast<quint8>(data.at(pos))
        | (static_cast<quint8>(data.at(pos + 1)) << 8));
}

quint32 readU32(const QByteArray& data, int pos)
{
    if (pos < 0 || pos + 4 > data.size()) return 0;
    return static_cast<quint32>(static_cast<quint8>(data.at(pos))
        | (static_cast<quint8>(data.at(pos + 1)) << 8)
        | (static_cast<quint8>(data.at(pos + 2)) << 16)
        | (static_cast<quint8>(data.at(pos + 3)) << 24));
}

qint32 readI32(const QByteArray& data, int pos)
{
    return static_cast<qint32>(readU32(data, pos));
}

bool parseHeader(const QByteArray& data, LipFile::Header& header)
{
    if (data.size() < LipFile::kHeaderSize) return false;
    header.version = readU32(data, 0);
    header.gridSize = readU32(data, 4);
    header.numCurves = readU32(data, 8);
    header.frames = readU16(data, 12);
    header.const14 = readU16(data, 14);
    header.first = readI32(data, 16);
    header.const20 = readU16(data, 20);
    header.u22 = readU16(data, 22);
    return true;
}

// The payload is a byte stream where 0x00 starts a run of `count` zero bytes
// and any other byte is a literal. Long stretches of a mostly-zero grid
// compress to almost nothing, which is why a 3-second line is only a few
// hundred bytes.
bool unRle(const QByteArray& payload, QByteArray& out, QString* error)
{
    out.clear();
    out.reserve(payload.size());
    int index = 0;
    while (index < payload.size())
    {
        const quint8 token = static_cast<quint8>(payload.at(index));
        if (token == 0)
        {
            if (index + 3 > payload.size())
            {
                if (error) *error = QStringLiteral("truncated zero-run packet");
                return false;
            }
            const quint16 count = static_cast<quint16>(
                static_cast<quint8>(payload.at(index + 1))
                | (static_cast<quint8>(payload.at(index + 2)) << 8));
            if (count == 0)
            {
                if (error) *error = QStringLiteral("zero-length zero-run packet");
                return false;
            }
            if (out.size() + static_cast<int>(count) > kMaxDecodedBytes)
            {
                if (error)
                {
                    *error = QStringLiteral("decoded .lip payload exceeds %1 bytes")
                                 .arg(kMaxDecodedBytes);
                }
                return false;
            }
            out.append(static_cast<int>(count), '\0');
            index += 3;
        }
        else
        {
            if (out.size() >= kMaxDecodedBytes)
            {
                if (error)
                {
                    *error = QStringLiteral("decoded .lip payload exceeds %1 bytes")
                                 .arg(kMaxDecodedBytes);
                }
                return false;
            }
            out.append(static_cast<char>(token));
            ++index;
        }
    }
    return true;
}

float floatAt(const QByteArray& bytes, int offset)
{
    if (offset < 0 || offset + 4 > bytes.size()) return 0.0f;
    float value = 0.0f;
    memcpy(&value, bytes.constData() + offset, sizeof(value));
    return value;
}

} // namespace

const char* const LipFile::kVisemeNames[LipFile::kVisemeCount] = {
    "Aah", "BigAah", "BMP", "ChJSh", "DST", "Eee", "Eh", "FV",
    "I", "K", "N", "Oh", "OohQ", "R", "Th", "W"
};

bool LipFile::decode(const QByteArray& data, LipFile& out, QString* error)
{
    out = LipFile();
    if (!out.decodeInternal(data))
    {
        if (error) *error = out.errorString();
        return false;
    }
    return true;
}

bool LipFile::decodeFuz(const QByteArray& fuzBytes, LipFile& out, QString* error,
                        QString* audioFourCC)
{
    FuzParser container;
    if (!FuzParser::parse(fuzBytes, container))
    {
        if (error) *error = QStringLiteral("not a .fuz container");
        return false;
    }
    if (audioFourCC) *audioFourCC = container.audioFourCC;
    if (container.lipData.isEmpty())
    {
        if (error) *error = QStringLiteral("container carries no lip data");
        return false;
    }
    return decode(container.lipData, out, error);
}

bool LipFile::decodeInternal(const QByteArray& data)
{
    if (!parseHeader(data, mHeader))
    {
        mError = QStringLiteral("file is shorter than the .lip header (%1 bytes)")
                     .arg(LipFile::kHeaderSize);
        return false;
    }

    // The payload does not always start immediately after the 24-byte header:
    // files written by some generators carry extra record-header bytes. Try
    // each candidate and keep the one that recovers the most curve data.
    struct Candidate {
        int extra;
        int floats;
        QVector<float> values;
    };
    // `gridSize` is derived from `frames`, not independent: 33 float slots
    // and 28 bytes of leading padding per frame. Every file in the shipped
    // voice archive agrees exactly, so a header that disagrees with itself is
    // corrupt rather than merely unusual.
    const int declaredSlots = static_cast<int>(mHeader.frames) * LipFile::kSlotsPerFrame;
    if (mHeader.gridSize != declaredSlots * 4 + kGridSizeBias)
    {
        mError = QStringLiteral("header claims %1 frames but sizes its grid for %2")
                     .arg(mHeader.frames)
                     .arg(mHeader.gridSize);
        return false;
    }

    // The payload does not always start immediately after the 24-byte header:
    // files written by some generators carry extra record-header bytes. Try
    // each candidate and keep the one whose curve count lands closest to the
    // declared grid — a misaligned offset reads through the stream and cannot
    // come close to the right length.
    struct Candidate {
        int extra;
        int floats;
        QVector<float> values;
    };
    QVector<Candidate> candidates;
    for (int extra = 0; extra <= kMaxExtraHeaderBytes && declaredSlots > 0; ++extra)
    {
        const int offset = LipFile::kHeaderSize + extra;
        if (offset > data.size()) break;
        const QByteArray payload = data.mid(offset);
        QByteArray raw;
        if (!unRle(payload, raw, nullptr)) continue;
        if (raw.isEmpty() || raw.size() % 4 != 0) continue;

        const int count = raw.size() / 4;
        if (count <= 0) continue;

        bool sane = true;
        for (int i = 0; i < count; ++i)
        {
            const float value = floatAt(raw, i * 4);
            if (!(value >= -2.0f && value <= 2.0f))
            {
                sane = false;
                break;
            }
        }
        if (!sane) continue;

        Candidate candidate;
        candidate.extra = extra;
        candidate.floats = count;
        candidate.values.resize(count);
        for (int i = 0; i < count; ++i)
        {
            candidate.values[i] = floatAt(raw, i * 4);
        }
        candidates.append(candidate);
    }

    if (candidates.isEmpty())
    {
        mError = QStringLiteral("no payload offset in [%1, %2] holds a curve stream")
                     .arg(LipFile::kHeaderSize)
                     .arg(LipFile::kHeaderSize + kMaxExtraHeaderBytes);
        return false;
    }

    // Rank by how much of the declared grid the candidate implies it covers,
    // preferring the earliest offset on a tie.
    int bestIndex = 0;
    int bestDistance = qAbs(candidates.at(0).floats - declaredSlots);
    for (int i = 1; i < candidates.size(); ++i)
    {
        const int distance = qAbs(candidates.at(i).floats - declaredSlots);
        if (distance < bestDistance)
        {
            bestIndex = i;
            bestDistance = distance;
        }
    }

    const Candidate& chosen = candidates.at(bestIndex);
    const int floats = chosen.floats;

    // Curves beyond the declared grid have no slot to live in, and inventing a
    // row count the header never claimed would desynchronise the animation
    // from the audio it is meant to follow.
    if (floats > declaredSlots)
    {
        mError = QStringLiteral("payload carries %1 curve slots for a grid of %2")
                     .arg(floats)
                     .arg(declaredSlots);
        return false;
    }

    mPayloadOffset = LipFile::kHeaderSize + chosen.extra;
    mPayloadFloatCount = floats;

    // `frames` is the row count. The RLE stream stops once the remaining
    // curves are silent, so the tail is zero-filled rather than invented, and
    // how much was absent is reported.
    mRows = static_cast<int>(mHeader.frames);
    mGrid.fill(0.0f, static_cast<qsizetype>(mRows) * LipFile::kSlotsPerFrame);
    for (int i = 0; i < floats; ++i) mGrid[i] = chosen.values.at(i);
    mMissingSlots = declaredSlots - floats;

        resolveTiming(data);
    return true;
}

void LipFile::resolveTiming(const QByteArray& data)
{
    Q_UNUSED(data);

    // `first` is a pre-roll offset in frames, negative before the clip
    // starts, and `const20` says whether the pair is trustworthy. A shipped
    // line with a truncated payload carries garbage here; using it would put
    // every frame at the wrong time, so fall back to no pre-roll rather than
    // silently adopting a wild offset.
    mHasTiming = (mHeader.const20 == kConst20TimingValid
                  && mHeader.first >= kMinPreRollFrames
                  && mHeader.first <= kMaxPreRollFrames);
}

int LipFile::slotAt(int row, int slot) const
{
    if (row < 0 || row >= mRows || slot < 0 || slot >= kSlotsPerFrame) return -1;
    return row * kSlotsPerFrame + slot;
}

float LipFile::value(int row, int slot) const
{
    const int index = slotAt(row, slot);
    return index >= 0 ? mGrid.at(index) : 0.0f;
}

QVarLengthArray<float, LipFile::kVisemeCount> LipFile::visemesAt(int row) const
{
    QVarLengthArray<float, kVisemeCount> out(kVisemeCount);
    for (int i = 0; i < kVisemeCount; ++i)
    {
        out[i] = value(row, kVisemeBase + i);
    }
    return out;
}

float LipFile::frameTime(int row) const
{
    if (row < 0 || row >= mRows) return std::numeric_limits<float>::quiet_NaN();
    // Without trustworthy timing the clip still runs at the format's native
    // 30 fps, which is at least monotonic and correctly spaced.
    const float preRoll = mHasTiming ? static_cast<float>(mHeader.first) : 0.0f;
    return (static_cast<float>(row) + preRoll) / kFps;
}

float LipFile::peakViseme(int visemeIndex) const
{
    if (visemeIndex < 0 || visemeIndex >= kVisemeCount) return 0.0f;
    float peak = 0.0f;
    for (int row = 0; row < mRows; ++row)
    {
        const float magnitude = qAbs(value(row, kVisemeBase + visemeIndex));
        if (magnitude > peak) peak = magnitude;
    }
    return peak;
}

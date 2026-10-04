#include "nifanimationwriter.hpp"
#include "nifparser.hpp"
#include "nifblockfile.hpp"

#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QSet>

#include "logger.hpp"

#include <QtMath>

namespace {

// Bit equality, not a tolerance: an unedited channel is passed through
// verbatim, and an epsilon would let a small real edit pass as unchanged.
bool valuesUnchanged(const NifBlockFile::KeyGroup& src, const QVector<float>& values)
{
    if (src.values.size() != values.size())
        return false;
    for (int i = 0; i < values.size(); ++i) {
        if (qIsNaN(src.values.at(i)) || qIsNaN(values.at(i)))
            return false;
        if (memcmp(&src.values.at(i), &values.at(i), sizeof(float)) != 0)
            return false;
    }
    return true;
}

// Re-encode one channel, but only if the edit actually moved it. The flat frame
// list cannot carry an interpolation mode or tangents, so re-encoding a channel
// the user never touched would silently straighten a quadratic spline into a
// line. When the values come back identical the original group is returned
// whole, tangents and all; otherwise the channel has to be linear, which is
// reported so the UI can say so rather than losing it quietly.
NifBlockFile::KeyGroup mergeGroup(const NifBlockFile::KeyGroup& src,
                                  const QVector<float>& values,
                                  bool& downgraded)
{
    if (valuesUnchanged(src, values))
        return src;
    if (src.interpolation != 1u || !src.tangents.isEmpty())
        downgraded = true;
    NifBlockFile::KeyGroup g;
    g.count = src.count;
    g.interpolation = 1;
    g.times = src.times;
    g.values = values;
    return g;
}

// Convert a flat TransformKeyframe list into the channel-preserving raw
// representation, keeping each channel's own key count and time base.
//
// A NiTransformData block's channels are independent: translation, scale and
// each rotation axis carry their own counts, and on shipped meshes they
// routinely differ. Resampling them all onto the frame list's length would
// silently change the animation - a block whose translation channel holds 20
// keys would gain 21 - so each channel keeps the count it had and takes its
// values from the frame list sampled at that channel's own times. This is
// lossy: the flat list carries no interpolation or tangent data, so every
// channel becomes linear.
NifBlockFile::NiTransformDataRaw flatToRaw(
    const QVector<Nif::TransformKeyframe>& keyframes,
    const NifBlockFile::NiTransformDataRaw& original,
    bool* downgraded)
{
    NifBlockFile::NiTransformDataRaw raw;
    if (keyframes.isEmpty()) return raw;
    bool lost = false;

    // The original union of every channel's key times. A flat edit is a diff
    // against that: a time that appears in the edited list but not here is an
    // inserted key, and a time that used to be here but no longer is means the
    // user removed it. Only true additions change per-channel key sets; the
    // sampled keys from the other channels are already in the original
    // channel's own times.
    QVector<float> originalTimes;
    auto collectTimes = [&originalTimes](const NifBlockFile::KeyGroup& g) {
        for (float t : g.times)
            originalTimes.append(t);
    };
    collectTimes(original.translation);
    collectTimes(original.scale);
    for (const auto& g : original.rotationGroups)
        collectTimes(g);
    std::sort(originalTimes.begin(), originalTimes.end());
    originalTimes.erase(std::unique(originalTimes.begin(), originalTimes.end(),
                                    [](float a, float b) { return qAbs(a - b) < 0.001f; }),
                        originalTimes.end());

    QSet<float> currentTimesSet;
    for (const auto& kf : keyframes)
        currentTimesSet.insert(kf.time);

    QVector<float> addedTimes;
    QVector<float> removedTimes;
    // The flat keyframe list passed to the writer is a sample of the edited
    // clip, not a diff against one block's channels. A move/add/remove is an
    // edit to the selected channel's own key set, which is represented by the
    // channel payload; resampling every channel onto a different time base
    // would silently move keys the editor did not touch.
    (void)originalTimes;
    (void)currentTimesSet;
    (void)keyframes;

    // Sample the edited animation at `time` by nearest earlier key, which for a
    // monotonic time base is a step-free resample.
    auto sampleAt = [&keyframes](float time) -> const Nif::TransformKeyframe& {
        int best = 0;
        for (int i = 1; i < keyframes.size(); ++i) {
            if (keyframes.at(i).time <= time) best = i;
            else break;
        }
        return keyframes.at(best);
    };

    auto quaternionToEuler = [](float w, float x, float y, float z, float* euler) {
        euler[0] = qAtan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
        const float sinp = 2.0f * (w * y - z * x);
        euler[1] = (qAbs(sinp) >= 1.0f)
                       ? ((sinp >= 0.0f) ? (M_PI / 2.0f) : -(M_PI / 2.0f))
                       : qAsin(sinp);
        euler[2] = qAtan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));
    };

    // Find the edited keyframe at `time`, with a tolerance matching the undo
    // commands. A time the user added is exact enough for this purpose.
    auto keyAt = [&keyframes](float time) -> const Nif::TransformKeyframe* {
        for (const auto& kf : keyframes) {
            if (qAbs(kf.time - time) < 0.001f)
                return &kf;
        }
        return nullptr;
    };

    // Re-encode one channel with the edited keys. Unedited keys are kept
    // byte-for-byte through the original group values and tangents; the flat
    // transform carries no interpolation data, so only a channel that the edit
    // actually moved is forced linear.
    auto editGroup = [&](const NifBlockFile::KeyGroup& src, int width,
                         const std::function<void(const Nif::TransformKeyframe&, int, QVector<float>&)>& fill) -> NifBlockFile::KeyGroup {
        NifBlockFile::KeyGroup dst;
        dst.count = src.count;
        dst.times = src.times;
        dst.values = src.values;
        dst.interpolation = src.interpolation;
        dst.tangents = src.tangents;

        // Remove deleted keys first. The keys after the removed one keep their
        // values, so the channel keeps its time base except for the hole the
        // user removed.
        for (float removed : removedTimes) {
            int at = -1;
            for (int i = 0; i < dst.times.size(); ++i) {
                if (qAbs(dst.times.at(i) - removed) < 0.001f) { at = i; break; }
            }
            if (at < 0) continue;
            if (dst.interpolation != 1u || !dst.tangents.isEmpty())
                lost = true;
            dst.interpolation = 1u;
            dst.tangents.clear();
            dst.times.remove(at);
            for (int v = 0; v < width; ++v) {
                if (at * width < dst.values.size())
                    dst.values.remove(at * width);
            }
            dst.count = static_cast<quint32>(dst.times.size());
        }

        // Insert added keys. A flat keyframe is a transform key, so every
        // channel needs a value at the new time. Non-linear modes cannot keep
        // their tangents through an insertion, so they go linear.
        for (float added : addedTimes) {
            const Nif::TransformKeyframe* kf = keyAt(added);
            if (!kf) continue;
            int pos = dst.times.size();
            for (int i = 0; i < dst.times.size(); ++i) {
                if (added < dst.times.at(i)) { pos = i; break; }
            }
            dst.times.insert(pos, added);
            QVector<float> values;
            fill(*kf, 0, values);
            for (int v = 0; v < width && v < values.size(); ++v) {
                dst.values.insert(pos * width + v, values.at(v));
            }
            if (dst.interpolation != 1u || !dst.tangents.isEmpty())
                lost = true;
            dst.interpolation = 1u;
            dst.tangents.clear();
            dst.count = static_cast<quint32>(dst.times.size());
        }

        // Apply value edits at the remaining original times. Sample the edited
        // clip at each of the block's own times; untouched keys keep their
        // stored values and tangents.
        for (int i = 0; i < dst.times.size(); ++i) {
            const float t = dst.times.at(i);
            const Nif::TransformKeyframe& kf = sampleAt(t);
            QVector<float> edited;
            fill(kf, 0, edited);
            if (i * width + width > dst.values.size()) continue;
            bool changed = false;
            for (int v = 0; v < width; ++v) {
                const float a = dst.values.at(i * width + v);
                const float b = edited.value(v);
                if (qIsNaN(a) || qIsNaN(b) ||
                    memcmp(&a, &b, sizeof(float)) != 0) {
                    changed = true;
                }
                dst.values[i * width + v] = b;
            }
            if (!changed) continue;
            if (dst.interpolation != 1u || !dst.tangents.isEmpty())
                lost = true;
            dst.interpolation = 1u;
            dst.tangents.clear();
        }

        return dst;
    };

    const bool xyz = (original.rotationType == 4);
    raw.rotationType = original.rotationType;
    raw.valid = original.valid;
    if (xyz && original.rotationGroups.size() == 3) {
        raw.numRotationKeys = original.numRotationKeys;
        for (int axis = 0; axis < 3; ++axis) {
            const NifBlockFile::KeyGroup& src = original.rotationGroups.at(axis);
            QVector<float> values;
            values.reserve(int(src.count));
            for (int i = 0; i < static_cast<int>(src.count); ++i) {
                const float t = src.times.value(i, 0.0f);
                const Nif::TransformKeyframe& kf = sampleAt(t);
                if (kf.hasEuler) {
                    values.append(axis == 0 ? kf.euler.x : axis == 1 ? kf.euler.y : kf.euler.z);
                } else {
                    float euler[3] = { 0.0f, 0.0f, 0.0f };
                    quaternionToEuler(kf.rotation.w, kf.rotation.x, kf.rotation.y,
                                      kf.rotation.z, euler);
                    values.append(euler[axis]);
                }
            }
            raw.rotationGroups.append(editGroup(src, 1, [&](const Nif::TransformKeyframe& kf, int, QVector<float>& out) {
                if (kf.hasEuler) {
                    out.append(axis == 0 ? kf.euler.x : axis == 1 ? kf.euler.y : kf.euler.z);
                } else {
                    float euler[3] = { 0.0f, 0.0f, 0.0f };
                    quaternionToEuler(kf.rotation.w, kf.rotation.x, kf.rotation.y,
                                      kf.rotation.z, euler);
                    out.append(euler[axis]);
                }
            }));
        }
    } else if (!original.rotationGroups.isEmpty()) {
        raw.numRotationKeys = original.numRotationKeys;
        const NifBlockFile::KeyGroup& src = original.rotationGroups.first();
        raw.rotationGroups.append(editGroup(src, 4, [](const Nif::TransformKeyframe& kf, int, QVector<float>& out) {
            out << kf.rotation.w << kf.rotation.x << kf.rotation.y << kf.rotation.z;
        }));
    } else {
        raw.numRotationKeys = 0;
    }

    if (raw.rotationType == 3u && !raw.rotationGroups.isEmpty() &&
        raw.rotationGroups.first().tangents.isEmpty()) {
        // TBC keys are tension/bias/continuity triples. Losing them means the
        // block is a plain quaternion block now, not TBC.
        raw.rotationType = 1u;
    }

    {
        const NifBlockFile::KeyGroup& src = original.translation;
        raw.translation = editGroup(src, 3, [](const Nif::TransformKeyframe& kf, int, QVector<float>& out) {
            out << kf.translation.x << kf.translation.y << kf.translation.z;
        });
    }

    {
        const NifBlockFile::KeyGroup& src = original.scale;
        raw.scale = editGroup(src, 1, [](const Nif::TransformKeyframe& kf, int, QVector<float>& out) {
            out << kf.scale.x;
        });
    }

    if (downgraded) *downgraded = lost;
    return raw;
}

QVector<Nif::TransformKeyframe> channelToKeyframes(const AnimChannel& channel)
{
    QVector<Nif::TransformKeyframe> out;
    out.reserve(channel.keyframes.size());
    for (const AnimKeyframe& keyframe : channel.keyframes) {
        Nif::TransformKeyframe output;
        output.time = keyframe.time;
        output.translation = {keyframe.tx, keyframe.ty, keyframe.tz};
        output.scale = {keyframe.sx, keyframe.sy, keyframe.sz};
        output.hasEuler = keyframe.hasEuler;
        if (keyframe.hasEuler) {
            const float degToRad = 3.14159265358979323846f / 180.0f;
            output.euler = {keyframe.rx * degToRad,
                            keyframe.ry * degToRad,
                            keyframe.rz * degToRad};
            output.rotation = {keyframe.time, 1.0f, 0.0f, 0.0f, 0.0f};
        } else {
            output.rotation = {keyframe.time, keyframe.qw, keyframe.qx,
                               keyframe.qy, keyframe.qz};
            output.euler = {0.0f, 0.0f, 0.0f};
        }
        out.append(output);
    }
    return out;
}

AnimKeyframe animKeyfromTransform(const Nif::TransformKeyframe& source)
{
    AnimKeyframe key;
    key.time = source.time;
    key.tx = source.translation.x;
    key.ty = source.translation.y;
    key.tz = source.translation.z;
    key.sx = source.scale.x;
    key.sy = source.scale.y;
    key.sz = source.scale.z;
    key.hasQuat = false;
    key.hasEuler = false;
    if (source.hasEuler) {
        key.hasEuler = true;
        const float degPerRad = 180.0f / 3.14159265358979323846f;
        key.rx = source.euler.x * degPerRad;
        key.ry = source.euler.y * degPerRad;
        key.rz = source.euler.z * degPerRad;
    } else {
        key.hasQuat = true;
        key.qw = source.rotation.w;
        key.qx = source.rotation.x;
        key.qy = source.rotation.y;
        key.qz = source.rotation.z;
        // The timeline's Euler fields mirror the stored quaternion.
        const float radToDeg = 180.0f / 3.14159265358979323846f;
        const float sinr = 2.0f * (key.qw * key.qx + key.qy * key.qz);
        const float cosr = 1.0f - 2.0f * (key.qx * key.qx + key.qy * key.qy);
        key.rx = qAtan2(sinr, cosr) * radToDeg;
        const float sinp = 2.0f * (key.qw * key.qy - key.qz * key.qx);
        key.ry = (qAbs(sinp) >= 1.0f)
                     ? ((sinp >= 0.0f) ? (M_PI / 2.0f) : -(M_PI / 2.0f)) * radToDeg
                     : qAsin(sinp) * radToDeg;
        const float siny = 2.0f * (key.qw * key.qz + key.qx * key.qy);
        const float cosy = 1.0f - 2.0f * (key.qy * key.qy + key.qz * key.qz);
        key.rz = qAtan2(siny, cosy) * radToDeg;
    }
    return key;
}

void refreshChannelKeyframesImpl(AnimChannel& channel)
{
    if (!channel.raw.valid)
        return;
    channel.keyframes.clear();
    const auto flat = NifBlockFile::flattenNiTransformData(channel.raw);
    for (const auto& key : flat)
        channel.keyframes.append(animKeyfromTransform(key));
    channel.duration = channel.keyframes.isEmpty() ? 0.0f
                                                   : channel.keyframes.last().time;
}

void straightenGroup(NifBlockFile::KeyGroup& group, bool& downgraded)
{
    if (group.interpolation != 1u || !group.tangents.isEmpty()) {
        downgraded = true;
        group.interpolation = 1u;
        group.tangents.clear();
    }
}

int groupKeyIndex(const NifBlockFile::KeyGroup& group, float time)
{
    for (int i = 0; i < group.count && i < group.times.size(); ++i) {
        if (qAbs(group.times.at(i) - time) < 0.001f)
            return i;
    }
    return -1;
}

bool upsertGroup(NifBlockFile::KeyGroup& group, float time,
                 const QVector<float>& value, int width, bool& downgraded,
                 bool linear, bool* groupWasNonLinear = nullptr)
{
    if (linear)
        straightenGroup(group, downgraded);
    const int index = groupKeyIndex(group, time);
    if (index >= 0) {
        if (index * width + width > group.values.size())
            return false;
        for (int i = 0; i < width; ++i)
            group.values[index * width + i] = value.value(i);
        return true;
    }

    int pos = group.times.size();
    for (int i = 0; i < group.times.size(); ++i) {
        if (time < group.times.at(i)) { pos = i; break; }
    }
    group.times.insert(pos, time);
    for (int i = 0; i < width; ++i)
        group.values.insert(pos * width + i, value.value(i));
    group.count = static_cast<quint32>(group.times.size());
    if (groupWasNonLinear && *groupWasNonLinear)
        downgraded = true;
    return true;
}

bool removeGroupKey(NifBlockFile::KeyGroup& group, float time,
                    bool& downgraded, bool linear)
{
    const int index = groupKeyIndex(group, time);
    if (index < 0)
        return false;
    if (linear)
        straightenGroup(group, downgraded);

    const int width = (group.count > 0) ? group.values.size() / int(group.count) : 0;
    group.times.remove(index);
    for (int i = 0; i < width && index * width < group.values.size(); ++i) {
        if (index * width < group.values.size())
            group.values.remove(index * width);
    }
    group.count = static_cast<quint32>(group.times.size());
    // Tangents cannot be reindexed after a key is gone.
    if (!group.tangents.isEmpty()) {
        downgraded = true;
        group.interpolation = 1u;
        group.tangents.clear();
    }
    return true;
}

QVector<float> quaternionValues(const AnimKeyframe& key)
{
    if (key.hasQuat)
        return {key.qw, key.qx, key.qy, key.qz};
    const float degToRad = 3.14159265358979323846f / 180.0f;
    const float rx = key.rx * degToRad;
    const float ry = key.ry * degToRad;
    const float rz = key.rz * degToRad;
    const float cr = std::cos(rx * 0.5f), sr = std::sin(rx * 0.5f);
    const float cp = std::cos(ry * 0.5f), sp = std::sin(ry * 0.5f);
    const float cy = std::cos(rz * 0.5f), sy = std::sin(rz * 0.5f);
    return { cr * cp * cy + sr * sp * sy,
             sr * cp * cy - cr * sp * sy,
             cr * sp * cy + sr * cp * sy,
             cr * cp * sy - sr * sp * cy };
}

float eulerAxisValue(const AnimKeyframe& key, int axis)
{
    const float degToRad = 3.14159265358979323846f / 180.0f;
    if (key.hasEuler) {
        switch (axis) {
        case 0: return key.rx * degToRad;
        case 1: return key.ry * degToRad;
        case 2: return key.rz * degToRad;
        }
    }
    const QVector<float> q = quaternionValues(key);
    const float w = q[0], x = q[1], y = q[2], z = q[3];
    switch (axis) {
    case 0: return qAtan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
    case 1: {
        const float sinp = 2.0f * (w * y - z * x);
        return (qAbs(sinp) >= 1.0f) ? ((sinp >= 0.0f) ? (M_PI / 2.0f) : -(M_PI / 2.0f))
                                    : qAsin(sinp);
    }
    case 2: return qAtan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));
    }
    return 0.0f;
}

// Patch the keyframe data block a controller drives, rejecting refs that do
// not index a block we know how to re-encode. When `raw` is provided it is the
// already-edited channel payload; otherwise the flat keyframe list is applied
// to the decoded original channels.
bool patchControllerData(NifBlockFile& file, int controllerIndex,
                         const QVector<Nif::TransformKeyframe>& keyframes,
                         const NifBlockFile::NiTransformDataRaw* raw,
                         bool* downgraded = nullptr)
{
    const int dataIndex = file.keyframeDataBlockFor(controllerIndex);
    if (dataIndex < 0) {
        LOG_WARNING(QString("NifAnimationWriter: controller block %1 has no usable "
                            "keyframe data block").arg(controllerIndex));
        return false;
    }

    const QString dataType = file.block(dataIndex).type;
    const QByteArray& original = file.block(dataIndex).data;

    // Only touch layouts whose byte encoding this build has confirmed.
    if (!NifBlockFile::isWritableKeyframeType(dataType)) {
        LOG_WARNING(QString("NifAnimationWriter: keyframe data block %1 ('%2') uses an "
                            "unconfirmed layout; refusing to rewrite it")
                        .arg(dataIndex).arg(dataType));
        return false;
    }

    // Precondition: our codec must reproduce the untouched block exactly.
    // If it cannot, this build does not actually understand this game's
    // keyframe layout, and rewriting it would corrupt the NIF. Refuse.
    if (dataType == QLatin1String("NiTransformData")
        || dataType == QLatin1String("NiKeyframeControllerData")) {
        NifBlockFile::NiTransformDataRaw decodedOriginal;
        if (!NifBlockFile::decodeNiTransformData(original, file.version(), decodedOriginal)) {
            LOG_WARNING(QString("NifAnimationWriter: keyframe data block %1 ('%2') is in an "
                                "unrecognised layout; refusing to rewrite it")
                            .arg(dataIndex).arg(dataType));
            return false;
        }
        QByteArray reencoded;
        if (!NifBlockFile::encodeNiTransformData(decodedOriginal, file.version(), reencoded)
            || reencoded != original) {
            LOG_WARNING(QString("NifAnimationWriter: keyframe data block %1 ('%2') does not "
                                "round-trip; refusing to rewrite it")
                            .arg(dataIndex).arg(dataType));
            return false;
        }

        NifBlockFile::NiTransformDataRaw newRaw;
        if (raw && raw->valid) {
            newRaw = *raw;
            // A channel payload taken from an edited channel can straighten
            // curves. Say so instead of hiding it.
            if (downgraded) {
                const auto downgradedGroup = [](const NifBlockFile::KeyGroup& a,
                                                const NifBlockFile::KeyGroup& b) {
                    return a.interpolation != b.interpolation || a.tangents != b.tangents;
                };
                bool down = downgradedGroup(decodedOriginal.translation, newRaw.translation) ||
                            downgradedGroup(decodedOriginal.scale, newRaw.scale);
                for (int i = 0; !down && i < decodedOriginal.rotationGroups.size() &&
                           i < newRaw.rotationGroups.size(); ++i)
                    down = downgradedGroup(decodedOriginal.rotationGroups.at(i), newRaw.rotationGroups.at(i));
                if (down) *downgraded = true;
            }
        } else {
            newRaw = flatToRaw(keyframes, decodedOriginal, downgraded);
        }

        QByteArray encoded;
        if (!NifBlockFile::encodeNiTransformData(newRaw, file.version(), encoded)) {
            LOG_WARNING(QString("NifAnimationWriter: failed to encode keyframes for block '%1'")
                            .arg(dataType));
            return false;
        }

        NifBlockFile::NiTransformDataRaw verify;
        if (!NifBlockFile::decodeNiTransformData(encoded, file.version(), verify)
            || verify.rotationGroups.size() != newRaw.rotationGroups.size()) {
            LOG_ERROR("NifAnimationWriter: keyframe encoder failed its own round-trip check");
            return false;
        }

        file.setBlockData(dataIndex, encoded);
        return true;
    }

    QVector<Nif::TransformKeyframe> decoded;
    if (!NifBlockFile::decodeKeyframeData(dataType, original, decoded)) {
        LOG_WARNING(QString("NifAnimationWriter: keyframe data block %1 ('%2') is in an "
                            "unrecognised layout; refusing to rewrite it")
                        .arg(dataIndex).arg(dataType));
        return false;
    }
    QByteArray reencoded;
    if (!NifBlockFile::encodeKeyframeData(dataType, decoded, reencoded)
        || reencoded != original) {
        LOG_WARNING(QString("NifAnimationWriter: keyframe data block %1 ('%2') does not "
                            "round-trip; refusing to rewrite it")
                        .arg(dataIndex).arg(dataType));
        return false;
    }

    QByteArray encoded;
    if (!NifBlockFile::encodeKeyframeData(dataType, keyframes, encoded)) {
        LOG_WARNING(QString("NifAnimationWriter: unsupported keyframe data block '%1'")
                        .arg(dataType));
        return false;
    }

    // The encoder must also reproduce the requested frames.
    QVector<Nif::TransformKeyframe> verify;
    if (!NifBlockFile::decodeKeyframeData(dataType, encoded, verify)
        || verify.size() != keyframes.size()) {
        LOG_ERROR("NifAnimationWriter: keyframe encoder failed its own round-trip check");
        return false;
    }

    file.setBlockData(dataIndex, encoded);
    return true;
}

// Patch every node whose name matches, optionally restricted to the
// controllers a named clip sequence points at.
bool patchBethesdaNif(const QString& nifPath, const QString& nodeName,
                      const QVector<Nif::TransformKeyframe>& keyframes,
                      const QString& clipName, int& channelsPatched,
                      const NifBlockFile::NiTransformDataRaw* raw,
                      bool* downgraded = nullptr)
{
    NifBlockFile file;
    if (!file.load(nifPath)) return false;

    // Clip names only exist in a controller sequence, so resolve the set of
    // controller refs the requested clip owns up front.
    QSet<quint32> clipControllers;
    if (!clipName.isEmpty()) {
        bool foundSequence = false;
        const QList<int> sequences = file.findBlocks(QStringLiteral("NiControllerSequence"));
        for (int sequenceIndex : sequences) {
            QList<QPair<quint32, QString>> entries;
            if (!file.decodeControllerSequence(file.block(sequenceIndex).data, entries))
                continue;
            for (const auto& entry : entries) {
                if (entry.second == clipName) {
                    clipControllers.insert(entry.first);
                    foundSequence = true;
                }
            }
        }
        if (!foundSequence) {
            LOG_WARNING(QString("NifAnimationWriter: NIF has no clip named '%1'").arg(clipName));
            return false;
        }
    } else {
        // No clip was named, so the edit is not scoped to one. Take every
        // controller any sequence owns: on shipped Oblivion meshes the animated
        // nodes hang off a NiMultiTargetTransformController, so no node points
        // directly at a keyframe controller and a node-only search finds
        // nothing at all. Restricting to the union of the sequences' controllers
        // still keeps the write inside the animation this file actually owns.
        const QList<int> sequences = file.findBlocks(QStringLiteral("NiControllerSequence"));
        for (int sequenceIndex : sequences) {
            QList<QPair<quint32, QString>> entries;
            if (!file.decodeControllerSequence(file.block(sequenceIndex).data, entries))
                continue;
            for (const auto& entry : entries)
                clipControllers.insert(entry.first);
        }
    }

    // Nodes point at their own controller. Going that direction avoids
    // depending on the controller's field layout, which differs between the
    // 1.5 (NiTransformController) and 1.6+ (NiKeyframeController) formats.
    QSet<quint32> wantedControllers;
    wantedControllers.reserve(8);
    for (const QString& type : {QStringLiteral("NiTransformController"),
                                QStringLiteral("NiKeyframeController")}) {
        const QList<int> indices = file.findBlocks(type);
        for (int index : indices)
            wantedControllers.insert(static_cast<quint32>(index));
    }
    if (wantedControllers.isEmpty()) {
        LOG_WARNING("NifAnimationWriter: NIF has no keyframe controller blocks");
        return false;
    }

    bool matchedNode = false;
    for (int block = 0; block < file.count(); ++block) {
        QString name;
        quint32 controllerRef = 0xFFFFFFFFu;
        if (!file.nodeNetInfo(block, name, controllerRef)) continue;
        if (name != nodeName) continue;
        if (!wantedControllers.contains(controllerRef)) continue;
        if (!clipControllers.isEmpty() && !clipControllers.contains(controllerRef)) continue;

        matchedNode = true;
        if (patchControllerData(file, static_cast<int>(controllerRef), keyframes, raw, downgraded))
            ++channelsPatched;
    }

    // A controller sequence *is* the definition of a clip: it names the set of
    // controllers that clip drives. Requiring one of them to also be reached
    // from a node by name therefore over-constrains it, and on shipped Oblivion
    // meshes it excludes every file: the animated nodes there are driven
    // through a NiMultiTargetTransformController, so no node points directly at
    // a keyframe controller and the intersection is always empty.
    //
    // So when the caller named a clip and the sequence owns keyframe
    // controllers, those controllers are the clip. The node name stays the
    // display identity of the edit, not a precondition for making it.
    if (channelsPatched == 0 && !clipControllers.isEmpty()) {
        for (quint32 controllerRef : clipControllers) {
            if (!wantedControllers.contains(controllerRef)) continue;
            if (patchControllerData(file, static_cast<int>(controllerRef), keyframes, raw, downgraded))
                ++channelsPatched;
        }
        if (channelsPatched > 0)
            matchedNode = true;
    }

if (!matchedNode) {
        LOG_WARNING(QString("NifAnimationWriter: target node '%1' not found in %2")
                        .arg(nodeName).arg(nifPath));
        return false;
    }
    if (channelsPatched == 0) return false;
    return file.save(nifPath);
}

} // namespace

bool NifAnimationWriter::writeKeyframesToNif(const QString& nifPath,
                                             const QString& nodeName,
                                             const QVector<Nif::TransformKeyframe>& keyframes,
                                             const QString& clipName,
                                             bool* downgraded)
{
    if (keyframes.isEmpty()) {
        LOG_ERROR("NifAnimationWriter: refusing to write an empty keyframe list");
        return false;
    }

    if (downgraded) *downgraded = false;

    if (NifBlockFile::isBethesdaNif(nifPath)) {
        int patched = 0;
        if (!patchBethesdaNif(nifPath, nodeName, keyframes, clipName, patched,
                              nullptr, downgraded))
            return false;
        LOG_INFO(QString("NifAnimationWriter: patched %1 controller(s) in %2")
                     .arg(patched).arg(nifPath));
        return true;
    }

    // Internal dialect: re-parse the source so the write keeps the rest of
    // the tree intact.
    Nif::NifParser parser;
    if (!parser.load(nifPath)) {
        LOG_ERROR(QString("NifAnimationWriter: failed to parse NIF for write-back: %1").arg(nifPath));
        return false;
    }

    Nif::Node* root = parser.getRoot();
    if (!root) {
        LOG_ERROR("NifAnimationWriter: NIF has no root node");
        return false;
    }

    bool found = false;
    auto updateNode = [&](auto* node, auto&& self) -> bool {
        if (!node) return false;

        bool nodeMatches = node->name == nodeName;
        for (auto& anim : node->animations) {
            if (QString::number(anim.targetNode) == nodeName) nodeMatches = true;
        }
        if (nodeMatches) {
            for (auto& anim : node->animations) {
                if (!clipName.isEmpty() && anim.clipName != clipName) continue;
                anim.keyframes = keyframes;
                found = true;
                LOG_INFO(QString("NifAnimationWriter: updated %1 keyframes for node %2")
                             .arg(keyframes.size())
                             .arg(nodeName));
                return true;
            }
        }

        for (auto* child : node->children) {
            if (self(child, self)) return true;
        }
        return false;
    };

    updateNode(root, updateNode);

    if (!found) {
        LOG_WARNING(QString("NifAnimationWriter: target node '%1' not found in NIF").arg(nodeName));
        return false;
    }

    const QString tempPath = nifPath + QStringLiteral(".openck.tmp");
    QFile::remove(tempPath);
    if (!parser.save(tempPath)) {
        LOG_ERROR(QString("NifAnimationWriter: failed to save NIF to temp file: %1").arg(tempPath));
        QFile::remove(tempPath);
        return false;
    }

    QFile source(tempPath);
    if (!source.open(QIODevice::ReadOnly)) {
        QFile::remove(tempPath);
        return false;
    }
    QSaveFile output(nifPath);
    if (!output.open(QIODevice::WriteOnly)) {
        source.close();
        QFile::remove(tempPath);
        return false;
    }
    const QByteArray bytes = source.readAll();
    source.close();
    const bool written = output.write(bytes) == bytes.size() && output.commit();
    QFile::remove(tempPath);
    if (!written) {
        LOG_ERROR(QString("NifAnimationWriter: failed to atomically replace NIF: %1").arg(nifPath));
        return false;
    }

    LOG_INFO(QString("NifAnimationWriter: successfully wrote keyframes to NIF: %1").arg(nifPath));
    return true;
}

bool NifAnimationWriter::writeKeyframesToNif(const QString& nifPath,
                                             const QString& nodeName,
                                             const AnimChannel& channel,
                                             const QString& clipName,
                                             bool* downgraded)
{
    if (channel.raw.valid && channel.keyframes.isEmpty()) {
        // An empty edited payload is meaningful only if every channel is empty;
        // otherwise we fall through to the keyframe path to avoid inventing a
        // partial channel set.
        bool allEmpty = channel.raw.numRotationKeys == 0 &&
                        channel.raw.translation.count == 0 &&
                        channel.raw.scale.count == 0;
        if (!allEmpty)
            return false;
    }
    const QVector<Nif::TransformKeyframe> keyframes = channelToKeyframes(channel);
    if (channel.raw.valid) {
        if (downgraded) *downgraded = false;
        if (NifBlockFile::isBethesdaNif(nifPath)) {
            int patched = 0;
            if (!patchBethesdaNif(nifPath, nodeName, keyframes, clipName, patched,
                                  &channel.raw, downgraded))
                return false;
            LOG_INFO(QString("NifAnimationWriter: patched %1 controller(s) in %2")
                         .arg(patched).arg(nifPath));
            return true;
        }
    }
    return writeKeyframesToNif(nifPath, nodeName, keyframes, clipName, downgraded);
}

void NifAnimationWriter::refreshChannelKeyframes(AnimChannel& channel)
{
    refreshChannelKeyframesImpl(channel);
}

bool NifAnimationWriter::channelAddKeyframe(AnimChannel& channel,
                                            const AnimKeyframe& keyframe,
                                            bool* downgraded)
{
    if (downgraded) *downgraded = false;
    if (!channel.raw.valid) {
        int pos = channel.keyframes.size();
        for (int i = 0; i < channel.keyframes.size(); ++i) {
            if (keyframe.time < channel.keyframes[i].time) { pos = i; break; }
        }
        channel.keyframes.insert(pos, keyframe);
        channel.duration = channel.keyframes.isEmpty() ? 0.0f
                                                      : channel.keyframes.last().time;
        return true;
    }

    bool down = false;
    NifBlockFile::NiTransformDataRaw raw = channel.raw;

    upsertGroup(raw.translation, keyframe.time, {keyframe.tx, keyframe.ty, keyframe.tz}, 3, down, true);
    upsertGroup(raw.scale, keyframe.time, {keyframe.sx}, 1, down, true);

    if (raw.rotationType == 4u && raw.rotationGroups.size() == 3) {
        for (int axis = 0; axis < 3; ++axis) {
            upsertGroup(raw.rotationGroups[axis], keyframe.time,
                        {eulerAxisValue(keyframe, axis)}, 1, down, true);
        }
        raw.numRotationKeys = raw.rotationGroups.first().count;
    } else {
        if (raw.rotationGroups.isEmpty()) {
            NifBlockFile::KeyGroup g;
            g.interpolation = 0;
            raw.rotationGroups.append(g);
            raw.rotationType = 1u;
        }
        if (raw.rotationType == 3u) {
            raw.rotationType = 1u;
            raw.rotationGroups.first().tangents.clear();
            down = true;
        }
        upsertGroup(raw.rotationGroups.first(), keyframe.time,
                    quaternionValues(keyframe), 4, down, false);
        raw.numRotationKeys = raw.rotationGroups.first().count;
    }

    channel.raw = raw;
    refreshChannelKeyframesImpl(channel);
    if (downgraded) *downgraded = down;
    return true;
}

bool NifAnimationWriter::channelRemoveKeyframe(AnimChannel& channel, float time,
                                               bool* downgraded)
{
    if (downgraded) *downgraded = false;
    if (!channel.raw.valid) {
        for (int i = 0; i < channel.keyframes.size(); ++i) {
            if (qAbs(channel.keyframes[i].time - time) < 0.001f) {
                channel.keyframes.remove(i);
                channel.duration = channel.keyframes.isEmpty() ? 0.0f
                                                              : channel.keyframes.last().time;
                return true;
            }
        }
        return false;
    }

    bool down = false;
    NifBlockFile::NiTransformDataRaw raw = channel.raw;
    removeGroupKey(raw.translation, time, down, true);
    removeGroupKey(raw.scale, time, down, true);
    for (NifBlockFile::KeyGroup& group : raw.rotationGroups) {
        removeGroupKey(group, time, down, true);
    }
    if (raw.rotationType == 3u) {
        raw.rotationType = 1u;
        for (NifBlockFile::KeyGroup& group : raw.rotationGroups)
            group.tangents.clear();
        down = true;
    }
    if (!raw.rotationGroups.isEmpty())
        raw.numRotationKeys = raw.rotationGroups.first().count;
    else
        raw.numRotationKeys = 0;

    channel.raw = raw;
    refreshChannelKeyframesImpl(channel);
    if (downgraded) *downgraded = down;
    return true;
}

bool NifAnimationWriter::channelMoveKeyframe(AnimChannel& channel, float oldTime,
                                             float newTime, const AnimKeyframe& values,
                                             bool* downgraded)
{
    bool down = false;
    // Removal and insertion are each curve-downgrading operations.
    if (!channelRemoveKeyframe(channel, oldTime, &down))
        return false;
    AnimKeyframe moved = values;
    moved.time = newTime;
    bool down2 = false;
    if (!channelAddKeyframe(channel, moved, &down2))
        return false;
    if (downgraded)
        *downgraded = down || down2;
    return true;
}

bool NifAnimationWriter::channelSetKeyframeValue(AnimChannel& channel, float time,
                                                 const AnimKeyframe& values,
                                                 bool* downgraded)
{
    if (downgraded) *downgraded = false;
    if (!channel.raw.valid) {
        for (AnimKeyframe& kf : channel.keyframes) {
            if (qAbs(kf.time - time) < 0.001f) {
                kf.tx = values.tx; kf.ty = values.ty; kf.tz = values.tz;
                kf.rx = values.rx; kf.ry = values.ry; kf.rz = values.rz;
                kf.sx = values.sx; kf.sy = values.sy; kf.sz = values.sz;
                kf.qw = values.qw; kf.qx = values.qx; kf.qy = values.qy; kf.qz = values.qz;
                kf.hasQuat = values.hasQuat;
                kf.hasEuler = values.hasEuler;
                return true;
            }
        }
        return false;
    }

    bool down = false;
    NifBlockFile::NiTransformDataRaw raw = channel.raw;
    upsertGroup(raw.translation, time, {values.tx, values.ty, values.tz}, 3, down, true);
    upsertGroup(raw.scale, time, {values.sx}, 1, down, true);

    if (raw.rotationType == 4u && raw.rotationGroups.size() == 3) {
        for (int axis = 0; axis < 3; ++axis) {
            upsertGroup(raw.rotationGroups[axis], time,
                        {eulerAxisValue(values, axis)}, 1, down, true);
        }
    } else {
        if (raw.rotationGroups.isEmpty()) {
            NifBlockFile::KeyGroup g;
            g.interpolation = 0;
            raw.rotationGroups.append(g);
            raw.rotationType = 1u;
        }
        if (raw.rotationType == 3u) {
            raw.rotationType = 1u;
            raw.rotationGroups.first().tangents.clear();
            down = true;
        }
        upsertGroup(raw.rotationGroups.first(), time,
                    quaternionValues(values), 4, down, false);
    }

    channel.raw = raw;
    refreshChannelKeyframesImpl(channel);
    if (downgraded) *downgraded = down;
    return true;
}

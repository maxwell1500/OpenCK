#pragma once

#include <QString>
#include <QVector>

#include "nifanimation.hpp"

namespace Nif { struct TransformKeyframe; }

class NifAnimationWriter
{
public:
    // Set when the write had to straighten a channel that carried a non-linear
    // interpolation mode or tangents. The edited keyframe list carries values
    // only, so a channel that actually moved cannot keep its curve; the
    // untouched channels around it do keep theirs. Callers should say so
    // rather than let it pass unnoticed, because the change is visible when the
    // animation is played back.
    static bool writeKeyframesToNif(const QString& nifPath,
                                     const QString& nodeName,
                                     const QVector<Nif::TransformKeyframe>& keyframes,
                                     const QString& clipName = QString(),
                                     bool* downgraded = nullptr);

    // NIF-loaded channels carry their original channel-preserving payload. The
    // flat keyframe list is the edited view of that payload, so a save uses the
    // payload's own counts/times/interpolation rather than resampling every
    // channel onto the display frame list.
    static bool writeKeyframesToNif(const QString& nifPath,
                                     const QString& nodeName,
                                     const AnimChannel& channel,
                                     const QString& clipName = QString(),
                                     bool* downgraded = nullptr);

    static bool setClipMarkersToNif(const QString& nifPath,
                                    const QString& clipName,
                                    const QVector<AnimMarker>& markers);

    // Writes an entire NifAnimation back to the source NIF in a single pass.
    // Handles both Bethesda/NetImmerse NIFs (via NifBlockFile) and internal
    // dialect NIFs (via NifParser), saving atomically via QSaveFile.
    static bool writeAnimationToNif(const QString& nifPath,
                                    const NifAnimation& anim,
                                    int* savedCount = nullptr,
                                    int* failedCount = nullptr,
                                    int* downgradedCount = nullptr);

    // Channel payload editing helpers. These mutate channel.raw and rebuild the
    // flat keyframes the timeline shows, so keyframe add/remove/move and
    // property edits survive as per-channel data instead of a sampled transform
    // list.
    static bool channelAddKeyframe(AnimChannel& channel,
                                   const AnimKeyframe& keyframe,
                                   bool* downgraded = nullptr);
    static bool channelRemoveKeyframe(AnimChannel& channel, float time,
                                      bool* downgraded = nullptr);
    static bool channelMoveKeyframe(AnimChannel& channel, float oldTime,
                                    float newTime, const AnimKeyframe& values,
                                    bool* downgraded = nullptr);
    static bool channelSetKeyframeValue(AnimChannel& channel, float time,
                                        const AnimKeyframe& values,
                                        bool* downgraded = nullptr);
    static void refreshChannelKeyframes(AnimChannel& channel);
};

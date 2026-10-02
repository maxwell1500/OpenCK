#pragma once

#include <QString>
#include <QVector>

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
};

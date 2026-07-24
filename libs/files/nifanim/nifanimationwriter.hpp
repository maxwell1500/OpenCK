#pragma once

#include <QString>
#include <QVector>

namespace Nif { struct TransformKeyframe; }

class NifAnimationWriter
{
public:
    static bool writeKeyframesToNif(const QString& nifPath,
                                     const QString& nodeName,
                                     const QVector<Nif::TransformKeyframe>& keyframes);
};

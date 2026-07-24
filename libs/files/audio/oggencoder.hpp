#pragma once

#include <QVector>
#include <QString>

class OggEncoder
{
public:
    static bool encode(const QVector<float>& samples, const QString& outPath,
                       int sampleRate = 44100, int channels = 1, int quality = 3);
};

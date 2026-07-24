#pragma once

#include <QImage>
#include <QVector>
#include <QString>

class WavEncoder
{
public:
    static bool encode(const QVector<float>& samples, const QString& outPath,
                       int sampleRate = 44100, int channels = 1, int bitsPerSample = 16);
    static bool encodeFromImage(const QImage& waveform, const QString& outPath,
                                int sampleRate = 44100);
};

#pragma once

#include <QImage>
#include <QVector>
#include <QString>

class OggDecoder
{
public:
    struct OggInfo
    {
        int sampleRate = 0;
        int channels = 0;
        int totalSamples = 0;
        int durationSeconds = 0;
        bool valid = false;
    };

    static QImage decodeToWaveform(const QString& filePath, int width = 800, int height = 200);
    static QVector<float> decodeSamples(const QString& filePath);
    static int getSampleRate(const QString& filePath);
    static int getDuration(const QString& filePath);
    static OggInfo getOggInfo(const QString& filePath);

private:
    static OggInfo parseOggHeader(const QString& filePath);
};

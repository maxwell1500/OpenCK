#include "oggdecoder.hpp"

#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

#include "log/logger.hpp"

#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_STDIO
#include "../../../external/stb_vorbis.h"

QImage OggDecoder::decodeToWaveform(const QString& filePath, int width, int height)
{
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(QColor(30, 30, 30));

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        QPainter p(&img);
        p.setPen(QColor(170, 170, 170));
        p.drawText(img.rect(), Qt::AlignCenter, QStringLiteral("Cannot open: %1").arg(filePath));
        return img;
    }

    QByteArray data = f.readAll();
    f.close();

    int channels = 0, sampleRate = 0;
    short* output = nullptr;
    int samples = stb_vorbis_decode_memory(
        reinterpret_cast<const unsigned char*>(data.constData()),
        data.size(), &channels, &sampleRate, &output);

    if (samples <= 0) {
        QPainter p(&img);
        p.setPen(QColor(170, 170, 170));
        p.drawText(img.rect(), Qt::AlignCenter, QStringLiteral("Cannot decode: %1").arg(filePath));
        return img;
    }

    QVector<float> floatSamples;
    floatSamples.reserve(samples);
    for (int i = 0; i < samples * channels; i += channels) {
        float val = 0.0f;
        for (int c = 0; c < channels; ++c)
            val += output[i + c];
        val /= channels;
        floatSamples.append(val / 32768.0f);
    }
    free(output);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen linePen(QColor(100, 180, 255), 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(linePen);

    float centerY = static_cast<float>(height) / 2.0f;
    float maxAmp = centerY * 0.9f;

    QPainterPath path;
    for (int x = 0; x < width; ++x) {
        int sampleIndex = (x * floatSamples.size()) / width;
        if (sampleIndex >= floatSamples.size()) sampleIndex = floatSamples.size() - 1;
        float y = centerY - floatSamples[sampleIndex] * maxAmp;
        if (x == 0)
            path.moveTo(x, y);
        else
            path.lineTo(x, y);
    }
    painter.drawPath(path);

    QPen mirrorPen(QColor(80, 140, 220, 180), 0.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(mirrorPen);
    QPainterPath mirrorPath;
    for (int x = 0; x < width; ++x) {
        int sampleIndex = (x * floatSamples.size()) / width;
        if (sampleIndex >= floatSamples.size()) sampleIndex = floatSamples.size() - 1;
        float y = centerY + floatSamples[sampleIndex] * maxAmp;
        if (x == 0)
            mirrorPath.moveTo(x, y);
        else
            mirrorPath.lineTo(x, y);
    }
    painter.drawPath(mirrorPath);

    QPen centerPen(QColor(255, 255, 255, 40), 1.0);
    painter.setPen(centerPen);
    painter.drawLine(0, static_cast<int>(centerY), width, static_cast<int>(centerY));

    return img;
}

QVector<float> OggDecoder::decodeSamples(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("OggDecoder: cannot open file: %1").arg(filePath));
        return {};
    }

    QByteArray data = f.readAll();
    f.close();

    int channels = 0, sampleRate = 0;
    short* output = nullptr;
    int samples = stb_vorbis_decode_memory(
        reinterpret_cast<const unsigned char*>(data.constData()),
        data.size(), &channels, &sampleRate, &output);

    if (samples <= 0) {
        LOG_ERROR(QString("OggDecoder: failed to decode: %1").arg(filePath));
        return {};
    }

    QVector<float> result;
    result.reserve(samples);
    for (int i = 0; i < samples * channels; i += channels) {
        float val = 0.0f;
        for (int c = 0; c < channels; ++c)
            val += output[i + c];
        val /= channels;
        result.append(val / 32768.0f);
    }
    free(output);
    return result;
}

int OggDecoder::getSampleRate(const QString& filePath)
{
    return getOggInfo(filePath).sampleRate;
}

int OggDecoder::getDuration(const QString& filePath)
{
    return getOggInfo(filePath).durationSeconds;
}

OggDecoder::OggInfo OggDecoder::getOggInfo(const QString& filePath)
{
    return parseOggHeader(filePath);
}

OggDecoder::OggInfo OggDecoder::parseOggHeader(const QString& filePath)
{
    OggInfo info;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("OggDecoder: cannot open file: %1").arg(filePath));
        return info;
    }

    QByteArray data = f.readAll();
    f.close();

    int error = 0;
    stb_vorbis* v = stb_vorbis_open_memory(
        reinterpret_cast<const unsigned char*>(data.constData()),
        data.size(), &error, nullptr);

    if (!v) {
        LOG_ERROR(QString("OggDecoder: not a valid OGG Vorbis file: %1 (error %2)").arg(filePath).arg(error));
        return info;
    }

    stb_vorbis_info vi = stb_vorbis_get_info(v);
    info.sampleRate = static_cast<int>(vi.sample_rate);
    info.channels = vi.channels;
    info.valid = true;

    unsigned int totalSamples = stb_vorbis_stream_length_in_samples(v);
    info.totalSamples = static_cast<int>(totalSamples);
    if (info.sampleRate > 0)
        info.durationSeconds = totalSamples / info.sampleRate;

    stb_vorbis_close(v);
    return info;
}

#include "wavencoder.hpp"

#include <QFile>
#include <QDataStream>
#include <QImage>

#include "log/logger.hpp"

static void writeLe16(QDataStream& ds, quint16 val)
{
    ds << val;
}

static void writeLe32(QDataStream& ds, quint32 val)
{
    ds << val;
}

bool WavEncoder::encode(const QVector<float>& samples, const QString& outPath,
                         int sampleRate, int channels, int bitsPerSample)
{
    if (samples.isEmpty()) {
        LOG_ERROR("WavEncoder: no samples to encode");
        return false;
    }

    if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 32) {
        LOG_ERROR(QString("WavEncoder: unsupported bits per sample: %1").arg(bitsPerSample));
        return false;
    }

    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("WavEncoder: cannot open output file: %1").arg(outPath));
        return false;
    }

    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::LittleEndian);

    int bytesPerSample = bitsPerSample / 8;
    int blockAlign = channels * bytesPerSample;
    int byteRate = sampleRate * blockAlign;
    int totalSamplesPerChannel = samples.size() / channels;
    int dataSize = totalSamplesPerChannel * blockAlign;

    ds.writeRawData("RIFF", 4);
    writeLe32(ds, 36 + static_cast<quint32>(dataSize));
    ds.writeRawData("WAVE", 4);

    ds.writeRawData("fmt ", 4);
    writeLe32(ds, 16);
    writeLe16(ds, 1);
    writeLe16(ds, static_cast<quint16>(channels));
    writeLe32(ds, static_cast<quint32>(sampleRate));
    writeLe32(ds, static_cast<quint32>(byteRate));
    writeLe16(ds, static_cast<quint16>(blockAlign));
    writeLe16(ds, static_cast<quint16>(bitsPerSample));

    ds.writeRawData("data", 4);
    writeLe32(ds, static_cast<quint32>(dataSize));

    for (int i = 0; i < samples.size(); ++i) {
        float sample = qBound(-1.0f, samples[i], 1.0f);

        if (bitsPerSample == 8) {
            quint8 val = static_cast<quint8>((sample + 1.0f) * 127.5f);
            ds << val;
        } else if (bitsPerSample == 16) {
            qint16 val = static_cast<qint16>(sample * 32767.0f);
            ds << val;
        } else if (bitsPerSample == 32) {
            qint32 val = static_cast<qint32>(sample * 2147483647.0f);
            ds << val;
        }
    }

    f.close();

    LOG_INFO(QString("WavEncoder: wrote %1 samples (%2-bit, %3 Hz, %4 ch) to %5")
                 .arg(samples.size()).arg(bitsPerSample).arg(sampleRate).arg(channels).arg(outPath));
    return true;
}

bool WavEncoder::encodeFromImage(const QImage& waveform, const QString& outPath, int sampleRate)
{
    if (waveform.isNull()) {
        LOG_ERROR("WavEncoder: null image for encoding");
        return false;
    }

    int width = waveform.width();
    if (width <= 0) {
        LOG_ERROR("WavEncoder: image width is zero");
        return false;
    }

    QVector<float> samples;
    samples.reserve(width);

    for (int x = 0; x < width; ++x) {
        float minVal = 1.0f;
        float maxVal = -1.0f;

        for (int y = 0; y < waveform.height(); ++y) {
            QRgb pixel = waveform.pixel(x, y);
            int luminance = qGray(pixel);

            float normalized = static_cast<float>(luminance) / 128.0f - 1.0f;

            if (normalized < minVal) minVal = normalized;
            if (normalized > maxVal) maxVal = normalized;
        }

        float amplitude = (maxVal - minVal) * 0.5f;
        float center = (minVal + maxVal) * 0.5f;
        float sample = center + amplitude * 0.5f;

        sample = qBound(-1.0f, sample, 1.0f);
        samples.append(sample);
    }

    return encode(samples, outPath, sampleRate, 1, 16);
}

#include "xwmadecoder.hpp"

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mftransform.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <wrl/client.h>

#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>

#include "../log/logger.hpp"

using Microsoft::WRL::ComPtr;

namespace {

void logHr(HRESULT hr, const char* what)
{
    LOG_ERROR(QString("%1 failed: 0x%2").arg(QString::fromLatin1(what)).arg(static_cast<quint32>(hr), 8, 16, QChar('0')));
}

// Reads a 4-byte big-endian FourCC from raw bytes.
quint32 fourCC(const QByteArray& b, int pos)
{
    if (pos + 4 > b.size()) return 0;
    return (static_cast<quint32>(static_cast<quint8>(b.at(pos))) << 24)
         | (static_cast<quint32>(static_cast<quint8>(b.at(pos + 1))) << 16)
         | (static_cast<quint32>(static_cast<quint8>(b.at(pos + 2))) << 8)
         | static_cast<quint32>(static_cast<quint8>(b.at(pos + 3)));
}

constexpr quint32 CC_RIFF = 0x52494646; // 'RIFF'
constexpr quint32 CC_WAVE = 0x57415645; // 'WAVE'
constexpr quint32 CC_XWMA = 0x58574D41; // 'XWMA'
constexpr quint32 CC_fmt  = 0x666D7420; // 'fmt '
constexpr quint32 CC_data = 0x64617461; // 'data'

// Little-endian u16/u32 helpers on raw bytes (fmt chunk payload).
quint16 le16(const QByteArray& b, int pos)
{
    if (pos + 2 > b.size()) return 0;
    return static_cast<quint16>(static_cast<quint8>(b.at(pos)))
         | (static_cast<quint16>(static_cast<quint8>(b.at(pos + 1))) << 8);
}

quint32 le32(const QByteArray& b, int pos)
{
    if (pos + 4 > b.size()) return 0;
    return static_cast<quint32>(static_cast<quint8>(b.at(pos)))
         | (static_cast<quint32>(static_cast<quint8>(b.at(pos + 1))) << 8)
         | (static_cast<quint32>(static_cast<quint8>(b.at(pos + 2))) << 16)
         | (static_cast<quint32>(static_cast<quint8>(b.at(pos + 3))) << 24);
}

} // namespace

namespace {

// Full-length decode via the ffmpeg CLI (present on PATH). ffmpeg understands
// Bethesda's xWMA framing natively and produces complete 16-bit PCM.
XwmaDecoder::Result decodeViaFfmpeg(const QByteArray& xwmaBytes,
                                    quint32 sampleRate, quint16 channels)
{
    XwmaDecoder::Result result;
    const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (ffmpeg.isEmpty())
        return result;

    QTemporaryFile in;
    if (!in.open()) return result;
    in.write(xwmaBytes);
    in.close();

    QProcess p;
    p.setProcessChannelMode(QProcess::SeparateChannels);
    p.start(ffmpeg, {
        QStringLiteral("-v"), QStringLiteral("error"),
        QStringLiteral("-i"), in.fileName(),
        QStringLiteral("-f"), QStringLiteral("s16le"),
        QStringLiteral("-acodec"), QStringLiteral("pcm_s16le"),
        QStringLiteral("-")
    });
    if (!p.waitForStarted(5000))
    {
        LOG_WARNING("XwmaDecoder: ffmpeg did not start");
        return result;
    }

    QByteArray out;
    while (p.state() != QProcess::NotRunning || p.bytesAvailable() > 0)
    {
        p.waitForReadyRead(200);
        out += p.readAllStandardOutput();
    }
    p.waitForFinished();
    out += p.readAllStandardOutput();

    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0)
    {
        LOG_WARNING(QString("XwmaDecoder: ffmpeg exit code %1").arg(p.exitCode()));
        return result;
    }
    if (out.isEmpty())
        return result;

    result.ok = true;
    result.pcm = out;
    result.sampleRate = sampleRate;
    result.channels = channels;
    result.bitsPerSample = 16;
    LOG_INFO(QString("XwmaDecoder: ffmpeg decoded %1 bytes of %2 Hz %3ch PCM")
        .arg(out.size()).arg(sampleRate).arg(channels));
    return result;
}

} // namespace

XwmaDecoder::Result XwmaDecoder::decode(const QByteArray& xwmaBytes)
{
    Result result;

    // Parse the RIFF container.
    if (xwmaBytes.size() < 12 || fourCC(xwmaBytes, 0) != CC_RIFF)
        return result;
    if (fourCC(xwmaBytes, 8) != CC_WAVE && fourCC(xwmaBytes, 8) != CC_XWMA)
        return result;

    QByteArray fmtData;
    QByteArray dataChunk;
    int pos = 12;
    while (pos + 8 <= xwmaBytes.size())
    {
        const quint32 tag = fourCC(xwmaBytes, pos);
        const quint32 size = le32(xwmaBytes, pos + 4);
        if (pos + 8 + static_cast<int>(size) > xwmaBytes.size())
            break;
        if (tag == CC_fmt)
            fmtData = xwmaBytes.mid(pos + 8, static_cast<int>(size));
        else if (tag == CC_data)
            dataChunk = xwmaBytes.mid(pos + 8, static_cast<int>(size));
        pos += 8 + static_cast<int>(size);
    }

    if (fmtData.size() < 16 || dataChunk.isEmpty()) {
        LOG_ERROR("XwmaDecoder: missing fmt or data chunk");
        return result;
    }

    const quint16 formatTag = le16(fmtData, 0);
    const quint16 channels = le16(fmtData, 2);
    const quint32 sampleRate = le32(fmtData, 4);

    // Only WMA standard (0x0161) / xWMA variants are expected.
    if (formatTag != 0x0161 && formatTag != 0x0162) {
        LOG_ERROR(QString("XwmaDecoder: unsupported format tag 0x%1").arg(formatTag, 4, 16, QChar('0')));
        return result;
    }

    // ffmpeg produces a complete, correct decode when it is on PATH.
    const Result ffmpegResult = decodeViaFfmpeg(xwmaBytes, sampleRate, channels);
    if (ffmpegResult.ok)
        return ffmpegResult;

    // ------------------------------------------------------------------
    // Fallback: Windows Media Foundation WMA decoder MFT (debug-only
    // diagnostics). Produces valid PCM but on this machine only the opening
    // ~0.1s of a Bethesda voice (the decoder stops early / silences the
    // rest). Kept as a self-contained path that works without external
    // tools. See docs/TECHNICAL_DEBT.md L3.
    // ------------------------------------------------------------------

    static bool initialized = false;
    static bool initOk = false;
    if (!initialized) {
        initialized = true;
        initOk = (MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET) == S_OK);
    }
    if (!initOk) {
        LOG_ERROR("XwmaDecoder: Media Foundation failed to start");
        return result;
    }

    ComPtr<IMFTransform> decoder;
    HRESULT hr = CoCreateInstance(CLSID_CWMADecMediaObject, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&decoder));
    if (FAILED(hr)) {
        logHr(hr, "CoCreateInstance(CLSID_CWMADecMediaObject)");
        return result;
    }

    ComPtr<IMFMediaType> inputType;
    hr = MFCreateMediaType(&inputType);
    if (SUCCEEDED(hr)) hr = inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(hr)) hr = inputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8);
    if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, channels);
    if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate);
    if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
    if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, le16(fmtData, 12));
    if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, le32(fmtData, 8));
    // The WMA decoder needs codec private data. xWMA has none; per the ffmpeg
    // xwma demuxer a synthesized 6-byte blob (all zero except byte 4 = 0x1F)
    // works, exposed as WAVEFORMATEXWMA (fmt WAVEFORMATEX + appended blob).
    if (SUCCEEDED(hr))
    {
        const QByteArray codecBlob("\x00\x00\x00\x00\x1f\x00", 6);
        QByteArray userData = fmtData;
        if (userData.size() >= 18)
        {
            userData[16] = static_cast<char>(codecBlob.size() & 0xFF);
            userData[17] = static_cast<char>((codecBlob.size() >> 8) & 0xFF);
        }
        userData.append(codecBlob);
        hr = inputType->SetBlob(MF_MT_USER_DATA, reinterpret_cast<const UINT8*>(userData.constData()),
                                static_cast<UINT32>(userData.size()));
    }
    if (SUCCEEDED(hr)) hr = decoder->SetInputType(0, inputType.Get(), 0);
    if (FAILED(hr)) {
        logHr(hr, "SetInputType(WMA)");
        return result;
    }

    ComPtr<IMFMediaType> outputType;
    hr = decoder->GetOutputAvailableType(0, 0, &outputType);
    if (FAILED(hr)) {
        hr = MFCreateMediaType(&outputType);
        if (SUCCEEDED(hr)) hr = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        if (SUCCEEDED(hr)) hr = outputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        if (SUCCEEDED(hr)) hr = outputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        if (SUCCEEDED(hr)) hr = outputType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, channels);
        if (SUCCEEDED(hr)) hr = outputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate);
    }
    if (SUCCEEDED(hr)) hr = decoder->SetOutputType(0, outputType.Get(), 0);
    if (FAILED(hr)) {
        logHr(hr, "SetOutputType(PCM)");
        return result;
    }

    UINT32 outRate = sampleRate, outChannels = channels, outBits = 16;
    outputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &outRate);
    outputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &outChannels);
    outputType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &outBits);
    if (outRate == 0) outRate = sampleRate;
    if (outChannels == 0) outChannels = channels;

    MFT_OUTPUT_STREAM_INFO outInfo = {};
    decoder->GetOutputStreamInfo(0, &outInfo);

    QByteArray pcm;
    const quint32 blockAlign = le16(fmtData, 12);

    auto pullOutput = [&]() -> bool {
        int iterations = 0;
        while (iterations < 200)
        {
            ++iterations;
            ComPtr<IMFSample> outSample;
            hr = MFCreateSample(&outSample);
            if (FAILED(hr)) return false;
            ComPtr<IMFMediaBuffer> outBuffer;
            if (FAILED(MFCreateMemoryBuffer(outInfo.cbSize > 0 ? outInfo.cbSize : 8192, &outBuffer)))
                return false;
            outSample->AddBuffer(outBuffer.Get());

            MFT_OUTPUT_DATA_BUFFER outData = {};
            outData.dwStreamID = 0;
            outData.pSample = outSample.Get();
            DWORD status = 0;
            hr = decoder->ProcessOutput(0, 1, &outData, &status);
            if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
                return true;
            if (FAILED(hr)) {
                logHr(hr, "ProcessOutput");
                return false;
            }

            ComPtr<IMFMediaBuffer> gotBuffer;
            if (SUCCEEDED(outSample->ConvertToContiguousBuffer(&gotBuffer))) {
                BYTE* p = nullptr;
                DWORD len = 0;
                if (SUCCEEDED(gotBuffer->Lock(&p, nullptr, &len)))
                    pcm.append(reinterpret_cast<const char*>(p), static_cast<int>(len));
            }
        }
        return true;
    };

    decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);

    {
        const int packetSize = blockAlign > 0 ? static_cast<int>(blockAlign) : 1487;
        for (int offset = 0; offset < dataChunk.size(); offset += packetSize)
        {
            const int chunk = qMin(packetSize, dataChunk.size() - offset);
            ComPtr<IMFSample> sample;
            ComPtr<IMFMediaBuffer> buffer;
            hr = MFCreateSample(&sample);
            if (SUCCEEDED(hr)) hr = MFCreateMemoryBuffer(static_cast<DWORD>(chunk), &buffer);
            if (SUCCEEDED(hr)) hr = sample->AddBuffer(buffer.Get());
            if (SUCCEEDED(hr)) {
                BYTE* p = nullptr;
                hr = buffer->Lock(&p, nullptr, nullptr);
                if (SUCCEEDED(hr)) {
                    memcpy(p, dataChunk.constData() + offset, static_cast<size_t>(chunk));
                    buffer->Unlock();
                }
                if (SUCCEEDED(hr)) hr = buffer->SetCurrentLength(static_cast<DWORD>(chunk));
            }
            if (SUCCEEDED(hr)) hr = sample->SetSampleTime(0);
            if (FAILED(hr)) {
                logHr(hr, "build packet sample");
                return result;
            }
            hr = decoder->ProcessInput(0, sample.Get(), 0);
            if (hr == MF_E_NOTACCEPTING)
            {
                pullOutput();
                --offset;
                continue;
            }
            if (FAILED(hr)) {
                logHr(hr, "ProcessInput(packet)");
                return result;
            }
            pullOutput();
        }
    }

    pullOutput();
    decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
    decoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
    pullOutput();

    if (pcm.isEmpty()) {
        LOG_ERROR("XwmaDecoder: decoder produced no PCM");
        return result;
    }

    result.ok = true;
    result.pcm = pcm;
    result.sampleRate = outRate;
    result.channels = static_cast<quint16>(outChannels);
    result.bitsPerSample = static_cast<quint16>(outBits);
    LOG_INFO(QString("XwmaDecoder: MFT decoded %1 bytes of %2 Hz %3ch PCM")
        .arg(pcm.size()).arg(outRate).arg(outChannels));
    return result;
}

#include "xwmadecoder.hpp"

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mftransform.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <wrl/client.h>
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
constexpr quint32 CC_dpds = 0x64706473; // 'dpds'

} // namespace

XwmaDecoder::Result XwmaDecoder::decode(const QByteArray& xwmaBytes)
{
    XwmaDecoder::Result result;

    // Parse the RIFF container.
    if (xwmaBytes.size() < 12 || fourCC(xwmaBytes, 0) != CC_RIFF)
        return result;
    if (fourCC(xwmaBytes, 8) != CC_WAVE && fourCC(xwmaBytes, 8) != CC_XWMA)
        return result;

    QByteArray fmtData;
    QByteArray dataChunk;
    QByteArray dpdsData;
    int pos = 12;
    while (pos + 8 <= xwmaBytes.size())
    {
        const quint32 tag = fourCC(xwmaBytes, pos);
        const quint32 size = static_cast<quint32>(
            static_cast<quint8>(xwmaBytes.at(pos + 4))
            | (static_cast<quint8>(xwmaBytes.at(pos + 5)) << 8)
            | (static_cast<quint8>(xwmaBytes.at(pos + 6)) << 16)
            | (static_cast<quint8>(xwmaBytes.at(pos + 7)) << 24));
        if (pos + 8 + static_cast<int>(size) > xwmaBytes.size())
            break;
        if (tag == CC_fmt)
            fmtData = xwmaBytes.mid(pos + 8, static_cast<int>(size));
        else if (tag == CC_data)
            dataChunk = xwmaBytes.mid(pos + 8, static_cast<int>(size));
        else if (tag == CC_dpds)
            dpdsData = xwmaBytes.mid(pos + 8, static_cast<int>(size));
        pos += 8 + static_cast<int>(size);
    }

    if (fmtData.size() < 16 || dataChunk.isEmpty()) {
        LOG_ERROR("XwmaDecoder: missing fmt or data chunk");
        return result;
    }

    const quint16 formatTag = static_cast<quint16>(
        static_cast<quint8>(fmtData.at(0)) | (static_cast<quint8>(fmtData.at(1)) << 8));
    const quint16 channels = static_cast<quint16>(
        static_cast<quint8>(fmtData.at(2)) | (static_cast<quint8>(fmtData.at(3)) << 8));
    const quint32 sampleRate = static_cast<quint32>(
        static_cast<quint8>(fmtData.at(4))
        | (static_cast<quint8>(fmtData.at(5)) << 8)
        | (static_cast<quint8>(fmtData.at(6)) << 16)
        | (static_cast<quint8>(fmtData.at(7)) << 24));

    // Only WMA standard (0x0161) / xWMA variants are expected.
    if (formatTag != 0x0161 && formatTag != 0x0162) {
        LOG_ERROR(QString("XwmaDecoder: unsupported format tag 0x%1").arg(formatTag, 4, 16, QChar('0')));
        return result;
    }

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

    // Instantiate the WMA decoder MFT.
    ComPtr<IMFTransform> decoder;
    HRESULT hr = CoCreateInstance(CLSID_CWMADecMediaObject, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&decoder));
    if (FAILED(hr)) {
        logHr(hr, "CoCreateInstance(CLSID_CWMADecMediaObject)");
        return result;
    }

    // Input type: WMA from the fmt chunk. The WMA decoder also needs the
    // codec private data (from the dpds chunk) as MF_MT_USER_DATA and the
    // bits-per-sample attribute.
    ComPtr<IMFMediaType> inputType;
    hr = MFCreateMediaType(&inputType);
    if (SUCCEEDED(hr)) hr = inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(hr)) hr = inputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8);
    if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, channels);
    if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate);
    if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
    if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,
        static_cast<quint16>(fmtData.at(12)) | (static_cast<quint16>(fmtData.at(13)) << 8));
    if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,
        static_cast<quint32>(
            static_cast<quint8>(fmtData.at(8))
            | (static_cast<quint8>(fmtData.at(9)) << 8)
            | (static_cast<quint8>(fmtData.at(10)) << 16)
            | (static_cast<quint8>(fmtData.at(11)) << 24)));
    // The WMA decoder requires the codec private data. Build the
    // WAVEFORMATEXWMA structure: the fmt WAVEFORMATEX (with cbSize pointing at
    // the appended codec blob) followed by the xWMA codec data from dpds.
    if (SUCCEEDED(hr) && !dpdsData.isEmpty())
    {
        QByteArray userData = fmtData;
        if (userData.size() >= 18)
        {
            // Set cbSize (bytes 16-17) to the codec blob size.
            userData[16] = static_cast<char>(dpdsData.size() & 0xFF);
            userData[17] = static_cast<char>((dpdsData.size() >> 8) & 0xFF);
        }
        userData.append(dpdsData);
        hr = inputType->SetBlob(MF_MT_USER_DATA, reinterpret_cast<const UINT8*>(userData.constData()),
                                static_cast<UINT32>(userData.size()));
    }
    if (SUCCEEDED(hr)) hr = decoder->SetInputType(0, inputType.Get(), 0);
    if (FAILED(hr)) {
        logHr(hr, "SetInputType(WMA)");
        return result;
    }

    // Output type: negotiate the decoder's native PCM output rather than
    // forcing attributes the decoder may reject.
    ComPtr<IMFMediaType> outputType;
    hr = decoder->GetOutputAvailableType(0, 0, &outputType);
    if (FAILED(hr)) {
        // Fall back to a hand-built PCM type.
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

    // Read back the negotiated sample rate / channels.
    UINT32 outRate = sampleRate, outChannels = channels, outBits = 16;
    outputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &outRate);
    outputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &outChannels);
    outputType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &outBits);
    if (outRate == 0) outRate = sampleRate;
    if (outChannels == 0) outChannels = channels;

    MFT_OUTPUT_STREAM_INFO outInfo = {};
    decoder->GetOutputStreamInfo(0, &outInfo);

    QByteArray pcm;
    const quint32 blockAlign = static_cast<quint16>(fmtData.at(12)) | (static_cast<quint16>(fmtData.at(13)) << 8);
    const int packetSize = blockAlign > 0 ? static_cast<int>(blockAlign) : 20480;
    // Feed the compressed data as per-frame packets (WMA decoders buffer one
    // frame at a time). Each packet carries a 100ns frame duration.
    const LONGLONG frameDuration = static_cast<LONGLONG>(
        10000000.0 * packetSize / static_cast<double>(outRate > 0 ? outRate : sampleRate));
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
        if (SUCCEEDED(hr)) hr = sample->SetSampleTime(offset * frameDuration / packetSize);
        if (SUCCEEDED(hr)) hr = sample->SetSampleDuration(frameDuration);
        if (FAILED(hr)) {
            logHr(hr, "build packet sample");
            return result;
        }
        hr = decoder->ProcessInput(0, sample.Get(), 0);
        if (FAILED(hr)) {
            logHr(hr, "ProcessInput(packet)");
            return result;
        }
    }

    // Drain all output samples (WMA decoders buffer; drain flushes them).
    decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
    decoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);

    int iterations = 0;
    while (iterations < 200)
    {
        ++iterations;
        ComPtr<IMFSample> outSample;
        hr = MFCreateSample(&outSample);
        if (FAILED(hr)) break;
        ComPtr<IMFMediaBuffer> outBuffer;
        if (FAILED(MFCreateMemoryBuffer(outInfo.cbSize > 0 ? outInfo.cbSize : 8192, &outBuffer)))
            break;
        outSample->AddBuffer(outBuffer.Get());

        MFT_OUTPUT_DATA_BUFFER outData = {};
        outData.dwStreamID = 0;
        outData.pSample = outSample.Get();
        DWORD status = 0;
        hr = decoder->ProcessOutput(0, 1, &outData, &status);
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
            break;
        if (FAILED(hr)) {
            logHr(hr, "ProcessOutput");
            break;
        }

        ComPtr<IMFMediaBuffer> gotBuffer;
        if (SUCCEEDED(outSample->ConvertToContiguousBuffer(&gotBuffer))) {
            BYTE* p = nullptr;
            DWORD len = 0;
            if (SUCCEEDED(gotBuffer->Lock(&p, nullptr, &len)))
                pcm.append(reinterpret_cast<const char*>(p), static_cast<int>(len));
        }
    }

    if (pcm.isEmpty()) {
        LOG_ERROR("XwmaDecoder: decoder produced no PCM");
        return result;
    }

    result.ok = true;
    result.pcm = pcm;
    result.sampleRate = outRate;
    result.channels = static_cast<quint16>(outChannels);
    result.bitsPerSample = static_cast<quint16>(outBits);
    LOG_INFO(QString("XwmaDecoder: decoded %1 bytes of %2 Hz %3ch PCM")
        .arg(pcm.size()).arg(outRate).arg(outChannels));
    return result;
}

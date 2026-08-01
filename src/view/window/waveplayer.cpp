#include "waveplayer.hpp"

#include "../../libs/files/log/logger.hpp"

#include <QtGlobal>

#ifdef _WIN32
#pragma comment(lib, "winmm.lib")
#endif

namespace {
constexpr int BufferSampleCount = 8192;
}

WavePlayer::WavePlayer(QObject* parent)
    : QObject(parent)
    , mSampleRate(44100)
    , mTotalSamples(0)
    , mPlaying(false)
    , mHandle(nullptr)
    , mNextBuffer(0)
    , mPlayedSamples(0)
    , mSubmittedBuffers(0)
{
}

WavePlayer::~WavePlayer()
{
    stop();
    freeBuffers();
}

bool WavePlayer::load(const QVector<float>& samples, int sampleRate)
{
    stop();
    freeBuffers();

    if (sampleRate <= 0) {
        return false;
    }

    mSampleRate = sampleRate;
    mTotalSamples = samples.size();
    mPcm.resize(mTotalSamples);

    for (int i = 0; i < mTotalSamples; ++i) {
        const float s = qBound(-1.0f, samples[i], 1.0f);
        mPcm[i] = static_cast<short>(s * 32767.0f);
    }

    // Split into fixed-size buffers for the queued streaming.
    const int bufferCount = (mTotalSamples + BufferSampleCount - 1) / BufferSampleCount;
    mBufferData.resize(bufferCount);
    mHeaders.resize(bufferCount);
    for (int b = 0; b < bufferCount; ++b) {
        const int start = b * BufferSampleCount;
        const int count = qMin(BufferSampleCount, mTotalSamples - start);
        mBufferData[b].resize(count * sizeof(short));
        memcpy(mBufferData[b].data(), mPcm.constData() + start, mBufferData[b].size());

        WAVEHDR& hdr = mHeaders[b];
        memset(&hdr, 0, sizeof(hdr));
        hdr.lpData = mBufferData[b].data();
        hdr.dwBufferLength = static_cast<DWORD>(mBufferData[b].size());
        hdr.dwUser = static_cast<DWORD_PTR>(count);
    }

    openDevice();
    return true;
}

void WavePlayer::clear()
{
    stop();
    freeBuffers();
    mPcm.clear();
    mTotalSamples = 0;
    mPlayedSamples = 0;
}

void WavePlayer::openDevice()
{
    if (mHandle) {
        return;
    }
    WAVEFORMATEX fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 1;
    fmt.nSamplesPerSec = static_cast<DWORD>(mSampleRate);
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = (fmt.wBitsPerSample / 8) * fmt.nChannels;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    MMRESULT res = waveOutOpen(&mHandle, WAVE_MAPPER, &fmt,
        reinterpret_cast<DWORD_PTR>(&WavePlayer::waveOutProc),
        reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR) {
        LOG_WARNING(QString("WavePlayer: waveOutOpen failed (0x%1)").arg(res, 0, 16));
        mHandle = nullptr;
    }
}

void WavePlayer::closeDevice()
{
    if (!mHandle) {
        return;
    }
    waveOutReset(mHandle);
    waveOutClose(mHandle);
    mHandle = nullptr;
}

void WavePlayer::freeBuffers()
{
    if (mHandle) {
        for (WAVEHDR& hdr : mHeaders) {
            if (hdr.dwFlags & WHDR_PREPARED) {
                waveOutUnprepareHeader(mHandle, &hdr, sizeof(hdr));
            }
        }
    }
    mBufferData.clear();
    mHeaders.clear();
    mNextBuffer = 0;
    mSubmittedBuffers = 0;
}

bool WavePlayer::play(int startSample)
{
    if (mPcm.isEmpty() || !mHandle) {
        return false;
    }

    stop();
    openDevice();
    if (!mHandle) {
        return false;
    }

    mPlayedSamples = 0;
    mNextBuffer = 0;
    mSubmittedBuffers = 0;

    // Skip buffers wholly before the start sample; begin mid-buffer by
    // rewinding the first submitted buffer's data pointer.
    const int start = qBound(0, startSample, mTotalSamples);
    int firstBuffer = start / BufferSampleCount;

    if (firstBuffer > 0) {
        mNextBuffer = firstBuffer;
        mPlayedSamples = firstBuffer * BufferSampleCount;
    }

    const int offsetInBuffer = start - (firstBuffer * BufferSampleCount);
    if (offsetInBuffer > 0 && firstBuffer < mHeaders.size()) {
        WAVEHDR& hdr = mHeaders[firstBuffer];
        hdr.lpData = mBufferData[firstBuffer].data() + offsetInBuffer * sizeof(short);
        hdr.dwBufferLength = static_cast<DWORD>(
            (static_cast<int>(hdr.dwUser) - offsetInBuffer) * sizeof(short));
    }

    mPlaying = true;
    while (mNextBuffer < mHeaders.size()) {
        WAVEHDR& hdr = mHeaders[mNextBuffer];
        MMRESULT res = waveOutPrepareHeader(mHandle, &hdr, sizeof(hdr));
        if (res != MMSYSERR_NOERROR) {
            LOG_WARNING(QString("WavePlayer: prepare header failed (0x%1)").arg(res, 0, 16));
            mPlaying = false;
            return false;
        }
        waveOutWrite(mHandle, &hdr, sizeof(hdr));
        ++mSubmittedBuffers;
        ++mNextBuffer;
    }
    return true;
}

void WavePlayer::pause()
{
    if (mPlaying && mHandle) {
        waveOutPause(mHandle);
    }
}

void WavePlayer::resume()
{
    if (mPlaying && mHandle) {
        waveOutRestart(mHandle);
    }
}

void WavePlayer::stop()
{
    if (mPlaying) {
        mPlaying = false;
        if (mHandle) {
            waveOutReset(mHandle);
            for (WAVEHDR& hdr : mHeaders) {
                if (hdr.dwFlags & WHDR_PREPARED) {
                    waveOutUnprepareHeader(mHandle, &hdr, sizeof(hdr));
                }
            }
        }
        emit playbackStopped();
    }
}

void WavePlayer::handleBufferDone(WAVEHDR* header)
{
    const int sampleCount = static_cast<int>(header->dwUser);
    mPlayedSamples += sampleCount;
    emit positionChanged(mPlayedSamples);

    if (--mSubmittedBuffers <= 0 && mPlaying) {
        mPlaying = false;
        emit playbackStopped();
    }
}

void CALLBACK WavePlayer::waveOutProc(HWAVEOUT, UINT msg, DWORD_PTR instance,
                                      DWORD_PTR param1, DWORD_PTR)
{
    if (msg != WOM_DONE) {
        return;
    }
    auto* player = reinterpret_cast<WavePlayer*>(instance);
    if (!player) {
        return;
    }
    auto* header = reinterpret_cast<WAVEHDR*>(param1);
    player->handleBufferDone(header);
}

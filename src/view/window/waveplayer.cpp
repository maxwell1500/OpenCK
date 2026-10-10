#include "waveplayer.hpp"

#include "../../libs/files/log/logger.hpp"

#include <QtGlobal>
#include <QtEndian>
#include <QTimer>

#ifdef _WIN32
#  pragma comment(lib, "winmm.lib")
#endif

namespace {
constexpr int ChunkSampleCount = 8192;
}

WavePlayer::WavePlayer(QObject* parent)
    : QObject(parent)
    , mSampleRate(44100)
    , mTotalSamples(0)
    , mPlaying(false)
    , mPlayedSamples(0)
{
}

WavePlayer::~WavePlayer()
{
    stop();
    reset();
}

bool WavePlayer::isAvailable() const
{
#if defined(OPENCK_WITH_QT_MULTIMEDIA)
    return true;
#else
    return false;
#endif
}

void WavePlayer::reset()
{
    closeSink();
    mPcm.clear();
    mSampleRate = 44100;
    mTotalSamples = 0;
    mPlayedSamples = 0;
    mPlaying = false;
}

bool WavePlayer::load(const QVector<float>& samples, int sampleRate)
{
    stop();
    reset();

    if (sampleRate <= 0 || samples.isEmpty()) {
        return false;
    }

    mSampleRate = sampleRate;
    mTotalSamples = samples.size();
    mPcm.resize(mTotalSamples);

    for (int i = 0; i < mTotalSamples; ++i) {
        const float s = qBound(-1.0f, samples[i], 1.0f);
        mPcm[i] = static_cast<short>(s * 32767.0f);
    }
    return true;
}

void WavePlayer::clear()
{
    stop();
    reset();
}

bool WavePlayer::play(int startSample)
{
    if (mPcm.isEmpty()) {
        return false;
    }
    stop();

    const int start = qBound(0, startSample, mTotalSamples);
    mPlayedSamples = start;

#if defined(OPENCK_WITH_QT_MULTIMEDIA)
    if (!mSink && !openSink()) {
        return false;
    }
    if (mSink->state() == QAudio::SuspendedState) {
        mSink->resume();
    } else if (mSink->state() != QAudio::ActiveState) {
        // Rewind the device before the first write of this session.
        mSinkDevice->reset();
    }
    mWrittenSamples = start;
    mEof = false;
    mPlaying = true;
    writeSinkData(start);
    return true;
#else
    // No Qt Multimedia: report an honest failure so the preview dialog can
    // say "audio unavailable" instead of silently doing nothing.
    mPlaying = false;
    LOG_WARNING(QStringLiteral("WavePlayer: Qt Multimedia not available; audio preview disabled"));
    return false;
#endif
}

void WavePlayer::pause()
{
    if (!mPlaying) {
        return;
    }
#if defined(OPENCK_WITH_QT_MULTIMEDIA)
    if (mSink) {
        mSink->suspend();
    }
#endif
    mPlaying = false;
}

void WavePlayer::resume()
{
    if (mPlaying || mPcm.isEmpty()) {
        return;
    }
#if defined(OPENCK_WITH_QT_MULTIMEDIA)
    if (!mSink && !openSink()) {
        return;
    }
    if (mSink->state() == QAudio::SuspendedState) {
        mSink->resume();
        mPlaying = true;
    }
#endif
}

void WavePlayer::stop()
{
#if defined(OPENCK_WITH_QT_MULTIMEDIA)
    if (mSink && mSink->state() != QAudio::StoppedState) {
        mSink->stop();
    }
#endif
    mPlaying = false;
}

#if defined(OPENCK_WITH_QT_MULTIMEDIA)

bool WavePlayer::openSink()
{
    if (mSink) {
        return true;
    }
    mFormat.setSampleRate(mSampleRate);
    mFormat.setChannelCount(1);
    mFormat.setSampleFormat(QAudioFormat::Int16);

    mSink = new QAudioSink(mFormat, this);
    mSinkDevice = mSink->start();
    if (!mSinkDevice) {
        LOG_WARNING(QStringLiteral("WavePlayer: no audio device available"));
        delete mSink;
        mSink = nullptr;
        return false;
    }
    connect(mSink, &QAudioSink::stateChanged, this, [this](QAudio::State state) {
        if (state == QAudio::IdleState && mPlaying) {
            mPlaying = false;
            emit playbackStopped();
        }
    });
    // QAudioSink drains asynchronously: bytesFree() only refills if something
    // writes again, so feed it from a timer rather than once.
    QTimer* feeder = new QTimer(this);
    feeder->setInterval(25);
    connect(feeder, &QTimer::timeout, this, [this] {
        if (mPlaying) {
            writeSinkData(mWrittenSamples);
        }
    });
    feeder->start();
    return true;
}

void WavePlayer::closeSink()
{
    if (mSink) {
        mSink->stop();
        mSink->deleteLater();
        mSink = nullptr;
        mSinkDevice = nullptr;
    }
}

// Write in fixed-size chunks and keep at least two chunks queued. QAudioSink
// drops out to IdleState once the buffer drains, which is the natural
// end-of-playback signal; positionChanged is emitted from the bytes written.
void WavePlayer::writeSinkData(qint64 fromSample)
{
    if (!mSink || !mSinkDevice) {
        return;
    }
    while (!mEof && mWrittenSamples < mTotalSamples) {
        const qint64 bytesFree = mSink->bytesFree();
        if (bytesFree < ChunkSampleCount * static_cast<qint64>(sizeof(short))) {
            break;
        }
        const int chunk = static_cast<int>(
            qMin<qint64>(ChunkSampleCount, mTotalSamples - mWrittenSamples));
        const int byteCount = chunk * static_cast<int>(sizeof(short));
        const char* data = reinterpret_cast<const char*>(mPcm.constData() + mWrittenSamples);
        if (mSinkDevice->write(data, byteCount) != byteCount) {
            break;
        }
        mWrittenSamples += chunk;
        mPlayedSamples = mWrittenSamples;
        emit positionChanged(mPlayedSamples);
        if (mWrittenSamples >= mTotalSamples) {
            mEof = true;
        }
    }
}

#else

bool WavePlayer::openSink()
{
    return false;
}

void WavePlayer::closeSink()
{
}

#endif

#ifndef WAVEPLAYER_HPP
#define WAVEPLAYER_HPP

#include <QObject>
#include <QVector>

// WavePlayer streams float samples to the audio device.
//
// Windows uses the Win32 waveOut API; everywhere else it uses Qt's
// QAudioSink, which is the portable answer across ALSA/PipeWire/PulseAudio
// (and on macOS/Linux-equivalent backends generally) so the same preview
// dialog works natively on Linux and Steam Deck.
//
// When Qt Multimedia is not built at all, playback is simply unavailable and
// isPlaying() reports false; the preview dialog keeps every other function.

#if defined(OPENCK_WITH_QT_MULTIMEDIA)
#  include <QAudioSink>
#  include <QAudioFormat>
#  include <QIODevice>
#endif

class WavePlayer : public QObject
{
    Q_OBJECT

public:
    explicit WavePlayer(QObject* parent = nullptr);
    ~WavePlayer();

    bool load(const QVector<float>& samples, int sampleRate);
    void clear();

    bool play(int startSample = 0);
    void pause();
    void resume();
    void stop();

    bool isPlaying() const { return mPlaying; }
    int totalSamples() const { return mTotalSamples; }
    int sampleRate() const { return mSampleRate; }
    qint64 playedSamples() const { return mPlayedSamples; }
    /// True when an audio device can actually be opened on this platform.
    bool isAvailable() const;

signals:
    void playbackStopped();
    void positionChanged(qint64 samples);

private:
    void reset();
    bool openSink();
    void closeSink();
#if defined(OPENCK_WITH_QT_MULTIMEDIA)
    void writeSinkData(qint64 fromSample);
#endif

    QVector<short> mPcm;
    int mSampleRate = 0;
    int mTotalSamples = 0;
    bool mPlaying = false;
    qint64 mPlayedSamples = 0;

#if defined(OPENCK_WITH_QT_MULTIMEDIA)
    QAudioFormat mFormat;
    QAudioSink* mSink = nullptr;
    QIODevice* mSinkDevice = nullptr;
    qint64 mWrittenSamples = 0;
    bool mEof = false;
#endif
};

#endif // WAVEPLAYER_HPP

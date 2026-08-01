#ifndef WAVEPLAYER_HPP
#define WAVEPLAYER_HPP

#include <QObject>
#include <QVector>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

// WavePlayer streams a set of float samples to the audio device using the
// Win32 waveOut API. The samples are converted to 16-bit PCM and split into
// fixed-size buffers that are queued ahead of time; playback position is
// tracked from the completed-buffer callback.
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

signals:
    void playbackStopped();
    void positionChanged(qint64 samples);

private:
    void openDevice();
    void closeDevice();
    void freeBuffers();
    void handleBufferDone(WAVEHDR* header);
    static void CALLBACK waveOutProc(HWAVEOUT handle, UINT msg, DWORD_PTR instance,
                                     DWORD_PTR param1, DWORD_PTR param2);

    QVector<short> mPcm;
    int mSampleRate;
    int mTotalSamples;
    bool mPlaying;

    HWAVEOUT mHandle;
    QVector<WAVEHDR> mHeaders;
    QVector<QByteArray> mBufferData;
    int mNextBuffer;
    qint64 mPlayedSamples;
    int mSubmittedBuffers;
};

#endif // WAVEPLAYER_HPP

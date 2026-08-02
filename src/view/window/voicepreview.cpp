#include "voicepreview.hpp"

#include "../libs/files/audio/xwmadecoder.hpp"
#include "logger.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QObject>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <mmsystem.h>
#  pragma comment(lib, "winmm.lib")
#endif

namespace VoicePreview {

namespace {

QString tempWavPath(const QString& tag)
{
    return QDir::tempPath() + QStringLiteral("/openck_voice_")
        + QString::number(QDateTime::currentMSecsSinceEpoch())
        + tag + QStringLiteral(".wav");
}

} // namespace

bool writePcmWav(const QByteArray& pcm, int sampleRate, int channels, const QString& outPath)
{
    if (pcm.isEmpty() || sampleRate <= 0 || channels <= 0) return false;

    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly)) return false;

    const quint32 dataSize = static_cast<quint32>(pcm.size());
    const quint32 byteRate = static_cast<quint32>(sampleRate) * static_cast<quint32>(channels) * 2u;
    const quint16 blockAlign = static_cast<quint16>(channels * 2);
    const quint16 bitsPerSample = 16;
    const quint16 audioFormat = 1;
    const quint32 fmtChunkSize = 16;
    const quint32 riffSize = 36 + dataSize;

    QByteArray hdr;
    hdr.append("RIFF");
    hdr.append(reinterpret_cast<const char*>(&riffSize), 4);
    hdr.append("WAVE");
    hdr.append("fmt ");
    hdr.append(reinterpret_cast<const char*>(&fmtChunkSize), 4);
    hdr.append(reinterpret_cast<const char*>(&audioFormat), 2);
    hdr.append(reinterpret_cast<const char*>(&channels), 2);
    hdr.append(reinterpret_cast<const char*>(&sampleRate), 4);
    hdr.append(reinterpret_cast<const char*>(&byteRate), 4);
    hdr.append(reinterpret_cast<const char*>(&blockAlign), 2);
    hdr.append(reinterpret_cast<const char*>(&bitsPerSample), 2);
    hdr.append("data");
    hdr.append(reinterpret_cast<const char*>(&dataSize), 4);

    const bool ok = f.write(hdr) == hdr.size() && f.write(pcm) == pcm.size();
    f.close();
    return ok;
}

bool playVoiceAudio(const QByteArray& audioData, const QString& fourCC, QWidget* parent)
{
    if (audioData.isEmpty()) return false;

    QString wavPath;
    if (audioData.startsWith("RIFF"))
    {
        wavPath = tempWavPath(QStringLiteral("_riff"));
        QFile wav(wavPath);
        if (!wav.open(QIODevice::WriteOnly)) return false;
        wav.write(audioData);
        wav.close();
    }
    else if (fourCC.compare(QStringLiteral("XWMA"), Qt::CaseInsensitive) == 0)
    {
        const XwmaDecoder::Result decoded = XwmaDecoder::decode(audioData);
        if (!decoded.ok || decoded.pcm.isEmpty())
        {
            LOG_WARNING("VoicePreview: xWMA decode produced no PCM");
            QMessageBox::warning(parent, QObject::tr("Play Voice File"),
                QObject::tr("This voice uses the xWMA codec and could not be decoded on this system."));
            return false;
        }
        wavPath = tempWavPath(QStringLiteral("_pcm"));
        if (!writePcmWav(decoded.pcm, static_cast<int>(decoded.sampleRate),
                         static_cast<int>(decoded.channels), wavPath))
            return false;
    }
    else
    {
        return false;
    }

#ifdef _WIN32
    std::wstring w = wavPath.toStdWString();
    return PlaySoundW(w.c_str(), nullptr, SND_FILENAME | SND_ASYNC) != FALSE;
#else
    QMessageBox::information(parent, QObject::tr("Play Voice File"),
        QObject::tr("Voice playback is not supported on this platform."));
    return false;
#endif
}

} // namespace VoicePreview

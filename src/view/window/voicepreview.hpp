#pragma once

#include <QByteArray>
#include <QString>

class QWidget;

// Turns .fuz voice audio into something the Win32 PlaySound API can play.
// The payload is either a plain RIFF WAV or an xWMA stream (decoded to PCM
// via the Media Foundation WMA decoder). Kept out of line so both the info
// editor and the archive browser can share one path.
namespace VoicePreview {
    // Writes 16-bit PCM samples to a WAV file.
    bool writePcmWav(const QByteArray& pcm, int sampleRate, int channels, const QString& outPath);
    // Writes a playable WAV to a temp file and starts async playback.
    // Returns false when the payload cannot be decoded to WAV.
    bool playVoiceAudio(const QByteArray& audioData, const QString& fourCC, QWidget* parent);
}

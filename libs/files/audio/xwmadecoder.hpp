#pragma once

#include <QString>
#include <QByteArray>

// Decodes xWMA audio (the codec used for Skyrim / Fallout voice .fuz files)
// into 16-bit PCM using the Windows Media Foundation SourceReader. xWMA files
// are RIFF containers with format tag 0x0161 (WMA standard); WMF provides the
// decoder. Returns PCM bytes (16-bit, interleaved for stereo) plus the output
// format details so the caller can size the playback buffer.
struct XwmaDecoder
{
    struct Result
    {
        bool ok = false;
        QByteArray pcm;
        quint32 sampleRate = 0;
        quint16 channels = 0;
        quint16 bitsPerSample = 16;
    };

    // Decodes an in-memory xWMA RIFF/WAVE stream to PCM.
    // Returns Result.ok == false on unsupported input or decoder failure.
    static Result decode(const QByteArray& xwmaBytes);
};

#ifndef FUZPARSER_HPP
#define FUZPARSER_HPP

#include <QString>
#include <QByteArray>

// Parses Skyrim / Fallout voice-over .fuz files. Real .fuz layout (per xEdit
// FUZer and verified against Skyrim SE archives):
//   "FUZE" magic, uint32 version, uint32 lipSize, raw lip data (lipSize bytes),
//   then the audio stream (a RIFF container: "RIFF" size "XWMA".../fmt chunk,
//   or a "WAVE" container for PCM). Some tools emit an older chunked form
//   ([4-byte FourCC][4-byte size][data]) which is also accepted for
//   compatibility.
// OpenCK extracts both lip and audio so voice preview can play audio and show
// lip cues.
struct FuzParser
{
    QByteArray lipData;
    QByteArray audioData;
    QString audioFourCC;   // e.g. "XWMA" or "WAVE"

    bool hasLip() const { return !lipData.isEmpty(); }
    bool hasAudio() const { return !audioData.isEmpty(); }

    // Parses .fuz bytes. Returns false if the magic is wrong or malformed.
    static bool parse(const QByteArray& bytes, FuzParser& out);

    // Loads and parses the given .fuz file. Returns false if unreadable.
    static bool loadFile(const QString& path, FuzParser& out);
};

#endif // FUZPARSER_HPP

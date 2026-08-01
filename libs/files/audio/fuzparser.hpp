#ifndef FUZPARSER_HPP
#define FUZPARSER_HPP

#include <QString>
#include <QByteArray>

// Parses Fallout 4 / Skyrim voice-over .fuz files. Format:
//   "FUZE" magic, then chunks of [4-byte FourCC][4-byte size LE][data].
// Known chunk types: "LIPF" (lip-sync data, plain text/JSON) and "XWAV"
// (embedded audio). OpenCK extracts both so voice preview can play the
// audio and show the lip cues.
struct FuzParser
{
    struct Chunk
    {
        QByteArray fourCC;
        QByteArray data;
    };

    QByteArray lipData;
    QByteArray audioData;
    QString audioFourCC;   // e.g. "XWAV" or "XWM "

    bool hasLip() const { return !lipData.isEmpty(); }
    bool hasAudio() const { return !audioData.isEmpty(); }

    // Parses .fuz bytes. Returns false if the magic is wrong or malformed.
    static bool parse(const QByteArray& bytes, FuzParser& out);

    // Loads and parses the given .fuz file. Returns false if unreadable.
    static bool loadFile(const QString& path, FuzParser& out);
};

#endif // FUZPARSER_HPP

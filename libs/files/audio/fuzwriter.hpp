#ifndef FUZWRITER_HPP
#define FUZWRITER_HPP

#include <QString>
#include <QByteArray>

// Writes Skyrim / Fallout voice-over .fuz containers in the forms the
// matching reader accepts (see FuzParser). With an audio stream: the real
// form —
//   "FUZE" magic, uint32 version, uint32 lipSize, raw lip data (lipSize
//   bytes), then the audio stream (a RIFF container: "RIFF" size "XWMA".../
//   fmt chunk, or a "WAVE" container for PCM).
// Without audio: the chunked form, "FUZE" + "LIPF" + uint32 size + lip,
// which is the layout lip-only archives use. Either way the container parses
// back to the same payloads, so a written file round-trips.
class FuzWriter
{
public:
    // Builds the container bytes. An empty lip section is legal (audio-only
    // voice); an empty audio section produces a lip-only container, which
    // FuzParser also accepts.
    static QByteArray build(const QByteArray& lipData,
                            const QByteArray& audioData,
                            quint32 version = 1);

    // Writes build() to disk; false when the target can't be opened.
    static bool writeFile(const QString& path, const QByteArray& lipData,
                          const QByteArray& audioData, quint32 version = 1);
};

#endif // FUZWRITER_HPP

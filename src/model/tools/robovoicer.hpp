#ifndef ROBOVOICER_HPP
#define ROBOVOICER_HPP

#include <QJsonObject>
#include <QString>
#include <QVector>

// RoboVoicer (REMAINING.md §3.8) — the batch dialogue-lipsync/TTS pipeline.
// The actual speech synthesis lives behind IVoiceSynthesizer so OpenCK stays
// engine-agnostic (a real build can plug in a TTS backend); this file defines
// the job model, its JSON round-trip, and the runner that drives a
// synthesizer over a plan. Tests use a fake synthesizer.
struct VoiceLine
{
    QString lineId;        // dialogue/info editor id
    QString speaker;       // actor name
    QString text;          // line to speak
    QString voiceId;       // requested voice
    QString outputPath;    // target .wav / .fuz
    bool done = false;
};

struct VoiceLinePlan
{
    QString name;
    QVector<VoiceLine> lines;

    int pendingCount() const;
    QJsonObject toJson() const;
    static VoiceLinePlan fromJson(const QJsonObject& obj);
};

class IVoiceSynthesizer
{
public:
    virtual ~IVoiceSynthesizer() = default;
    virtual QString name() const = 0;
    virtual bool isAvailable() const = 0;
    // Renders `text` in `voiceId` to `outputPath`. Returns success.
    virtual bool synthesize(const QString& text, const QString& voiceId,
                            const QString& outputPath) = 0;
};

struct VoiceRunReport
{
    int completed = 0;
    int failed = 0;
    QVector<QString> failures;   // line ids that failed
};

/// Synthesizes every not-yet-done line in the plan. Lines that succeed are
/// marked done. A null synthesizer marks every pending line failed.
VoiceRunReport runVoicePlan(VoiceLinePlan& plan, IVoiceSynthesizer* synth);

#ifdef _WIN32
// SapiVoiceSynthesizer (REMAINING.md §3.8) — the in-engine speech backend for
// Windows: renders lines through the built-in speech engine
// (System.Speech over SAPI: David/Zira/Haruka Desktop ship with Windows)
// straight to WAV files, which VoicePreview::playVoiceAudio can play back.
// It drives the engine in a short-lived powershell helper process so the
// build needs no SAPI SDK/ATL linkage beyond QtCore's QProcess. Each call
// costs roughly a second of interpreter startup; unavailable when no voice
// tokens are installed (e.g. Server Core) — callers fall back to the
// null-engine report path via isAvailable().
class SapiVoiceSynthesizer : public IVoiceSynthesizer
{
public:
    QString name() const override;
    bool isAvailable() const override;
    bool synthesize(const QString& text, const QString& voiceId,
                    const QString& outputPath) override;

    // Display names of the installed SAPI voices (empty when none).
    static QStringList availableVoices();
};
#endif

#endif // ROBOVOICER_HPP

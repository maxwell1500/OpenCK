#include "robovoicer.hpp"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>

#ifdef _WIN32
#include <QProcess>
#endif

int VoiceLinePlan::pendingCount() const
{
    int n = 0;
    for (const VoiceLine& l : lines)
        if (!l.done) ++n;
    return n;
}

QJsonObject VoiceLinePlan::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("name"), name);

    QJsonArray linesArr;
    for (const VoiceLine& l : lines)
    {
        QJsonObject lj;
        lj.insert(QStringLiteral("lineId"), l.lineId);
        lj.insert(QStringLiteral("speaker"), l.speaker);
        lj.insert(QStringLiteral("text"), l.text);
        lj.insert(QStringLiteral("voiceId"), l.voiceId);
        lj.insert(QStringLiteral("outputPath"), l.outputPath);
        lj.insert(QStringLiteral("done"), l.done);
        linesArr.append(lj);
    }
    obj.insert(QStringLiteral("lines"), linesArr);
    return obj;
}

VoiceLinePlan VoiceLinePlan::fromJson(const QJsonObject& obj)
{
    VoiceLinePlan plan;
    plan.name = obj.value(QStringLiteral("name")).toString();

    const QJsonArray arr = obj.value(QStringLiteral("lines")).toArray();
    for (const QJsonValue& v : arr)
    {
        if (!v.isObject())
            continue;
        const QJsonObject lj = v.toObject();
        VoiceLine l;
        l.lineId = lj.value(QStringLiteral("lineId")).toString();
        l.speaker = lj.value(QStringLiteral("speaker")).toString();
        l.text = lj.value(QStringLiteral("text")).toString();
        l.voiceId = lj.value(QStringLiteral("voiceId")).toString();
        l.outputPath = lj.value(QStringLiteral("outputPath")).toString();
        l.done = lj.value(QStringLiteral("done")).toBool(false);
        plan.lines.append(l);
    }
    return plan;
}

VoiceRunReport runVoicePlan(VoiceLinePlan& plan, IVoiceSynthesizer* synth)
{
    VoiceRunReport report;

    for (VoiceLine& line : plan.lines)
    {
        if (line.done)
            continue;

        const bool ok = synth && synth->isAvailable()
            && synth->synthesize(line.text, line.voiceId, line.outputPath);
        if (ok)
        {
            line.done = true;
            ++report.completed;
        }
        else
        {
            ++report.failed;
            report.failures.append(line.lineId);
        }
    }

    return report;
}

#ifdef _WIN32

namespace {

// Single-quote escaping for PowerShell string literals: doubling the quote
// is the documented escape inside '...', so embedded text can never break
// out of the literal.
QString psQuoted(const QString& s)
{
    QString escaped = s;
    escaped.replace(QLatin1Char('\''), QStringLiteral("''"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
}

// Runs powershell.exe -NoProfile -Command <script>, capturing stdout.
// Returns the exit code, or -1 on start/timeout failure.
int runPowerShell(const QString& script, QString* output, int timeoutMs)
{
    QProcess proc;
    proc.start(QStringLiteral("powershell"),
               {QStringLiteral("-NoProfile"), QStringLiteral("-ExecutionPolicy"),
                QStringLiteral("Bypass"), QStringLiteral("-Command"), script});
    if (!proc.waitForStarted(10000))
        return -1;
    if (!proc.waitForFinished(timeoutMs))
    {
        proc.kill();
        proc.waitForFinished(5000);
        return -1;
    }
    if (output)
        *output = QString::fromLocal8Bit(proc.readAllStandardOutput());
    return proc.exitCode();
}

} // namespace

QString SapiVoiceSynthesizer::name() const
{
    return QStringLiteral("SAPI");
}

QStringList SapiVoiceSynthesizer::availableVoices()
{
    QStringList voices;
    QString output;
    const QString script =
        QStringLiteral("Add-Type -AssemblyName System.Speech;")
        + QStringLiteral("$s = New-Object System.Speech.Synthesis.SpeechSynthesizer;")
        + QStringLiteral("$s.GetInstalledVoices().VoiceInfo.Name;")
        + QStringLiteral("$s.Dispose()");
    if (runPowerShell(script, &output, 30000) != 0)
        return voices;
    for (const QString& line : output.split(QLatin1Char('\n')))
    {
        const QString name = line.trimmed();
        if (!name.isEmpty())
            voices.append(name);
    }
    return voices;
}

bool SapiVoiceSynthesizer::isAvailable() const
{
    return !availableVoices().isEmpty();
}

bool SapiVoiceSynthesizer::synthesize(const QString& text, const QString& voiceId,
                                      const QString& outputPath)
{
    if (text.trimmed().isEmpty() || outputPath.trimmed().isEmpty())
        return false;

    // The runner passes plan-relative paths; make sure the target exists.
    const QFileInfo target(outputPath);
    if (!target.dir().mkpath(QStringLiteral(".")))
        return false;

    // Pick the requested voice by display-name substring; an empty voiceId
    // (or no match) keeps the default voice. try/catch maps any speech
    // failure to a nonzero exit code.
    const QString script =
        QStringLiteral("try { Add-Type -AssemblyName System.Speech;")
        + QStringLiteral("$s = New-Object System.Speech.Synthesis.SpeechSynthesizer;")
        + QStringLiteral("$wanted = ") + psQuoted(voiceId.trimmed())
        + QStringLiteral("; if ($wanted -ne '') { foreach ($v in $s.GetInstalledVoices())")
        + QStringLiteral(" { if ($v.VoiceInfo.Name -like ('*' + $wanted + '*'))")
        + QStringLiteral(" { $s.SelectVoice($v.VoiceInfo.Name); break } } }")
        + QStringLiteral("$s.SetOutputToWaveFile(") + psQuoted(outputPath)
        + QStringLiteral("); $s.Speak(") + psQuoted(text)
        + QStringLiteral("); $s.Dispose(); exit 0 } catch { exit 1 }");
    if (runPowerShell(script, nullptr, 60000) != 0)
        return false;

    const QFileInfo written(outputPath);
    return written.exists() && written.size() > 44;
}

#endif // _WIN32

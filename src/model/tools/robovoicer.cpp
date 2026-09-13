#include "robovoicer.hpp"

#include <QJsonArray>

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

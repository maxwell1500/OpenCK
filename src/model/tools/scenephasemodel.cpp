#include "scenephasemodel.hpp"

void ScenePhaseModel::insertPhase(QVector<ScenePhase>& phases, int index,
                                  const ScenePhase& phase)
{
    int idx = qBound(0, index, phases.size());

    double start = phase.startTime;
    if (idx > 0 && start < phases[idx - 1].endTime)
        start = phases[idx - 1].endTime;

    ScenePhase p = phase;
    p.startTime = start;
    if (p.endTime <= p.startTime)
        p.endTime = p.startTime + 1.0;

    phases.insert(idx, p);
}

void ScenePhaseModel::removePhase(QVector<ScenePhase>& phases, int index)
{
    if (index < 0 || index >= phases.size())
        return;
    phases.removeAt(index);
}

void ScenePhaseModel::movePhaseStart(QVector<ScenePhase>& phases, int index,
                                     double newStart)
{
    if (index < 0 || index >= phases.size())
        return;

    const double duration = phases[index].duration();
    double lo = 0.0;
    if (index > 0)
        lo = phases[index - 1].endTime;
    double hi = duration;
    if (index + 1 < phases.size())
        hi = phases[index + 1].endTime - duration;

    double start = qBound(lo, newStart, hi);
    phases[index].startTime = start;
    phases[index].endTime = start + duration;
}

void ScenePhaseModel::pack(QVector<ScenePhase>& phases, double gap)
{
    double cursor = 0.0;
    for (ScenePhase& phase : phases)
    {
        const double duration = phase.duration();
        phase.startTime = cursor;
        phase.endTime = cursor + duration;
        cursor = phase.endTime + gap;
    }
}

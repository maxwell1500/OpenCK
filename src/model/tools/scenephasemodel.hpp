#ifndef SCENEPHASEMODEL_HPP
#define SCENEPHASEMODEL_HPP

#include <QString>
#include <QVector>

// A scene (SCEN) phase in the timeline. Each phase has a start/end time
// (in seconds) and an optional name. This is the editor-side model for the
// Scene Timeline; the on-disk PHDA encoding round-trips through the record's
// raw subrecords until the binary format can be validated against real data.
struct ScenePhase
{
    QString name;
    double startTime = 0.0;
    double endTime = 1.0;
    quint32 flags = 0;

    double duration() const { return endTime - startTime; }
    bool isValid() const { return endTime > startTime; }
};

// Editing operations over a phase list, kept as pure functions so the
// timeline widget and tests share the same semantics.
struct ScenePhaseModel
{
    // Inserts a phase, clamping its start to >= previous end.
    static void insertPhase(QVector<ScenePhase>& phases, int index,
                            const ScenePhase& phase);

    // Removes the phase at index and closes the gap on its successor.
    static void removePhase(QVector<ScenePhase>& phases, int index);

    // Moves a phase's start time; clamps so it stays after the previous
    // phase and before the next (keeps the list sorted and non-overlapping).
    static void movePhaseStart(QVector<ScenePhase>& phases, int index,
                               double newStart);

    // Normalizes phases to start at 0 and pack consecutively, preserving
    // relative durations.
    static void pack(QVector<ScenePhase>& phases, double gap = 0.1);
};

#endif // SCENEPHASEMODEL_HPP

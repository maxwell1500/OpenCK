#ifndef HOUDINIBRIDGE_HPP
#define HOUDINIBRIDGE_HPP

#include <QString>
#include <QStringList>

// Houdini integration (REMAINING.md §3.8). OpenCK does not embed Houdini; it
// prepares a launch/batch command line that runs a user-supplied export or
// import script against a .hip file. This keeps the integration to a thin,
// testable command builder plus a dialog that fills it in.
struct HoudiniBridge
{
    QString houdiniExecutable;     // e.g. "C:/.../Houdini 20/bin/hython.exe"
    QString hipFile;               // .hip file to open (interactive only)
    QStringList extraArgs;

    // The command that launches an interactive Houdini session. The .hip file
    // (if any) is passed as the first argument, followed by extraArgs.
    QStringList interactiveCommand() const;

    // The command that runs `script` headlessly through hython, appending
    // `scriptArgs`.
    QStringList batchCommand(const QString& script,
                             const QStringList& scriptArgs = {}) const;

    // True when the executable path has been set.
    bool isConfigured() const { return !houdiniExecutable.trimmed().isEmpty(); }

    static QString defaultBatchExecutable();
};

#endif // HOUDINIBRIDGE_HPP

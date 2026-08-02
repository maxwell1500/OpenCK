#pragma once

#include <QString>
#include <QStringList>

// Thin wrapper around the Perforce p4 CLI for the version-control features
// (Check In / Check Out). Commands run in the given working directory; p4
// must be on PATH and a workspace (client) must map the directory.
class PerforceRepository
{
public:
    struct Result
    {
        bool ok = false;
        int exitCode = -1;
        QString stdoutText;
        QString stderrText;
    };

    // True if `p4` resolves on PATH.
    static bool isAvailable();

    // Runs `p4 <args>` in dir. Returns stdout/stderr + exit code.
    static Result run(const QString& dir, const QStringList& args);

    // True if dir is inside a Perforce workspace (p4 info succeeds for it).
    static bool isWorkspace(const QString& dir);

    // Opens the given files for edit (p4 edit). This is "check out".
    static Result checkOut(const QString& dir, const QStringList& paths);

    // Submits the given files (p4 submit -d <message> <paths>). This is
    // "check in". Returns false if p4 asks for a changelist interactively.
    static Result checkIn(const QString& dir, const QStringList& paths,
                          const QString& message);

    // Reverts local changes to the given files.
    static Result revert(const QString& dir, const QStringList& paths);

    // Lists files opened for edit (p4 opened).
    static Result opened(const QString& dir);

    // Returns `p4 diff -sd` output (summary of local diffs).
    static Result diffStat(const QString& dir);

    // Returns the client (workspace) name, or empty if not in a workspace.
    static QString clientName(const QString& dir);
};

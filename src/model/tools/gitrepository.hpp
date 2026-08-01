#ifndef GITREPOSITORY_HPP
#define GITREPOSITORY_HPP

#include <QString>
#include <QStringList>

// Thin wrapper around the git CLI for the version-control features
// (Check In / Check Out, commit, diff, status). All commands are run
// in the given working directory. Git must be on PATH.
class GitRepository
{
public:
    struct Result
    {
        bool ok = false;
        int exitCode = -1;
        QString stdoutText;
        QString stderrText;
    };

    // True if `git` resolves on PATH.
    static bool isAvailable();

    // Runs `git <args>` in dir. Returns stdout/stderr + exit code.
    static Result run(const QString& dir, const QStringList& args);

    // Returns true if dir is inside a git work tree.
    static bool isRepository(const QString& dir);

    // Commits the given paths (relative to dir) with message.
    static Result commitFiles(const QString& dir, const QStringList& paths,
                              const QString& message);

    // Adds the given paths to the index.
    static Result stageFiles(const QString& dir, const QStringList& paths);

    // Returns a short status summary (" M foo.esp\n?? bar.esp").
    static Result status(const QString& dir);

    // Returns `git diff --stat` output.
    static Result diffStat(const QString& dir);

    // Returns the current branch name (or "HEAD" detached).
    static QString currentBranch(const QString& dir);

    // Returns the full path of the .git directory for the work tree
    // containing dir, or empty if not a repository.
    static QString gitDir(const QString& dir);
};

#endif // GITREPOSITORY_HPP

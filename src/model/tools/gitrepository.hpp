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

    // Local branch names, one per line (current branch first). Empty when
    // the repo has no commits yet.
    static QStringList branches(const QString& dir);

    // Switches to an existing branch (fails when it does not exist or the
    // work tree is dirty in a way git refuses to carry over).
    static Result checkout(const QString& dir, const QString& branch);

    // Fetches and merges the remote tracking branch of the current branch.
    static Result pull(const QString& dir);

    // Pushes the current branch to its remote.
    static Result push(const QString& dir);

    // Fetches remotes without touching the work tree.
    static Result fetch(const QString& dir);

    // Full path of the .git directory for the work tree
    // containing dir, or empty if not a repository.
    static QString gitDir(const QString& dir);
};

#endif // GITREPOSITORY_HPP

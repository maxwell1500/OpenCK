#ifndef PAPYRUSCOMPILER_HPP
#define PAPYRUSCOMPILER_HPP

#include <QObject>
#include <QString>
#include <QProcess>
#include <QVector>
#include <QMap>
#include <QSet>
#include "../../../libs/files/filepaths.hpp"

struct CompilerError
{
    QString file;
    int line;
    int column;
    QString message;
    QString errorId;
    enum class Severity { Warning, Error, Fatal };
    Severity severity;
};

struct ScriptDependency
{
    QString scriptName;
    QVector<QString> dependencies;
    bool compiled = false;
    bool operator==(const ScriptDependency& other) const { return scriptName == other.scriptName; }
};

class PapyrusCompiler : public QObject
{
    Q_OBJECT

public:
    explicit PapyrusCompiler(QObject* parent = nullptr);
    ~PapyrusCompiler();

    bool setCompilerPath(const QString& path);
    static QString detectCompilerPath();
    bool setScriptPath(const QString& path);
    bool setOutputPath(const QString& path);
    void addIncludePath(const QString& path);

    void setGameVersion(GameId version);
    QStringList getCompilerFlags() const;
    GameId getGameVersion() const { return gameVersion; }

    // Loads and validates a Papyrus script flag file (.flg) against the
    // known property flags. Returns a human-readable list of problems,
    // empty if the file parses cleanly (or is absent).
    static QStringList validateFlagFile(const QString& flagFilePath);

    QVector<CompilerError> getLastErrors() const { return lastErrors; }

    bool compile();
    
    bool parseCompileOrder(const QString& filePath);
    QVector<QString> getCompilationOrder() const;
    bool hasCircularDependencies() const;
    QVector<QString> getCircularDependencies() const;
    bool batchCompile(const QVector<QString>& scriptPaths);
    
    void clearDependencies();

signals:
    void compilationStarted();
    void compilationFinished(bool success);
    void compilationError(const QString& error);
    void logMessage(const QString& message, bool isError);
    void batchProgress(int current, int total);

private:
    void parseCompilerOutput(const QString& output);
    bool parseWithStrategy1(const QString& line, CompilerError& error);
    bool parseWithStrategy2(const QString& line, CompilerError& error);
    bool parseWithStrategy3(const QString& line, CompilerError& error);
    bool parseWithStrategy4(const QString& line, CompilerError& error);
    QString extractLineNumber(const QString& line, int startPos) const;
    QString escapePath(const QString& path) const;
    bool compileScript(const QString& scriptPath);

    struct TopologicalResult {
        QVector<QString> order;
        bool hasCycle = false;
    };
    TopologicalResult topologicalSort() const;

    bool detectCycleDFS(const QString& node, QSet<QString>& visited, QSet<QString>& recStack, QVector<QString>& cyclePath) const;
    QString extractScriptName(const QString& path) const;
    QString getScriptPathForName(const QString& scriptName, const QString& compileOrderPath);

    QString compilerPath;
    QString scriptPath;
    QString outputPath;
    QString modFile;
    QVector<QString> includePaths;
    QVector<QString> libraries;
    QVector<CompilerError> lastErrors;
    QString output;
    QProcess process;
    GameId gameVersion = Game_None;
    
    QMap<QString, ScriptDependency> dependencyGraph;
    QVector<QString> compilationOrder;
    bool hasCircularDeps = false;
    QVector<QString> circularDepList;
};

#endif // PAPYRUSCOMPILER_HPP

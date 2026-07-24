#ifndef PAPYRUSDEBUGGER_H
#define PAPYRUSDEBUGGER_H

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QSet>
#include <QVector>
#include <QMap>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class Data;

struct FunctionInfo
{
    QString name;
    int startLine;
    int endLine;
    QStringList parameterNames;
};

struct VariableInfo
{
    QString name;
    QString type;
    QString value;
    int line;
};

enum class DebugState
{
    Stopped,
    Running,
    Stepping,
    Paused
};

class PapyrusDebugger : public QDialog
{
    Q_OBJECT

public:
    PapyrusDebugger(Data* data, QWidget* parent = nullptr);
    ~PapyrusDebugger();

private slots:
    void loadScripts();
    void selectScript(QTreeWidgetItem* item, int column);
    void runScript();
    void stopScript();
    void stepOver();
    void stepInto();
    void stepOut();
    void toggleBreakpoint();
    void updateVariables();
    void updateCallStack();

private:
    void setupUI();
    void loadScriptFile(const QString& filePath);
    void parseScript(const QString& content);
    void highlightCurrentLine(int line);
    void highlightBreakpoints();
    void updateBreakpointList();
    void clearHighlighting();
    int findNextExecutableLine(int fromLine);
    int findEndOfFunction(int startLine);
    bool isFunctionCallLine(const QString& line);
    QString extractFunctionName(const QString& line);
    QString extractCallFunctionName(const QString& line);
    bool isCommentOrBlank(const QString& line);
    void pushCallStack(const QString& funcName, int line);
    void popCallStack();

    Data* mData;

    // Script list
    QTreeWidget* mScriptList;
    QListWidget* mBreakpointList;

    // Code editor
    QTextEdit* mCodeEditor;

    // Variables and call stack
    QTreeWidget* mVariableList;
    QTreeWidget* mCallStackList;

    // Status
    QLabel* mStatusLabel;

    // Debug state
    DebugState mState;
    int mCurrentLine;
    QString mCurrentScript;
    QString mScriptContent;
    QStringList mScriptLines;

    // Breakpoints
    QSet<int> mBreakpoints;

    // Parsed script data
    QVector<FunctionInfo> mFunctions;
    QVector<VariableInfo> mVariables;
    QMap<QString, FunctionInfo> mFunctionMap;

    // Call stack (function name -> source line in caller)
    struct CallFrame
    {
        QString functionName;
        int returnLine;
    };
    QVector<CallFrame> mCallStack;

    // Current function context
    QString mCurrentFunction;
    int mCurrentFunctionStart;
    int mCurrentFunctionEnd;

    // Visual formats
    QTextCharFormat mCurrentLineFormat;
    QTextCharFormat mBreakpointFormat;
    QTextCharFormat mNormalFormat;
};

#endif // PAPYRUSDEBUGGER_H

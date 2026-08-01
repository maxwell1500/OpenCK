#ifndef SCRIPTEDITORWIDGET_HPP
#define SCRIPTEDITORWIDGET_HPP

#include <QWidget>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QFont>
#include <QVector>
#include <QTreeWidget>
#include <QPushButton>
#include <QSplitter>
#include <QRegularExpression>

class ScriptTextEdit : public QPlainTextEdit
{
public:
    using QPlainTextEdit::QPlainTextEdit;
    using QPlainTextEdit::firstVisibleBlock;
    using QPlainTextEdit::blockBoundingGeometry;
    using QPlainTextEdit::contentOffset;
    using QPlainTextEdit::blockBoundingRect;
    using QAbstractScrollArea::setViewportMargins;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    QString leadingWhitespace(const QString& line) const;
    bool isControlFlowBlockStart(const QString& line) const;
    bool isControlFlowBlockEnd(const QString& line) const;
};

class LineNumberWidget;
class PapyrusCompiler;
class QCompleter;
struct CompilerError;

class ScriptEditorWidget : public QWidget
{
    Q_OBJECT

    friend class LineNumberWidget;

public:
    explicit ScriptEditorWidget(QWidget* parent = nullptr);
    ~ScriptEditorWidget();

    void loadScript(const QString& fileName);
    void saveScript();

    bool compile(const QString& outputPath);
    void setCompilerPath(const QString& path);
    void setCompilerOutputDir(const QString& path);
    void addCompilerIncludePath(const QString& path);
    void gotoError(const CompilerError& error);

    void lineNumberPaintEvent(QPaintEvent* event);
    int lineNumberWidth() const;

    void toggleFold(QTextBlock& block);

    void logToConsole(const QString& message);
    void logErrorToConsole(const QString& message);
    void clearConsole();

    void checkSpelling();

    static const int FoldMarkerWidth = 14;

signals:
    void compilationStarted();
    void compilationFinished(bool success);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupSyntaxHighlighter();
    void setupFont();
    void setupCompleter();
    void setupReferenceBrowser();
    void setupConsole();
    void performCompletion();
    void updateLineNumberWidth(int blockCount);
    void updateLineNumber(const QRect& rect, int dy);
    void highlightErrorLine(int lineNumber);
    void updateFoldRegions();
    void recalcVisibility();
    void refreshTypeSquiggles();

    bool findFoldStart(const QString& text, QString& keyword) const;
    bool findFoldEnd(const QString& text, QString& keyword) const;
    QString foldEndToStart(const QString& endKeyword) const;

    void toggleReferenceBrowser();
    void toggleConsole();
    void onBrowserItemDoubleClicked(QTreeWidgetItem* item, int column);
    void refreshSpellingSquiggles();
    void loadBuiltinDictionary();
    void maybeStartLsp();
    void onLspResponse(int id, const QJsonObject& body);

    ScriptTextEdit* m_textEdit;
    QSyntaxHighlighter* highlighter;
    LineNumberWidget* lineNumberWidget;
    PapyrusCompiler* compiler;
    QCompleter* completer;
    QString currentFileName;
    QString compilerOutputDir;
    bool modified;
    bool updatingFolds;

    QTreeWidget* m_referenceBrowser;
    QPushButton* m_toggleBrowserBtn;
    QWidget* m_browserContainer;

    QPlainTextEdit* m_consoleOutput;
    QSplitter* m_splitter;
    QPushButton* m_toggleConsoleBtn;
    QWidget* m_consoleContainer;

    class PapyrusTypeChecker* m_typeChecker;
    class QToolTip* m_toolTipHelper;
    class SpellChecker* m_spellChecker;
    class PapyrusLanguageServer* m_lsp;
};

class LineNumberWidget : public QWidget
{
public:
    explicit LineNumberWidget(ScriptTextEdit* textEdit, ScriptEditorWidget* editor)
        : QWidget(textEdit), mTextEdit(textEdit), mEditor(editor) {}

    QSize sizeHint() const override {
        return QSize(mEditor->lineNumberWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        mEditor->lineNumberPaintEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override;

private:
    ScriptTextEdit* mTextEdit;
    ScriptEditorWidget* mEditor;
};

#endif // SCRIPTEDITORWIDGET_HPP

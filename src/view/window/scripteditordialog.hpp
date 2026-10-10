#ifndef SCRIPTEDITORDIALOG_HPP
#define SCRIPTEDITORDIALOG_HPP

#include <QDialog>
#include <QString>

class QCompleter;
class QDialogButtonBox;
class QLabel;
class QListWidget;
class QPlainTextEdit;
class ObScriptHighlighter;

// Editor for a script record's OBScript source (SCTX). Shows the source with
// syntax highlighting, a full diagnostics list (syntax, semantic and
// cross-script) with click-to-jump, and completion from keywords, symbols and
// game natives. Saving is blocked while a compile-level error is present.
class ScriptEditorDialog : public QDialog
{
    Q_OBJECT

public:
    /// `gameFlavor` selects the native catalog the diagnostics are checked
    /// against; `knownScripts` are the plugin's own SCRO editor ids.
    ScriptEditorDialog(const QString& editorId, const QString& scriptText,
                       const QString& gameFlavor = QStringLiteral("Unknown"),
                       const QStringList& knownScripts = QStringList(),
                       QWidget* parent = nullptr);

    QString scriptText() const;

    /// True when the current text has a compile-blocking error.
    bool hasBlockingError() const;

    /// Jumps the text cursor to a source line (1-based).
    void gotoLine(int line);

    /// The dialect the catalog was chosen from, for display.
    QString flavorName() const;

private slots:
    void checkSyntax();
    void onDiagnosticActivated();

private:
    QPlainTextEdit* m_edit = nullptr;
    QLabel* m_status = nullptr;
    QListWidget* m_issues = nullptr;
    ObScriptHighlighter* m_highlighter = nullptr;
    QCompleter* m_completer = nullptr;
    QString m_flavorName;
    QStringList m_knownScripts;
    bool m_hasBlockingError = false;
    QDialogButtonBox* m_buttons = nullptr;
};

#endif // SCRIPTEDITORDIALOG_HPP

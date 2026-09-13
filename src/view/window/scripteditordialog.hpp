#ifndef SCRIPTEDITORDIALOG_HPP
#define SCRIPTEDITORDIALOG_HPP

#include <QDialog>
#include <QString>

class QCompleter;
class QLabel;
class QPlainTextEdit;
class ObScriptHighlighter;

// Editor for a script record's OBScript source (SCTX). Shows the source with
// syntax highlighting, a live syntax check (via ObScript::parse) in a status
// line, and completion from keywords and symbols used in the script. The
// caller reads the edited text back with scriptText() on accept.
class ScriptEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptEditorDialog(const QString& editorId, const QString& scriptText,
                                QWidget* parent = nullptr);

    QString scriptText() const;

private slots:
    void checkSyntax();

private:
    QPlainTextEdit* m_edit = nullptr;
    QLabel* m_status = nullptr;
    ObScriptHighlighter* m_highlighter = nullptr;
    QCompleter* m_completer = nullptr;
};

#endif // SCRIPTEDITORDIALOG_HPP

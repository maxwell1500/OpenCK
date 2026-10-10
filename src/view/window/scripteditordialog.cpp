#include "scripteditordialog.hpp"

#include "obscripthighlighter.hpp"

#include "../../../libs/files/esm/obscriptbinder.hpp"
#include "../../../libs/files/esm/obscriptcatalog.hpp"
#include "../../../libs/files/esm/obscriptparser.hpp"
#include "../../../libs/files/esm/obscriptresolver.hpp"
#include "../../../libs/files/esm/obscripttypechecker.hpp"
#include "../../../libs/files/log/logger.hpp"

#include <QCompleter>
#include <QDialogButtonBox>
#include <QEvent>
#include <QKeyEvent>
#include <QListWidget>
#include <QListWidgetItem>
#include <QObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStringListModel>
#include <QTextCursor>

#include <QFontDatabase>
#include <QLabel>
#include <QVBoxLayout>

namespace
{

// Pops the word completion on Ctrl+Space. Kept as an event filter so the
// editor itself stays a plain QPlainTextEdit.
class CompletionEventFilter : public QObject
{
public:
    CompletionEventFilter(QPlainTextEdit* edit, QCompleter* completer)
        : QObject(edit), m_edit(edit), m_completer(completer)
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched != m_edit || event->type() != QEvent::KeyPress)
        {
            return QObject::eventFilter(watched, event);
        }
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if (key->modifiers().testFlag(Qt::ControlModifier)
            && key->key() == Qt::Key_Space)
        {
            QTextCursor cursor = m_edit->textCursor();
            cursor.select(QTextCursor::WordUnderCursor);
            const QString prefix = cursor.selectedText();
            m_completer->setCompletionPrefix(prefix);
            const QRect rect = m_edit->cursorRect();
            m_completer->complete(rect.adjusted(0, 0, 0, 180));
            return true;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QPlainTextEdit* m_edit = nullptr;
    QCompleter* m_completer = nullptr;
};

} // namespace

ScriptEditorDialog::ScriptEditorDialog(const QString& editorId,
                                       const QString& scriptText,
                                       const QString& gameFlavor,
                                       const QStringList& knownScripts,
                                       QWidget* parent)
    : QDialog(parent)
    , m_flavorName(gameFlavor)
    , m_knownScripts(knownScripts)
{
    LOG_INFO(QString("ScriptEditorDialog: opening script editor for '%1' (flavor '%2')")
                 .arg(editorId, m_flavorName));

    setWindowTitle(editorId.isEmpty()
                       ? tr("Script Editor")
                       : tr("Script Editor - %1").arg(editorId));
    resize(900, 620);

    auto* layout = new QVBoxLayout(this);

    m_edit = new QPlainTextEdit(this);
    m_edit->setPlainText(scriptText);
    m_edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_edit->setTabStopDistance(4 * m_edit->fontMetrics().horizontalAdvance(QLatin1Char(' ')));
    layout->addWidget(m_edit);

    m_highlighter = new ObScriptHighlighter(m_edit->document());

    // Completion is seeded with keywords, script symbols, the game's native
    // functions and the plugin's other scripts.
    QStringList words = ObScript::completionEntries(ObScript::parse(scriptText));
    words += ObScript::nativeNamesFor(ObScript::gameFlavorFromName(m_flavorName));
    words += m_knownScripts;
    words.removeDuplicates();
    words.sort();
    m_completer = new QCompleter(words, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setWidget(m_edit);
    m_edit->installEventFilter(new CompletionEventFilter(m_edit, m_completer));

    m_issues = new QListWidget(this);
    m_issues->setMaximumHeight(120);
    m_issues->setVisible(false);
    layout->addWidget(m_issues);
    connect(m_issues, &QListWidget::itemClicked, this,
            &ScriptEditorDialog::onDiagnosticActivated);

    m_status = new QLabel(this);
    layout->addWidget(m_status);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Discard, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    m_buttons = buttons;

    connect(m_edit, &QPlainTextEdit::textChanged, this,
            &ScriptEditorDialog::checkSyntax);
    checkSyntax();
}

QString ScriptEditorDialog::scriptText() const
{
    return m_edit->toPlainText();
}

bool ScriptEditorDialog::hasBlockingError() const
{
    return m_hasBlockingError;
}

QString ScriptEditorDialog::flavorName() const
{
    return m_flavorName;
}

void ScriptEditorDialog::gotoLine(int line)
{
    const int clamped = qMax(1, line);
    QTextCursor cursor = m_edit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    for (int i = 1; i < clamped && !cursor.atEnd(); ++i)
    {
        cursor.movePosition(QTextCursor::Down);
    }
    m_edit->setTextCursor(cursor);
    m_edit->setFocus();
}

void ScriptEditorDialog::onDiagnosticActivated()
{
    QListWidgetItem* item = m_issues->currentItem();
    if (!item)
    {
        return;
    }
    gotoLine(item->data(Qt::UserRole + 1).toInt());
}

void ScriptEditorDialog::checkSyntax()
{
    m_issues->clear();
    m_hasBlockingError = false;

    const QString source = m_edit->toPlainText();
    const ObScript::ParseResult parsed = ObScript::parse(source);

    auto addIssue = [this](bool isError, int line, const QString& message) {
        QListWidgetItem* item = new QListWidgetItem(
            tr("Line %1: %2").arg(line).arg(message), m_issues);
        item->setData(Qt::UserRole + 1, line);
        item->setForeground(isError ? QColor(0xc6, 0x28, 0x28)
                                    : QColor(0xf5, 0x7f, 0x17));
        if (isError)
        {
            m_hasBlockingError = true;
        }
    };

    if (!parsed.ok)
    {
        addIssue(true, parsed.errorLine, parsed.error);
        m_issues->setVisible(true);
        if (m_buttons)
        {
            m_buttons->button(QDialogButtonBox::Save)->setEnabled(false);
        }
        m_status->setText(tr("Line %1: %2").arg(parsed.errorLine).arg(parsed.error));
        m_status->setStyleSheet(QStringLiteral("color: #c62828;"));
        return;
    }

    const ObScript::GameFlavor flavor =
        ObScript::gameFlavorFromName(m_flavorName);

    // Native arity/type diagnostics.
    const ObScript::TypeCheckResult types =
        ObScript::typeCheck(parsed, ObScript::nativeCatalogFor(flavor));
    for (const auto& d : types.diagnostics)
    {
        addIssue(d.severity == ObScript::TypeSeverity::Error, d.line, d.message);
    }

    // Cross-script property resolution against the plugin's own scripts.
    QVector<ObScript::ScriptInfo> scripts;
    for (const QString& name : m_knownScripts)
    {
        ObScript::ScriptInfo info;
        info.name = name;
        scripts.append(info);
    }
    const ObScript::ResolveResult resolved =
        ObScript::resolveProgram(parsed, scripts);
    for (const auto& d : resolved.diagnostics)
    {
        addIssue(d.severity == ObScript::ResolveSeverity::Error, d.line,
                 d.message);
    }

    if (m_issues->count() > 0)
    {
        m_issues->setVisible(true);
    }

    QStringList words = ObScript::completionEntries(parsed);
    words += ObScript::nativeNamesFor(flavor);
    words += m_knownScripts;
    words.removeDuplicates();
    words.sort();
    m_completer->setModel(new QStringListModel(words, this));

    if (m_buttons)
    {
        m_buttons->button(QDialogButtonBox::Save)
            ->setEnabled(!m_hasBlockingError);
    }
    if (m_hasBlockingError)
    {
        m_status->setText(tr("%n problem(s) found — fix before saving.", nullptr,
                             m_issues->count()));
        m_status->setStyleSheet(QStringLiteral("color: #c62828;"));
        return;
    }

    if (!m_issues->count())
    {
        m_status->setText(tr("Syntax and types OK"));
        m_status->setStyleSheet(QStringLiteral("color: #66bb6a;"));
        return;
    }

    m_status->setText(tr("%n warning(s)", nullptr, m_issues->count()));
    m_status->setStyleSheet(QStringLiteral("color: #f57f17;"));
}

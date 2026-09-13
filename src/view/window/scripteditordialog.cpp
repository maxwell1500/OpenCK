#include "scripteditordialog.hpp"

#include "obscripthighlighter.hpp"

#include "../../../libs/files/esm/obscriptbinder.hpp"
#include "../../../libs/files/esm/obscriptparser.hpp"
#include "../../../libs/files/log/logger.hpp"

#include <QCompleter>
#include <QDialogButtonBox>
#include <QEvent>
#include <QKeyEvent>
#include <QObject>

#include <QFontDatabase>
#include <QLabel>
#include <QPlainTextEdit>
#include <QStringListModel>
#include <QVBoxLayout>

namespace
{

class CompletionEventFilter : public QObject
{
public:
    CompletionEventFilter(QPlainTextEdit* edit, QCompleter* completer)
        : m_edit(edit)
        , m_completer(completer)
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_edit && event->type() == QEvent::KeyPress)
        {
            auto* key = static_cast<QKeyEvent*>(event);
            if (key->key() == Qt::Key_Space &&
                (key->modifiers() & Qt::ControlModifier))
            {
                m_completer->complete();
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QPlainTextEdit* m_edit;
    QCompleter* m_completer;
};

} // namespace

ScriptEditorDialog::ScriptEditorDialog(const QString& editorId,
                                       const QString& scriptText, QWidget* parent)
    : QDialog(parent)
{
    LOG_INFO(QString("ScriptEditorDialog: opening script editor for '%1'").arg(editorId));

    setWindowTitle(editorId.isEmpty() ? tr("Script Editor")
                                      : tr("Script Editor - %1").arg(editorId));
    resize(760, 540);

    auto* layout = new QVBoxLayout(this);

    m_edit = new QPlainTextEdit(this);
    m_edit->setPlainText(scriptText);
    m_edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_edit->setTabStopDistance(4 * m_edit->fontMetrics().horizontalAdvance(QLatin1Char(' ')));
    layout->addWidget(m_edit);

    m_highlighter = new ObScriptHighlighter(m_edit->document());

    m_completer = new QCompleter(this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setWidget(m_edit);
    m_edit->installEventFilter(new CompletionEventFilter(m_edit, m_completer));

    m_status = new QLabel(this);
    layout->addWidget(m_status);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(m_edit, &QPlainTextEdit::textChanged, this, &ScriptEditorDialog::checkSyntax);
    checkSyntax();
}

QString ScriptEditorDialog::scriptText() const
{
    return m_edit->toPlainText();
}

void ScriptEditorDialog::checkSyntax()
{
    const ObScript::ParseResult result = ObScript::parse(m_edit->toPlainText());
    if (result.ok)
    {
        m_completer->setModel(new QStringListModel(ObScript::completionEntries(result), this));
        m_status->setText(tr("Syntax OK"));
        m_status->setStyleSheet(QStringLiteral("color: #2e7d32;"));
    }
    else
    {
        m_status->setText(tr("Line %1: %2").arg(result.errorLine).arg(result.error));
        m_status->setStyleSheet(QStringLiteral("color: #c62828;"));
    }
}

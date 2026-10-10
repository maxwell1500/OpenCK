#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QTest>

#include "view/window/obscripthighlighter.hpp"
#include "view/window/scripteditordialog.hpp"

class TestScriptEditor : public QObject
{
    Q_OBJECT

private slots:
    void testClassifyControlFlow();
    void testClassifyComment();
    void testClassifyStringAndOperator();
    void testClassifyLetType();
    void testClassifyHexNumber();
    void testClassifyCommentInsideString();
    void testClassifyEscapedQuote();
    void testDialogValidSyntax();
    void testDialogInvalidSyntax();
    void testDialogFlavorCatalog();
    void testDialogSaveGateOnBadNative();
    void testDialogCrossScriptWarning();
    void testDialogGotoLine();
};

static bool hasSpan(const QVector<ObScriptHighlightSpan>& spans, int start,
                    int length, ObScriptHighlightKind kind)
{
    for (const ObScriptHighlightSpan& s : spans)
    {
        if (s.start == start && s.length == length && s.kind == kind)
        {
            return true;
        }
    }
    return false;
}

void TestScriptEditor::testClassifyControlFlow()
{
    const QVector<ObScriptHighlightSpan> spans = classifyObScriptLine("if (x > 5)");
    QVERIFY(hasSpan(spans, 0, 2, ObScriptHighlightKind::ControlFlow));
    QVERIFY(hasSpan(spans, 6, 1, ObScriptHighlightKind::Operator));
    QVERIFY(hasSpan(spans, 8, 1, ObScriptHighlightKind::Number));

    const QVector<ObScriptHighlightSpan> end = classifyObScriptLine("endfunction");
    QVERIFY(hasSpan(end, 0, 11, ObScriptHighlightKind::ControlFlow));
}

void TestScriptEditor::testClassifyComment()
{
    const QVector<ObScriptHighlightSpan> spans = classifyObScriptLine("    ; a comment");
    QVERIFY(hasSpan(spans, 4, 11, ObScriptHighlightKind::Comment));
}

void TestScriptEditor::testClassifyStringAndOperator()
{
    const QVector<ObScriptHighlightSpan> spans = classifyObScriptLine("y = \"hi\"");
    QVERIFY(hasSpan(spans, 2, 1, ObScriptHighlightKind::Operator));
    QVERIFY(hasSpan(spans, 4, 4, ObScriptHighlightKind::String));
    QVERIFY(!hasSpan(spans, 0, 1, ObScriptHighlightKind::String));
}

void TestScriptEditor::testClassifyLetType()
{
    const QVector<ObScriptHighlightSpan> spans = classifyObScriptLine("let float z = 2.5");
    QVERIFY(hasSpan(spans, 0, 3, ObScriptHighlightKind::Keyword));
    QVERIFY(hasSpan(spans, 4, 5, ObScriptHighlightKind::Type));
    QVERIFY(hasSpan(spans, 12, 1, ObScriptHighlightKind::Operator));
    QVERIFY(hasSpan(spans, 14, 3, ObScriptHighlightKind::Number));
}

void TestScriptEditor::testClassifyHexNumber()
{
    const QVector<ObScriptHighlightSpan> spans = classifyObScriptLine("x = 0xFF");
    QVERIFY(hasSpan(spans, 4, 4, ObScriptHighlightKind::Number));
}

void TestScriptEditor::testClassifyCommentInsideString()
{
    const QVector<ObScriptHighlightSpan> spans = classifyObScriptLine("\"a;b\"");
    QVERIFY(hasSpan(spans, 0, 5, ObScriptHighlightKind::String));
    QVERIFY(!hasSpan(spans, 2, 2, ObScriptHighlightKind::Comment));
}

void TestScriptEditor::testClassifyEscapedQuote()
{
    const QVector<ObScriptHighlightSpan> spans = classifyObScriptLine("\"a\\\"b\"");
    QVERIFY(hasSpan(spans, 0, 6, ObScriptHighlightKind::String));
}

void TestScriptEditor::testDialogValidSyntax()
{
    const QString source = QStringLiteral("function f()\n    return 1\nendfunction\n");
    ScriptEditorDialog dlg(QStringLiteral("TestScript"), source);
    QCOMPARE(dlg.scriptText(), source);
    QLabel* status = dlg.findChild<QLabel*>();
    QVERIFY(status != nullptr);
    QVERIFY2(status->text().contains(QStringLiteral("OK")), qPrintable(status->text()));
}

void TestScriptEditor::testDialogInvalidSyntax()
{
    ScriptEditorDialog dlg(QStringLiteral("TestScript"),
                           QStringLiteral("if (x > 5)\n    y = 1\n"));
    QLabel* status = dlg.findChild<QLabel*>();
    QVERIFY(status != nullptr);
    QVERIFY2(status->text().contains(QStringLiteral("Line")), qPrintable(status->text()));
}

void TestScriptEditor::testDialogFlavorCatalog()
{
    // The flavor selects which natives validate. Skyrim SE has wait(); with
    // the cross-game builtins alone `wait()` is unknown.
    ScriptEditorDialog dlg(QStringLiteral("Flavored"),
                           QStringLiteral("function f()\n    wait(1.0)\nendfunction\n"),
                           QStringLiteral("Skyrim Special Edition"), QStringList());
    QVERIFY2(!dlg.hasBlockingError(), "Skyrim SE must know wait()");

    ScriptEditorDialog other(QStringLiteral("Unflavored"),
                             QStringLiteral("function f()\n    wait(1.0)\nendfunction\n"),
                             QStringLiteral("Unknown"), QStringList());
    QVERIFY(other.hasBlockingError() == false);
    QListWidget* issues = other.findChild<QListWidget*>();
    QVERIFY(issues != nullptr);
}

void TestScriptEditor::testDialogSaveGateOnBadNative()
{
    // AddItem() with no arguments is an arity error against the Skyrim SE
    // catalog, so the dialog must refuse to save.
    ScriptEditorDialog dlg(QStringLiteral("BadNative"),
                           QStringLiteral("function f()\n    AddItem()\nendfunction\n"),
                           QStringLiteral("Skyrim Special Edition"), QStringList());
    QVERIFY2(dlg.hasBlockingError(),
             "wrong arity against the flavor catalog must block saving");

    QListWidget* issues = dlg.findChild<QListWidget*>();
    QVERIFY(issues != nullptr);
    QVERIFY(issues->count() > 0);
}

void TestScriptEditor::testDialogCrossScriptWarning()
{
    // A property typed with a script the plugin does not ship is a warning,
    // not an error: it may come from a master file.
    ScriptEditorDialog dlg(QStringLiteral("Ref"),
                           QStringLiteral("OtherScript Property ref Auto\n"),
                           QStringLiteral("Skyrim Special Edition"),
                           QStringList());
    QVERIFY(!dlg.hasBlockingError());
    QListWidget* issues = dlg.findChild<QListWidget*>();
    QVERIFY(issues != nullptr);
    QCOMPARE(issues->count(), 1);

    // Passing the script as known clears the warning.
    ScriptEditorDialog known(QStringLiteral("Ref"),
                             QStringLiteral("OtherScript Property ref Auto\n"),
                             QStringLiteral("Skyrim Special Edition"),
                             QStringList{QStringLiteral("OtherScript")});
    QListWidget* knownIssues = known.findChild<QListWidget*>();
    QVERIFY(knownIssues != nullptr);
    QCOMPARE(knownIssues->count(), 0);
}

void TestScriptEditor::testDialogGotoLine()
{
    const QString source = QStringLiteral(
        "ScriptName Nav extends Quest\n"
        "Function A()\n"
        "  int x = 1\n"
        "EndFunction\n"
        "Function B()\n"
        "  int y = 2\n"
        "EndFunction\n");
    ScriptEditorDialog dlg(QStringLiteral("Nav"), source,
                           QStringLiteral("Skyrim Special Edition"));
    dlg.gotoLine(4);
    const QTextCursor cursor = dlg.findChild<QPlainTextEdit*>()->textCursor();
    QVERIFY(cursor.blockNumber() >= 2);
}

QTEST_MAIN(TestScriptEditor)
#include "test_scripteditor.moc"

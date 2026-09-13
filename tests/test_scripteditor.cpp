#include <QLabel>
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

QTEST_MAIN(TestScriptEditor)
#include "test_scripteditor.moc"

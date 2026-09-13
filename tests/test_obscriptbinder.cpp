#include <QTest>

#include "obscriptbinder.hpp"
#include "obscriptparser.hpp"

using namespace ObScript;

class TestObScriptBinder : public QObject
{
    Q_OBJECT

private slots:
    void testFunctionAndParams();
    void testDuplicateFunction();
    void testDuplicateParameter();
    void testDuplicateLocal();
    void testGlobalAndExternal();
    void testKnownExternalsSuppress();
    void testFieldAccessBaseOnly();
    void testForLoopVarIsLocal();
    void testCompletionEntries();
};

static bool hasSymbol(const QVector<BindSymbol>& symbols, const QString& name,
                      BindSymbolKind kind)
{
    for (const BindSymbol& s : symbols)
    {
        if (s.name == name && s.kind == kind)
        {
            return true;
        }
    }
    return false;
}

static bool hasError(const BindResult& r)
{
    for (const BindDiagnostic& d : r.diagnostics)
    {
        if (d.severity == BindSeverity::Error)
        {
            return true;
        }
    }
    return false;
}

void TestObScriptBinder::testFunctionAndParams()
{
    ParseResult p = parse("function Add(a, b)\n"
                          "    return a + b\n"
                          "endfunction\n");
    QVERIFY2(p.ok, qPrintable(p.error));
    BindResult r = bindProgram(p);
    QVERIFY(r.ok);
    QVERIFY(hasSymbol(r.symbols, "Add", BindSymbolKind::Function));
    QVERIFY(hasSymbol(r.symbols, "a", BindSymbolKind::Parameter));
    QVERIFY(hasSymbol(r.symbols, "b", BindSymbolKind::Parameter));
    QVERIFY(r.referencedExternals.isEmpty());
}

void TestObScriptBinder::testDuplicateFunction()
{
    ParseResult p = parse("function A()\n"
                          "endfunction\n"
                          "function A()\n"
                          "endfunction\n");
    QVERIFY2(p.ok, qPrintable(p.error));
    BindResult r = bindProgram(p);
    QVERIFY(!r.ok);
    QVERIFY(hasError(r));
}

void TestObScriptBinder::testDuplicateParameter()
{
    ParseResult p = parse("function A(x, x)\n"
                          "endfunction\n");
    QVERIFY2(p.ok, qPrintable(p.error));
    BindResult r = bindProgram(p);
    QVERIFY(!r.ok);
    QVERIFY(hasError(r));
}

void TestObScriptBinder::testDuplicateLocal()
{
    ParseResult p = parse("function A()\n"
                          "    let x = 1\n"
                          "    let x = 2\n"
                          "endfunction\n");
    QVERIFY2(p.ok, qPrintable(p.error));
    BindResult r = bindProgram(p);
    QVERIFY(!r.ok);
    QVERIFY(hasError(r));
}

void TestObScriptBinder::testGlobalAndExternal()
{
    ParseResult p = parse("begin s\n"
                          "    let g = 1\n"
                          "    g = g + UnknownThing\n"
                          "end\n");
    QVERIFY2(p.ok, qPrintable(p.error));
    BindResult r = bindProgram(p);
    QVERIFY(r.ok);
    QVERIFY(hasSymbol(r.symbols, "g", BindSymbolKind::Global));
    QVERIFY(r.referencedExternals.contains("UnknownThing"));
    QVERIFY(!r.referencedExternals.contains("g"));
}

void TestObScriptBinder::testKnownExternalsSuppress()
{
    ParseResult p = parse("function F()\n"
                          "    return GetActorValue(0)\n"
                          "endfunction\n");
    QVERIFY2(p.ok, qPrintable(p.error));

    BindResult without = bindProgram(p);
    QVERIFY(without.referencedExternals.contains("GetActorValue"));

    QSet<QString> known;
    known.insert("GetActorValue");
    BindResult with = bindProgram(p, known);
    QVERIFY(!with.referencedExternals.contains("GetActorValue"));
}

void TestObScriptBinder::testFieldAccessBaseOnly()
{
    ParseResult p = parse("function F()\n"
                          "    Debug.Notification(\"hi\")\n"
                          "endfunction\n");
    QVERIFY2(p.ok, qPrintable(p.error));
    BindResult r = bindProgram(p);
    QVERIFY(r.referencedExternals.contains("Debug"));
    QVERIFY(!r.referencedExternals.contains("Notification"));
}

void TestObScriptBinder::testForLoopVarIsLocal()
{
    ParseResult p = parse("function F()\n"
                          "    for i = 0 to 10\n"
                          "        let x = i\n"
                          "    endfor\n"
                          "endfunction\n");
    QVERIFY2(p.ok, qPrintable(p.error));
    BindResult r = bindProgram(p);
    QVERIFY(hasSymbol(r.symbols, "i", BindSymbolKind::Local));
    QVERIFY(!r.referencedExternals.contains("i"));
}

void TestObScriptBinder::testCompletionEntries()
{
    ParseResult p = parse("begin s\n"
                          "    function Add(a, b)\n"
                          "        return a + b\n"
                          "    endfunction\n"
                          "    let result = GetActorValue(0)\n"
                          "end\n");
    QVERIFY2(p.ok, qPrintable(p.error));
    QStringList entries = completionEntries(p);
    for (const QString& expected : {QStringLiteral("function"), QStringLiteral("let"),
                                    QStringLiteral("Add"), QStringLiteral("a"),
                                    QStringLiteral("result"),
                                    QStringLiteral("GetActorValue")})
    {
        QVERIFY2(entries.contains(expected), qPrintable(expected));
    }
}

QTEST_MAIN(TestObScriptBinder)
#include "test_obscriptbinder.moc"

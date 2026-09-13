#include <QtTest>
#include "../../libs/files/esm/obscripttypechecker.hpp"

using namespace ObScript;

class TestObScriptTypeChecker : public QObject
{
    Q_OBJECT

private slots:
    void testKnownCallValid();
    void testWrongArgCount();
    void testWrongArgType();
    void testUnknownFunctionWarning();
    void testPropertyCalledAsFunction();
    void testBuiltinCatalog();
};

void TestObScriptTypeChecker::testKnownCallValid()
{
    ParseResult pr = parse("print(\"hello\")\n");
    QVERIFY(pr.ok);
    TypeCheckResult r = typeCheck(pr, builtinCatalog());
    QVERIFY(r.ok);
    QCOMPARE(r.diagnostics.size(), 0);
}

void TestObScriptTypeChecker::testWrongArgCount()
{
    ParseResult pr = parse("print(\"a\", \"b\")\n");
    QVERIFY(pr.ok);
    TypeCheckResult r = typeCheck(pr, builtinCatalog());
    QVERIFY(!r.ok);
    QVERIFY(r.diagnostics.size() >= 1);
    QVERIFY(r.diagnostics[0].message.contains("expects"));
}

void TestObScriptTypeChecker::testWrongArgType()
{
    ParseResult pr = parse("getitemcount(\"notaform\")\n");
    QVERIFY(pr.ok);
    TypeCheckResult r = typeCheck(pr, builtinCatalog());
    QVERIFY(!r.ok);
    QVERIFY(r.diagnostics[0].message.contains("expects"));
}

void TestObScriptTypeChecker::testUnknownFunctionWarning()
{
    ParseResult pr = parse("myCustomFunction()\n");
    QVERIFY(pr.ok);
    TypeCheckResult r = typeCheck(pr, builtinCatalog());
    // Unknown functions are warnings, not errors
    QVERIFY(r.ok);
    QVERIFY(r.diagnostics.size() >= 1);
    QCOMPARE(r.diagnostics[0].severity, TypeSeverity::Warning);
}

void TestObScriptTypeChecker::testPropertyCalledAsFunction()
{
    ParseResult pr = parse("player()\n");
    QVERIFY(pr.ok);
    TypeCheckResult r = typeCheck(pr, builtinCatalog());
    QVERIFY(!r.ok);
    QVERIFY(r.diagnostics[0].message.contains("property"));
}

void TestObScriptTypeChecker::testBuiltinCatalog()
{
    const NativeCatalog cat = builtinCatalog();
    QVERIFY(cat.contains("print"));
    QVERIFY(cat.contains("PRINT")); // case-insensitive
    QVERIFY(cat.contains("player"));
    QVERIFY(!cat.contains("nonexistent"));
}

QTEST_MAIN(TestObScriptTypeChecker)
#include "test_obscripttypechecker.moc"

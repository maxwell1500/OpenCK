#include <QTest>

#include "obscriptcatalog.hpp"
#include "obscriptresolver.hpp"

using namespace ObScript;

// Cross-game native catalog plus cross-script property resolution. These are
// the two checks that make the script editor more useful than a text editor
// with spellcheck: a Skyrim-only native must not validate in Morrowind, and
// a property referencing a script the plugin does not ship must be flagged.
class TestObScriptCatalog : public QObject
{
    Q_OBJECT

private slots:
    void testFlavorFromName();
    void testFlavorSurfaceIsGated();
    void testKnownNatives();
    void testNamesSortedAndUnique();
    void testRealScriptHeaderParses();
    void testPropertyDeclarations();
    void testCompoundAssignment();
    void testResolvesKnownScript();
    void testFlagsMissingScript();
    void testPrimitivePropertiesAreSilent();
};

void TestObScriptCatalog::testFlavorFromName()
{
    QCOMPARE(gameFlavorFromName(QStringLiteral("Skyrim Special Edition")),
             GameFlavor::SkyrimSE);
    QCOMPARE(gameFlavorFromName(QStringLiteral("skyrimse")), GameFlavor::SkyrimSE);
    QCOMPARE(gameFlavorFromName(QStringLiteral("Skyrim Anniversary Edition")),
             GameFlavor::SkyrimSE);
    QCOMPARE(gameFlavorFromName(QStringLiteral("Skyrim")), GameFlavor::Skyrim);
    QCOMPARE(gameFlavorFromName(QStringLiteral("Fallout 4")), GameFlavor::Fallout4);
    QCOMPARE(gameFlavorFromName(QStringLiteral("Fallout New Vegas")),
             GameFlavor::FalloutNV);
    QCOMPARE(gameFlavorFromName(QStringLiteral("Morrowind")), GameFlavor::Morrowind);
    QCOMPARE(gameFlavorFromName(QStringLiteral("Starfield")), GameFlavor::Starfield);
    QCOMPARE(gameFlavorFromName(QStringLiteral("")), GameFlavor::Unknown);
    QCOMPARE(gameFlavorName(GameFlavor::SkyrimSE),
             QStringLiteral("Skyrim Special Edition"));
}

void TestObScriptCatalog::testFlavorSurfaceIsGated()
{
    // The curated surface applies only to Skyrim SE. Attributing it to other
    // flavors would silently validate natives those compilers do not ship.
    QVERIFY(hasKnownSurface(GameFlavor::SkyrimSE));
    QVERIFY(!hasKnownSurface(GameFlavor::Morrowind));
    QVERIFY(!hasKnownSurface(GameFlavor::Oblivion));
    QVERIFY(!hasKnownSurface(GameFlavor::Fallout4));
    QVERIFY(nativeCountFor(GameFlavor::SkyrimSE)
            > nativeCountFor(GameFlavor::Morrowind));
    QCOMPARE(nativeCountFor(GameFlavor::Fallout4),
             nativeCountFor(GameFlavor::Morrowind));
}

void TestObScriptCatalog::testKnownNatives()
{
    const NativeCatalog catalog = nativeCatalogFor(GameFlavor::SkyrimSE);
    QVERIFY(catalog.contains(QStringLiteral("additem")));
    QVERIFY(catalog.contains(QStringLiteral("getactorvalue")));
    QVERIFY(catalog.contains(QStringLiteral("wait")));

    const NativeFunction* additem = catalog.find(QStringLiteral("AddItem"));
    QVERIFY(additem != nullptr);
    QCOMPARE(int(additem->params.size()), 3);
    QCOMPARE(additem->returnType, ValueType::Nil);

    // Unknown flavors only carry the cross-game builtins.
    const NativeCatalog mw = nativeCatalogFor(GameFlavor::Morrowind);
    QVERIFY(mw.contains(QStringLiteral("print")));
    QVERIFY(!mw.contains(QStringLiteral("wait")));
}

void TestObScriptCatalog::testNamesSortedAndUnique()
{
    const QStringList names = nativeNamesFor(GameFlavor::SkyrimSE);
    QVERIFY(!names.isEmpty());
    for (int i = 1; i < names.size(); ++i)
    {
        QVERIFY(names.at(i - 1) <= names.at(i));
    }
    QCOMPARE(names.size(), nativeCountFor(GameFlavor::SkyrimSE));
}

void TestObScriptCatalog::testRealScriptHeaderParses()
{
    const ParseResult r = parse(QStringLiteral(
        "ScriptName MyQuest extends Quest\n"));
    QVERIFY(r.ok);
    QCOMPARE(int(r.statements.size()), 1);
    const Statement& h = *r.statements.at(0);
    QCOMPARE(h.kind, StmtKind::Header);
    QCOMPARE(h.funcName, QStringLiteral("MyQuest"));
    // The lexer lower-cases keyword tokens, so `Quest` must not arrive as `quest`.
    QCOMPARE(h.typeName, QStringLiteral("Quest"));
}

void TestObScriptCatalog::testPropertyDeclarations()
{
    const ParseResult r = parse(QStringLiteral(
        "ScriptName S extends Quest\n"
        "MyOtherScript Property ref Auto\n"
        "int Property count Auto\n"
        "float Property waitTime = 2.0\n"));
    QVERIFY(r.ok);
    const QVector<PropertyDecl> props = propertyDecls(r);
    QCOMPARE(int(props.size()), 3);
    QCOMPARE(props.at(0).typeName, QStringLiteral("MyOtherScript"));
    QCOMPARE(props.at(0).name, QStringLiteral("ref"));
    QVERIFY(props.at(0).isAuto);
    QCOMPARE(props.at(1).typeName, QStringLiteral("int"));
    QCOMPARE(props.at(1).name, QStringLiteral("count"));
    QVERIFY(props.at(1).isAuto);
    QCOMPARE(props.at(2).typeName, QStringLiteral("float"));
    QCOMPARE(props.at(2).name, QStringLiteral("waitTime"));
    QVERIFY(!props.at(2).isAuto);
}

void TestObScriptCatalog::testCompoundAssignment()
{
    const ParseResult r = parse(QStringLiteral(
        "ScriptName S\n"
        "int i = 0\n"
        "i += 1\n"));
    QVERIFY(r.ok);
    // The desugared form is a plain Let with a BinaryOp value, so every
    // downstream pass only ever sees `x = x + 1`.
    bool found = false;
    for (const auto& stmt : r.statements)
    {
        if (stmt->kind == StmtKind::Let && stmt->declares == false && stmt->value)
        {
            found = stmt->value->kind == ExprKind::BinaryOp && stmt->value->op == "+";
        }
    }
    QVERIFY(found);
}

void TestObScriptCatalog::testResolvesKnownScript()
{
    const ParseResult r = parse(QStringLiteral(
        "ScriptName S extends Quest\n"
        "MyOtherScript Property ref Auto\n"));
    QVERIFY(r.ok);
    QVector<ScriptInfo> known;
    ScriptInfo info;
    info.name = QStringLiteral("MyOtherScript");
    info.functionNames << QStringLiteral("DoThing");
    known.append(info);
    const ResolveResult res = resolveProgram(r, known);
    QVERIFY(res.ok);
    QVERIFY(res.diagnostics.isEmpty());
}

void TestObScriptCatalog::testFlagsMissingScript()
{
    const ParseResult r = parse(QStringLiteral(
        "ScriptName S extends Quest\n"
        "MyOtherScript Property ref Auto\n"));
    QVERIFY(r.ok);
    const ResolveResult res = resolveProgram(r, {});
    QCOMPARE(int(res.diagnostics.size()), 1);
    QCOMPARE(res.diagnostics.at(0).severity, ResolveSeverity::Warning);
    QVERIFY(res.unresolvedScripts.contains(QStringLiteral("MyOtherScript")));
    // A missing script is a warning, not an error: it may live in a master
    // file the caller did not hand over.
    QVERIFY(res.ok);
}

void TestObScriptCatalog::testPrimitivePropertiesAreSilent()
{
    const ParseResult r = parse(QStringLiteral(
        "ScriptName S extends Quest\n"
        "int Property count Auto\n"
        "float Property waitTime = 2.0\n"
        "Actor Property who Auto\n"));
    QVERIFY(r.ok);
    const ResolveResult res = resolveProgram(r, {});
    QVERIFY(res.diagnostics.isEmpty());
}

QTEST_MAIN(TestObScriptCatalog)
#include "test_obscriptcatalog.moc"

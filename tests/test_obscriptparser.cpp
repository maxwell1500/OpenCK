#include <QTest>

#include "obscriptparser.hpp"

using namespace ObScript;

class TestObScriptParser : public QObject
{
    Q_OBJECT

private slots:
    void testFunctionWithReturn();
    void testIfElseIfElse();
    void testWhile();
    void testFor();
    void testPrecedence();
    void testCallAndPostfix();
    void testAssignmentAndLet();
    void testBeginEndWrapper();
    void testUnaryAndLogical();
    void testMissingEndif();
    void testMismatchedParen();
    void testUnexpectedChar();
    void testEmptyWrapper();
};

static const Statement* onlyStatement(const ParseResult& r)
{
    if (r.statements.size() != 1)
    {
        return nullptr;
    }
    return r.statements.front().get();
}

void TestObScriptParser::testFunctionWithReturn()
{
    ParseResult r = parse("function test()\n"
                          "    return 5\n"
                          "endfunction\n");
    QVERIFY2(r.ok, qPrintable(r.error));
    QVERIFY(r.statements.size() == 1);
    const Statement& s = *r.statements.front();
    QCOMPARE(s.kind, StmtKind::Function);
    QCOMPARE(s.funcName, QString("test"));
    QCOMPARE(s.params.size(), 0);
    QCOMPARE(s.body.size(), 1);
    QCOMPARE(s.body.front()->kind, StmtKind::Return);
    QCOMPARE(s.body.front()->value->kind, ExprKind::LiteralInt);
    QCOMPARE(s.body.front()->value->intValue, 5);
}

void TestObScriptParser::testIfElseIfElse()
{
    ParseResult r = parse("if (x > 5)\n"
                          "    a = 1\n"
                          "elseif (x > 2)\n"
                          "    b = 2\n"
                          "else\n"
                          "    c = 3\n"
                          "endif\n");
    QVERIFY2(r.ok, qPrintable(r.error));
    const Statement* sp = onlyStatement(r);
    QVERIFY(sp != nullptr);
    const Statement& s = *sp;
    QCOMPARE(s.kind, StmtKind::If);
    QCOMPARE(s.condition->kind, ExprKind::BinaryOp);
    QCOMPARE(s.condition->op, QString(">"));
    QCOMPARE(s.body.size(), 1);
    QCOMPARE(s.body.front()->kind, StmtKind::Let);
    QCOMPARE(s.branches.size(), 1);
    QCOMPARE(s.branches.front().first->op, QString(">"));
    QCOMPARE(s.branches.front().second.size(), 1);
    QCOMPARE(s.elseBody.size(), 1);
    QCOMPARE(s.elseBody.front()->kind, StmtKind::Let);
}

void TestObScriptParser::testWhile()
{
    ParseResult r = parse("while (i < 10)\n"
                          "    i = i + 1\n"
                          "endwhile\n");
    QVERIFY2(r.ok, qPrintable(r.error));
    const Statement* sp = onlyStatement(r);
    QVERIFY(sp != nullptr);
    const Statement& s = *sp;
    QCOMPARE(s.kind, StmtKind::While);
    QCOMPARE(s.condition->kind, ExprKind::BinaryOp);
    QCOMPARE(s.condition->op, QString("<"));
    QCOMPARE(s.body.size(), 1);
    QCOMPARE(s.body.front()->kind, StmtKind::Let);
    QCOMPARE(s.body.front()->lhs->kind, ExprKind::Identifier);
    QCOMPARE(s.body.front()->lhs->name, QString("i"));
    QCOMPARE(s.body.front()->value->kind, ExprKind::BinaryOp);
    QCOMPARE(s.body.front()->value->op, QString("+"));
}

void TestObScriptParser::testFor()
{
    ParseResult r = parse("for i = 0 to 10\n"
                          "    x = i\n"
                          "endfor\n");
    QVERIFY2(r.ok, qPrintable(r.error));
    const Statement* sp = onlyStatement(r);
    QVERIFY(sp != nullptr);
    const Statement& s = *sp;
    QCOMPARE(s.kind, StmtKind::For);
    QCOMPARE(s.forVar, QString("i"));
    QCOMPARE(s.forInit->kind, ExprKind::LiteralInt);
    QCOMPARE(s.forInit->intValue, 0);
    QCOMPARE(s.forEnd->kind, ExprKind::LiteralInt);
    QCOMPARE(s.forEnd->intValue, 10);
    QCOMPARE(s.body.size(), 1);
    QCOMPARE(s.body.front()->kind, StmtKind::Let);
}

void TestObScriptParser::testPrecedence()
{
    // a + b * c  =>  +(a, *(b, c))
    ParseResult r = parse("x = a + b * c\n");
    QVERIFY2(r.ok, qPrintable(r.error));
    const Statement& s = *r.statements.front();
    QCOMPARE(s.kind, StmtKind::Let);
    QCOMPARE(s.value->kind, ExprKind::BinaryOp);
    QCOMPARE(s.value->op, QString("+"));
    QCOMPARE(s.value->left->kind, ExprKind::Identifier);
    QCOMPARE(s.value->right->kind, ExprKind::BinaryOp);
    QCOMPARE(s.value->right->op, QString("*"));

    // (a + b) * c  =>  *(+(a,b), c)
    ParseResult r2 = parse("x = (a + b) * c\n");
    QVERIFY2(r2.ok, qPrintable(r2.error));
    const Statement& s2 = *r2.statements.front();
    QCOMPARE(s2.value->kind, ExprKind::BinaryOp);
    QCOMPARE(s2.value->op, QString("*"));
    QCOMPARE(s2.value->left->kind, ExprKind::BinaryOp);
    QCOMPARE(s2.value->left->op, QString("+"));
    QCOMPARE(s2.value->right->kind, ExprKind::Identifier);
}

void TestObScriptParser::testCallAndPostfix()
{
    ParseResult r = parse("foo(1, 2)\n");
    QVERIFY2(r.ok, qPrintable(r.error));
    const Statement& s = *r.statements.front();
    QCOMPARE(s.kind, StmtKind::ExprStmt);
    QCOMPARE(s.value->kind, ExprKind::Call);
    QCOMPARE(s.value->callee->name, QString("foo"));
    QCOMPARE(s.value->args.size(), 2);
    QCOMPARE(s.value->args.front()->kind, ExprKind::LiteralInt);
    QCOMPARE(s.value->args.at(1)->intValue, 2);

    // obj.field[0]
    ParseResult r2 = parse("y = obj.field[0]\n");
    QVERIFY2(r2.ok, qPrintable(r2.error));
    const Statement& s2 = *r2.statements.front();
    QCOMPARE(s2.value->kind, ExprKind::Index);
    QCOMPARE(s2.value->base->kind, ExprKind::FieldAccess);
    QCOMPARE(s2.value->base->name, QString("field"));
    QCOMPARE(s2.value->base->base->kind, ExprKind::Identifier);
    QCOMPARE(s2.value->base->base->name, QString("obj"));
    QCOMPARE(s2.value->index->kind, ExprKind::LiteralInt);
}

void TestObScriptParser::testAssignmentAndLet()
{
    ParseResult r = parse("x = 10\n");
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(r.statements.front()->kind, StmtKind::Let);
    QCOMPARE(r.statements.front()->lhs->name, QString("x"));
    QCOMPARE(r.statements.front()->value->intValue, 10);

    ParseResult r2 = parse("let float y = 2.5\n");
    QVERIFY2(r2.ok, qPrintable(r2.error));
    QCOMPARE(r2.statements.front()->kind, StmtKind::Let);
    QCOMPARE(r2.statements.front()->lhs->name, QString("y"));
    QCOMPARE(r2.statements.front()->value->kind, ExprKind::LiteralFloat);
    QCOMPARE(r2.statements.front()->value->floatValue, 2.5);
}

void TestObScriptParser::testBeginEndWrapper()
{
    ParseResult r = parse("begin MyScript\n"
                          "    x = 1\n"
                          "end\n");
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(r.statements.size(), 1);
    QCOMPARE(r.statements.front()->kind, StmtKind::Let);
    QCOMPARE(r.statements.front()->lhs->name, QString("x"));
}

void TestObScriptParser::testUnaryAndLogical()
{
    ParseResult r = parse("x = -a && !b\n");
    QVERIFY2(r.ok, qPrintable(r.error));
    const Statement& s = *r.statements.front();
    // && binds looser than unary; right operand is the unary !b.
    QCOMPARE(s.value->kind, ExprKind::BinaryOp);
    QCOMPARE(s.value->op, QString("&&"));
    QCOMPARE(s.value->left->kind, ExprKind::UnaryOp);
    QCOMPARE(s.value->left->op, QString("-"));
    QCOMPARE(s.value->left->operand->name, QString("a"));
    QCOMPARE(s.value->right->kind, ExprKind::UnaryOp);
    QCOMPARE(s.value->right->op, QString("!"));
    QCOMPARE(s.value->right->operand->name, QString("b"));

    // a && b || c  =>  || (&&(a,b), c)
    ParseResult r2 = parse("x = a && b || c\n");
    QVERIFY2(r2.ok, qPrintable(r2.error));
    QCOMPARE(r2.statements.front()->value->op, QString("||"));
    QCOMPARE(r2.statements.front()->value->left->op, QString("&&"));
}

void TestObScriptParser::testMissingEndif()
{
    ParseResult r = parse("if (x > 5)\n"
                          "    a = 1\n");
    QVERIFY(!r.ok);
    QVERIFY(r.error.contains("endif"));
}

void TestObScriptParser::testMismatchedParen()
{
    ParseResult r = parse("if (x > 5\n"
                          "    a = 1\n"
                          "endif\n");
    QVERIFY(!r.ok);
    QVERIFY(r.error.contains(")"));
}

void TestObScriptParser::testUnexpectedChar()
{
    ParseResult r = parse("x = 1 + @\n");
    QVERIFY(!r.ok);
}

void TestObScriptParser::testEmptyWrapper()
{
    ParseResult r = parse("begin s\n"
                          "end\n");
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(r.statements.size(), 0);
}

QTEST_MAIN(TestObScriptParser)
#include "test_obscriptparser.moc"

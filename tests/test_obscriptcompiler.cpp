#include <QtTest>
#include "../../libs/files/esm/obscriptcompiler.hpp"

using namespace ObScript;

class TestObScriptCompiler : public QObject
{
    Q_OBJECT

private slots:
    void testLiteralInt();
    void testBinaryArithmetic();
    void testIfStatement();
    void testWhileLoop();
    void testFunctionCall();
    void testComparison();
    void testLogicalOps();
    void testVariableAssignment();
};

void TestObScriptCompiler::testLiteralInt()
{
    ParseResult pr = parse("42");
    QVERIFY(pr.ok);
    BytecodeProgram prog = compile(pr);
    QVERIFY(prog.ok);
    QVERIFY(prog.code.size() >= 2); // PushInt + Halt
    QCOMPARE(prog.code[0].op, Opcode::PushInt);
    QCOMPARE(prog.code[0].intValue, 42);
}

void TestObScriptCompiler::testBinaryArithmetic()
{
    ParseResult pr = parse("3 + 4");
    QVERIFY(pr.ok);
    BytecodeProgram prog = compile(pr);
    QVERIFY(prog.ok);
    // PushInt 3, PushInt 4, Add
    QCOMPARE(prog.code[0].op, Opcode::PushInt);
    QCOMPARE(prog.code[0].intValue, 3);
    QCOMPARE(prog.code[1].op, Opcode::PushInt);
    QCOMPARE(prog.code[1].intValue, 4);
    QCOMPARE(prog.code[2].op, Opcode::Add);
}

void TestObScriptCompiler::testIfStatement()
{
    ParseResult pr = parse("if (x > 5)\n  y = 10\nendif\n");
    QVERIFY(pr.ok);
    BytecodeProgram prog = compile(pr);
    QVERIFY(prog.ok);
    // Should contain LoadVar (x), PushInt 5, Gt, Jz
    bool hasJz = false;
    for (const auto& instr : prog.code)
    {
        if (instr.op == Opcode::Jz)
        {
            hasJz = true;
            break;
        }
    }
    QVERIFY(hasJz);
}

void TestObScriptCompiler::testWhileLoop()
{
    ParseResult pr = parse("while (i < 10)\n  i = i + 1\nendwhile\n");
    QVERIFY(pr.ok);
    BytecodeProgram prog = compile(pr);
    QVERIFY(prog.ok);
    // Should contain Jmp (back jump)
    bool hasJmp = false;
    for (const auto& instr : prog.code)
    {
        if (instr.op == Opcode::Jmp)
        {
            hasJmp = true;
            break;
        }
    }
    QVERIFY(hasJmp);
}

void TestObScriptCompiler::testFunctionCall()
{
    ParseResult pr = parse("print(\"hello\")");
    QVERIFY(pr.ok);
    BytecodeProgram prog = compile(pr);
    QVERIFY(prog.ok);
    // Should contain Call with arg count 1
    bool hasCall = false;
    for (const auto& instr : prog.code)
    {
        if (instr.op == Opcode::Call)
        {
            hasCall = true;
            QCOMPARE(instr.operand, 1);
            break;
        }
    }
    QVERIFY(hasCall);
}

void TestObScriptCompiler::testComparison()
{
    ParseResult pr = parse("a < b");
    QVERIFY(pr.ok);
    BytecodeProgram prog = compile(pr);
    QVERIFY(prog.ok);
    // LoadVar a, LoadVar b, Lt
    QCOMPARE(prog.code[0].op, Opcode::LoadVar);
    QCOMPARE(prog.code[1].op, Opcode::LoadVar);
    QCOMPARE(prog.code[2].op, Opcode::Lt);
}

void TestObScriptCompiler::testLogicalOps()
{
    ParseResult pr = parse("a && b");
    QVERIFY(pr.ok);
    BytecodeProgram prog = compile(pr);
    QVERIFY(prog.ok);
    QCOMPARE(prog.code[0].op, Opcode::LoadVar);
    QCOMPARE(prog.code[1].op, Opcode::LoadVar);
    QCOMPARE(prog.code[2].op, Opcode::And);
}

void TestObScriptCompiler::testVariableAssignment()
{
    ParseResult pr = parse("let x = 5");
    QVERIFY(pr.ok);
    BytecodeProgram prog = compile(pr);
    QVERIFY(prog.ok);
    // PushInt 5, StoreVar (x)
    bool hasStore = false;
    for (const auto& instr : prog.code)
    {
        if (instr.op == Opcode::StoreVar)
        {
            hasStore = true;
            break;
        }
    }
    QVERIFY(hasStore);
}

QTEST_MAIN(TestObScriptCompiler)
#include "test_obscriptcompiler.moc"

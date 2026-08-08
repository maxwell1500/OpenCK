#include <QtTest>

#include "../../src/model/tools/undostack.hpp"
#include "../../src/model/tools/command.hpp"
#include "../../src/model/tools/macrocommand.hpp"

class TestUndoStack : public QObject
{
    Q_OBJECT

private slots:
    void testPushUndo();
    void testPushRedo();
    void testCanUndoRedo();
    void testMaxDepth();
    void testClear();
    void testMacroCommand();
};

class StubCommand : public Command
{
public:
    StubCommand(const QString& n) : mName(n), mExecuteCount(0), mUndoCount(0) {}

    void execute() override { mExecuteCount++; }
    void undo() override { mUndoCount++; }
    QString name() const override { return mName; }

    int executeCount() const { return mExecuteCount; }
    int undoCount() const { return mUndoCount; }

private:
    QString mName;
    int mExecuteCount;
    int mUndoCount;
};

void TestUndoStack::testPushUndo()
{
    UndoStack stack;
    stack.push(new StubCommand("cmd1"));

    QVERIFY(stack.canUndo());
    QCOMPARE(stack.undoCount(), 1);

    stack.undo();

    QVERIFY(!stack.canUndo());
    QCOMPARE(stack.undoCount(), 0);
}

void TestUndoStack::testPushRedo()
{
    UndoStack stack;
    StubCommand* cmd = new StubCommand("cmd1");
    stack.push(cmd);

    stack.undo();
    QVERIFY(stack.canRedo());

    stack.redo();
    QCOMPARE(cmd->executeCount(), 1);
    QVERIFY(!stack.canRedo());
}

void TestUndoStack::testCanUndoRedo()
{
    UndoStack stack;

    QVERIFY(!stack.canUndo());
    QVERIFY(!stack.canRedo());

    stack.push(new StubCommand("cmd1"));
    QVERIFY(stack.canUndo());
    QVERIFY(!stack.canRedo());

    stack.push(new StubCommand("cmd2"));
    QVERIFY(stack.canUndo());
    QVERIFY(!stack.canRedo());

    stack.undo();
    QVERIFY(stack.canUndo());
    QVERIFY(stack.canRedo());

    stack.undo();
    QVERIFY(!stack.canUndo());
    QVERIFY(stack.canRedo());
}

void TestUndoStack::testMaxDepth()
{
    UndoStack stack(3);

    stack.push(new StubCommand("cmd1"));
    stack.push(new StubCommand("cmd2"));
    stack.push(new StubCommand("cmd3"));
    QCOMPARE(stack.undoCount(), 3);

    stack.push(new StubCommand("cmd4"));
    QCOMPARE(stack.undoCount(), 3);

    stack.push(new StubCommand("cmd5"));
    QCOMPARE(stack.undoCount(), 3);

    QCOMPARE(stack.currentDescription(), QString("cmd5"));
}

void TestUndoStack::testClear()
{
    UndoStack stack;
    stack.push(new StubCommand("cmd1"));
    stack.push(new StubCommand("cmd2"));

    QVERIFY(stack.canUndo());

    stack.clear();

    QVERIFY(!stack.canUndo());
    QVERIFY(!stack.canRedo());
    QCOMPARE(stack.undoCount(), 0);
    QCOMPARE(stack.redoCount(), 0);
}

void TestUndoStack::testMacroCommand()
{
    UndoStack stack;
    MacroCommand* macro = new MacroCommand("macro1");
    StubCommand* a = new StubCommand("cmdA");
    StubCommand* b = new StubCommand("cmdB");
    macro->addCommand(a);
    macro->addCommand(b);

    stack.push(macro);

    QCOMPARE(stack.currentDescription(), QString("macro1"));

    stack.undo();
    QCOMPARE(a->undoCount(), 1);
    QCOMPARE(b->undoCount(), 1);

    stack.redo();
    QCOMPARE(a->executeCount(), 1);
    QCOMPARE(b->executeCount(), 1);
}

QTEST_MAIN(TestUndoStack)
#include "test_undostack.moc"

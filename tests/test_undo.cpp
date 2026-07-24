#include <QtTest>
#include <QRegularExpression>

#include "../../model/tools/undostack.hpp"
#include "../../model/tools/command.hpp"
#include "../../model/tools/macrocommand.hpp"

class TestUndoStack : public QObject
{
    Q_OBJECT

private slots:
    void testBasicPushUndoRedo();
    void testCanUndoCanRedo();
    void testMacroCommand();
};

class SimpleCommand : public Command
{
public:
    SimpleCommand(int* counter, int increment)
        : mCounter(counter), mIncrement(increment) {}
    
    void execute() override { if (mCounter) *mCounter += mIncrement; }
    void undo() override { if (mCounter) *mCounter -= mIncrement; }
    QString name() const override { return "SimpleCommand"; }

private:
    int* mCounter;
    int mIncrement;
};

void TestUndoStack::testBasicPushUndoRedo()
{
    int counter = 0;
    UndoStack stack;
    
    stack.push(new SimpleCommand(&counter, 5));
    stack.push(new SimpleCommand(&counter, 3));
    
    QCOMPARE(counter, 8);
    
    stack.undo();
    QCOMPARE(counter, 5);
    
    stack.undo();
    QCOMPARE(counter, 0);
    
    stack.redo();
    QCOMPARE(counter, 5);
    
    stack.redo();
    QCOMPARE(counter, 8);
}

void TestUndoStack::testCanUndoCanRedo()
{
    UndoStack stack;
    int counter = 0;
    
    QVERIFY(!stack.canUndo());
    QVERIFY(!stack.canRedo());
    QCOMPARE(stack.undoCount(), 0);
    QCOMPARE(stack.redoCount(), 0);
    
    stack.push(new SimpleCommand(&counter, 1));
    
    QVERIFY(stack.canUndo());
    QVERIFY(!stack.canRedo());
    QCOMPARE(stack.undoCount(), 1);
    QCOMPARE(stack.redoCount(), 0);
    
    stack.undo();
    QVERIFY(!stack.canUndo());
    QVERIFY(stack.canRedo());
    QCOMPARE(stack.undoCount(), 0);
    QCOMPARE(stack.redoCount(), 1);
}

void TestUndoStack::testMacroCommand()
{
    MacroCommand macro("Batch operation");
    int counter = 0;
    
    macro.addCommand(new SimpleCommand(&counter, 5));
    macro.addCommand(new SimpleCommand(&counter, 3));
    macro.addCommand(new SimpleCommand(&counter, 2));
    
    QCOMPARE(counter, 0);
    
    macro.execute();
    QCOMPARE(counter, 10);
    
    macro.undo();
    QCOMPARE(counter, 0);
    
    QCOMPARE(macro.name(), QString("Batch operation"));
}

QTEST_MAIN(TestUndoStack)
#include "test_undo.moc"

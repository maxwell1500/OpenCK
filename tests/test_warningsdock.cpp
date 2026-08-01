#include <QtTest>
#include <QApplication>
#include <QTableWidget>

#include "../../src/view/window/warningsdockwidget.hpp"
#include "../../src/model/doc/messages.hpp"

class TestWarningsDock : public QObject
{
    Q_OBJECT

private slots:
    void testStartsEmpty();
    void testAddMessage();
    void testSetMessages();
    void testClear();
};

void TestWarningsDock::testStartsEmpty()
{
    WarningsDockWidget dock;
    QCOMPARE(dock.count(), 0);
}

void TestWarningsDock::testAddMessage()
{
    WarningsDockWidget dock;
    dock.addMessage(Message(CkId(CkId::Type_Npc_, "TestNPC"),
        "NPC has an empty EditorID.", "", Message::Error));
    QCOMPARE(dock.count(), 1);

    QTableWidget* table = dock.findChild<QTableWidget*>();
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QString("Error"));
    QCOMPARE(table->item(0, 1)->text(), QString("NPC has an empty EditorID."));
    QCOMPARE(table->item(0, 2)->text(), QString("TestNPC"));
}

void TestWarningsDock::testSetMessages()
{
    Messages messages(Message::Default);
    messages.append(CkId(CkId::Type_Weap_, "IronSword"), "Damage is out of range.", "", Message::Error);
    messages.append(CkId(CkId::Type_Npc_, "Guard"), "Faction reference is dangling.", "", Message::Warning);

    WarningsDockWidget dock;
    dock.setMessages(messages);
    QCOMPARE(dock.count(), 2);

    QTableWidget* table = dock.findChild<QTableWidget*>();
    QVERIFY(table);
    QCOMPARE(table->item(0, 0)->text(), QString("Error"));
    QCOMPARE(table->item(1, 0)->text(), QString("Warning"));
}

void TestWarningsDock::testClear()
{
    WarningsDockWidget dock;
    dock.addMessage(Message(CkId(CkId::Type_Npc_, "TestNPC"), "msg", "", Message::Info));
    QCOMPARE(dock.count(), 1);
    dock.clear();
    QCOMPARE(dock.count(), 0);
}

QTEST_MAIN(TestWarningsDock)
#include "test_warningsdock.moc"

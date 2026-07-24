#include <QtTest>

#include "../../src/model/tools/editrecordcommand.hpp"
#include "../../src/model/world/collection.hpp"
#include "../../src/model/world/record.hpp"
#include "../../libs/files/esm/npcrecord.hpp"

class TestEditRecordCommand : public QObject
{
    Q_OBJECT

private slots:
    void testHasChanged_True();
    void testHasChanged_False();
    void testExecute();
    void testUndo();
    void testExecuteUndoRoundTrip();
    void testName_Default();
    void testName_Custom();
};

void TestEditRecordCommand::testHasChanged_True()
{
    NpcRecord original;
    original.editorId = "TestNPC";
    original.level = 10;

    NpcRecord modified;
    modified.editorId = "TestNPC";
    modified.level = 20;

    EditRecordCommand<NpcRecord> cmd(nullptr, 0, original, modified);

    QVERIFY(cmd.hasChanged());
}

void TestEditRecordCommand::testHasChanged_False()
{
    NpcRecord original;
    original.editorId = "TestNPC";
    original.level = 10;

    NpcRecord same;
    same.editorId = "TestNPC";
    same.level = 10;

    EditRecordCommand<NpcRecord> cmd(nullptr, 0, original, same);

    QVERIFY(!cmd.hasChanged());
}

void TestEditRecordCommand::testExecute()
{
    Collection<NpcRecord> collection;

    NpcRecord npc;
    npc.editorId = "TestNPC";
    npc.level = 10;
    collection.add(npc);

    NpcRecord modified;
    modified.editorId = "TestNPC";
    modified.level = 25;

    EditRecordCommand<NpcRecord> cmd(&collection, 0, npc, modified);
    cmd.execute();

    QCOMPARE(collection.getRecord(0).get().level, 25u);
}

void TestEditRecordCommand::testUndo()
{
    Collection<NpcRecord> collection;

    NpcRecord npc;
    npc.editorId = "TestNPC";
    npc.level = 10;
    collection.add(npc);

    NpcRecord modified;
    modified.editorId = "TestNPC";
    modified.level = 25;

    EditRecordCommand<NpcRecord> cmd(&collection, 0, npc, modified);
    cmd.execute();
    cmd.undo();

    QCOMPARE(collection.getRecord(0).get().level, 10u);
}

void TestEditRecordCommand::testExecuteUndoRoundTrip()
{
    Collection<NpcRecord> collection;

    NpcRecord npc;
    npc.editorId = "TestNPC";
    npc.level = 10;
    npc.health = 100;
    collection.add(npc);

    NpcRecord modified;
    modified.editorId = "TestNPC";
    modified.level = 99;
    modified.health = 999;

    EditRecordCommand<NpcRecord> cmd(&collection, 0, npc, modified);
    cmd.execute();
    cmd.undo();

    QCOMPARE(collection.getRecord(0).get().editorId, QString("TestNPC"));
    QCOMPARE(collection.getRecord(0).get().level, 10u);
    QCOMPARE(collection.getRecord(0).get().health, 100u);
}

void TestEditRecordCommand::testName_Default()
{
    NpcRecord npc;
    npc.editorId = "MyNPC";

    EditRecordCommand<NpcRecord> cmd(nullptr, 0, npc, npc);

    QCOMPARE(cmd.name(), QString("Edit record: MyNPC"));
}

void TestEditRecordCommand::testName_Custom()
{
    NpcRecord npc;
    npc.editorId = "MyNPC";

    EditRecordCommand<NpcRecord> cmd(nullptr, 0, npc, npc, "Custom description");

    QCOMPARE(cmd.name(), QString("Custom description"));
}

QTEST_MAIN(TestEditRecordCommand)
#include "test_editrecordcommand.moc"

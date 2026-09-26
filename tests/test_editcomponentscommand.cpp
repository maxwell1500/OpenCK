#include <QtTest>

#include "../../src/model/tools/editcomponentscommand.hpp"
#include "../../src/model/tools/undostack.hpp"
#include "../../src/model/world/collection.hpp"
#include "../../src/model/world/record.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/components/tesfullname.hpp"

// EditComponentsCommand is the type-erased replacement used by edit routes
// that only hold a BaseCollection — notably the Morrowind branch of
// ObjectWindowDialog::editSelected(), which previously passed an empty commit
// callback and let the dialog overwrite the live record's components with no
// undo entry at all. These tests pin the two properties that route depends on:
// nothing is written until execute(), and undo puts the original back.
class TestEditComponentsCommand : public QObject
{
    Q_OBJECT

private:
    // NpcRecord::initComponents() already seeds a TESFullName, so look the
    // component up before adding another one — findByName returns the first
    // match and a duplicate would read back empty.
    static void setName(openck::FormComponents& comps, const QString& value)
    {
        auto* found = comps.findByName(QStringLiteral("TESFullName"));
        if (auto* full = static_cast<tescomponents::TESFullName_Component*>(found))
            full->fullName = value;
        else
            comps.add<tescomponents::TESFullName_Component>()->fullName = value;
    }

    static QString nameOf(const Collection<NpcRecord>& c, int index)
    {
        const openck::FormComponents& comps = c.getRecord(index).get().components;
        const auto* found = comps.findByName(QStringLiteral("TESFullName"));
        const auto* full = static_cast<const tescomponents::TESFullName_Component*>(found);
        return full ? full->fullName : QString();
    }

    static NpcRecord makeRecord(const QString& editorId, const QString& name)
    {
        NpcRecord rec;
        rec.editorId = editorId;
        rec.initComponents();
        setName(rec.components, name);
        return rec;
    }

    // The "user typed a new name" working set the dialog would hand us.
    static openck::FormComponents renamed(const Collection<NpcRecord>& c, int index,
                                          const QString& value)
    {
        openck::FormComponents edited = c.getRecord(index).get().components;
        setName(edited, value);
        return edited;
    }

private slots:
    void activeComponentsReachesTheLiveRecord()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));
        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));

        auto* comps = collection.getRecord(0).activeComponents();
        QVERIFY(comps != nullptr);
        setName(*comps, QStringLiteral("After"));
        QCOMPARE(nameOf(collection, 0), QStringLiteral("After"));
    }

    void captureBeforeRejectsAnUnchangedEdit()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Same")));

        EditComponentsCommand cmd(&collection, 0, collection.getRecord(0).get().components);
        QVERIFY(!cmd.captureBefore());
        QVERIFY(!cmd.hasChanged());
    }

    void captureBeforeRejectsAnOutOfRangeIndex()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Same")));

        openck::FormComponents edited = renamed(collection, 0, QStringLiteral("X"));

        EditComponentsCommand cmd(&collection, 5, edited);
        QVERIFY(!cmd.captureBefore());
    }

    void nothingIsWrittenBeforeExecute()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));

        openck::FormComponents edited = renamed(collection, 0, QStringLiteral("After"));

        const int stateBefore = collection.getRecord(0).state;

        EditComponentsCommand cmd(&collection, 0, edited);
        QVERIFY(cmd.captureBefore());
        QVERIFY(cmd.hasChanged());

        // Constructing and capturing must not have touched the record: this is
        // the whole point of the session, since the dialog holds the edit for
        // as long as the user leaves it open.
        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));
        QCOMPARE(int(collection.getRecord(0).state), stateBefore);
    }

    void executeThenUndoRoundTrips()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));

        openck::FormComponents edited = renamed(collection, 0, QStringLiteral("After"));

        EditComponentsCommand cmd(&collection, 0, edited);
        QVERIFY(cmd.captureBefore());

        cmd.execute();
        QCOMPARE(nameOf(collection, 0), QStringLiteral("After"));
        QVERIFY(collection.getRecord(0).isModified());

        cmd.undo();
        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));
    }

    void undoStackPushUndoRedo()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));

        openck::FormComponents edited = renamed(collection, 0, QStringLiteral("After"));

        UndoStack stack;
        auto* cmd = new EditComponentsCommand(&collection, 0, edited,
            QStringLiteral("Edit NPC_"));
        QVERIFY(cmd->captureBefore());
        stack.push(cmd);
        QCOMPARE(stack.undoCount(), 1);
        QCOMPARE(nameOf(collection, 0), QStringLiteral("After"));

        stack.undo();
        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));
        QCOMPARE(stack.redoCount(), 1);

        stack.redo();
        QCOMPARE(nameOf(collection, 0), QStringLiteral("After"));
    }

    void anAbandonedEditLeavesTheRecordUntouched()
    {
        // The cancel case: the dialog built a command, the user hit Cancel, so
        // captureBefore() ran but nothing was ever pushed or executed.
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));

        openck::FormComponents edited = renamed(collection, 0, QStringLiteral("After"));

        const int stateBefore = collection.getRecord(0).state;

        EditComponentsCommand cmd(&collection, 0, edited);
        QVERIFY(cmd.captureBefore());

        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));
        QCOMPARE(int(collection.getRecord(0).state), stateBefore);
    }
};

QTEST_MAIN(TestEditComponentsCommand)
#include "test_editcomponentscommand.moc"

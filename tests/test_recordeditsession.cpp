#include <QtTest>

#include "../../src/model/tools/undostack.hpp"
#include "../../src/view/window/recordeditsession.hpp"
#include "../../src/model/world/collection.hpp"
#include "../../src/model/world/record.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/components/tesfullname.hpp"

// A RecordEditSession owns a working copy of a record. The dialog's property
// grid and any custom data widget both edit that copy; only commit() writes the
// live record, and it always goes through the undo stack. These tests pin the
// three properties every edit route depends on:
//
//  * the live record is untouched while an edit is merely pending (Cancel),
//  * a widget writing plain record fields — not just components — lands in the
//    working copy rather than the base record,
//  * OK produces exactly one undo entry that round-trips.
class TestRecordEditSession : public QObject
{
    Q_OBJECT

private:
    // NpcRecord::initComponents() already seeds a TESFullName, so look the
    // component up before adding another one; findByName returns the first
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

    using Session = openck::TypedRecordEditSession<NpcRecord>;

private slots:
    void aFreshSessionHasNoChanges()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));

        Session session(&collection, 0, nullptr, QStringLiteral("Edit NPC_"));
        QVERIFY(!session.hasChanges());
        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));
    }

    void workingComponentsPointAtTheWorkingCopy()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));

        Session session(&collection, 0, nullptr, QStringLiteral("Edit NPC_"));
        QVERIFY(session.workingComponents() != nullptr);
        // The whole point: these are not the live record's components.
        QVERIFY(session.workingComponents()
                != &collection.getRecord(0).get().components);

        setName(*session.workingComponents(), QStringLiteral("After"));
        QVERIFY(session.hasChanges());
        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));
    }

    void aPendingEditNeverTouchesTheLiveRecord()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));
        const int stateBefore = collection.getRecord(0).state;
        const int levelBefore = collection.getRecord(0).get().level;

        Session session(&collection, 0, nullptr, QStringLiteral("Edit NPC_"));
        // Both the kinds of edit a dialog can make: a component property and a
        // plain record field a custom data widget would write.
        setName(*session.workingComponents(), QStringLiteral("After"));
        static_cast<NpcRecord*>(session.workingRecord())->level = 42;

        QVERIFY(session.hasChanges());
        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));
        QCOMPARE(collection.getRecord(0).get().level, levelBefore);
        QCOMPARE(int(collection.getRecord(0).state), stateBefore);
    }

    void discardDropsAPendingEdit()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));

        Session session(&collection, 0, nullptr, QStringLiteral("Edit NPC_"));
        setName(*session.workingComponents(), QStringLiteral("After"));
        static_cast<NpcRecord*>(session.workingRecord())->level = 42;

        session.discard();
        QVERIFY(session.isFinished());
        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));
    }

    void commitWritesBothTheComponentsAndThePlainFields()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));
        UndoStack stack;

        Session session(&collection, 0, &stack, QStringLiteral("Edit NPC_"));
        setName(*session.workingComponents(), QStringLiteral("After"));
        static_cast<NpcRecord*>(session.workingRecord())->level = 42;

        QVERIFY(session.commit());
        QCOMPARE(nameOf(collection, 0), QStringLiteral("After"));
        QCOMPARE(collection.getRecord(0).get().level, 42);
        QCOMPARE(stack.undoCount(), 1);
    }

    void commitIsIdempotentAndSkipsAnUnchangedEdit()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));
        UndoStack stack;

        Session unchanged(&collection, 0, &stack, QStringLiteral("Edit NPC_"));
        QVERIFY(!unchanged.commit());
        QCOMPARE(stack.undoCount(), 0);

        Session session(&collection, 0, &stack, QStringLiteral("Edit NPC_"));
        setName(*session.workingComponents(), QStringLiteral("After"));
        QVERIFY(session.commit());
        QCOMPARE(stack.undoCount(), 1);
        // A second commit must not push a duplicate undo entry.
        QVERIFY(!session.commit());
        QCOMPARE(stack.undoCount(), 1);
    }

    void undoRedoRoundTripRestoresEverything()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));
        UndoStack stack;

        Session session(&collection, 0, &stack, QStringLiteral("Edit NPC_"));
        setName(*session.workingComponents(), QStringLiteral("After"));
        static_cast<NpcRecord*>(session.workingRecord())->level = 42;
        QVERIFY(session.commit());

        stack.undo();
        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));
        QCOMPARE(collection.getRecord(0).get().level, 0);

        stack.redo();
        QCOMPARE(nameOf(collection, 0), QStringLiteral("After"));
        QCOMPARE(collection.getRecord(0).get().level, 42);
    }

    void commitWithoutAnUndoStackStillWrites()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));

        Session session(&collection, 0, nullptr, QStringLiteral("Edit NPC_"));
        setName(*session.workingComponents(), QStringLiteral("After"));
        QVERIFY(session.commit());
        QCOMPARE(nameOf(collection, 0), QStringLiteral("After"));
    }

    void anOutOfRangeSessionIsHarmless()
    {
        Collection<NpcRecord> collection;
        collection.add(makeRecord(QStringLiteral("TestNPC"), QStringLiteral("Before")));

        Session session(&collection, 7, nullptr, QStringLiteral("Edit NPC_"));
        QVERIFY(!session.hasChanges());
        QVERIFY(!session.commit());
        QCOMPARE(nameOf(collection, 0), QStringLiteral("Before"));
    }
};

QTEST_MAIN(TestRecordEditSession)
#include "test_recordeditsession.moc"

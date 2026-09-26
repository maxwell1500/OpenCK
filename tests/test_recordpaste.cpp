// Tests for the generic paste path.
//
// Paste used to run ~30 hand-written per-type branches that rebuilt a record
// from a JSON field map — copying only the handful of fields each branch
// happened to list — and then added it through a type-specific mData->addXxx()
// that never touched the undo stack. It now duplicates the whole record through
// AddRecordCommand, so these tests pin both halves: every field travels, and
// the paste is undoable.

#include <QtTest>

#include "../../src/model/tools/addrecordcommand.hpp"
#include "../../src/model/tools/recordpaste.hpp"
#include "../../src/model/tools/undostack.hpp"
#include "../../src/model/world/collection.hpp"
#include "../../src/model/world/idtable.hpp"
#include "../../src/model/world/record.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/esm/racerecord.hpp"
#include "../../libs/files/esm/sounrecord.hpp"
#include "../../libs/files/esm/wthrrecord.hpp"
#include "../../libs/components/tesfullname.hpp"

using openck::addRecordCopyThroughUndo;
using openck::copyRecordForPaste;

class TestRecordPaste : public QObject
{
    Q_OBJECT

private:
    static void setName(openck::FormComponents& comps, const QString& value)
    {
        auto* found = comps.findByName(QStringLiteral("TESFullName"));
        if (auto* full = static_cast<tescomponents::TESFullName_Component*>(found))
            full->fullName = value;
        else
            comps.add<tescomponents::TESFullName_Component>()->fullName = value;
    }

    static QString nameOfRecord(const NpcRecord& record)
    {
        const auto* found = record.components.findByName(QStringLiteral("TESFullName"));
        const auto* full = static_cast<const tescomponents::TESFullName_Component*>(found);
        return full ? full->fullName : QString();
    }

    static QString nameOf(const Collection<NpcRecord>& c, int index)
    {
        return nameOfRecord(c.getRecord(index).get());
    }

    static NpcRecord makeNpc(const QString& editorId, const QString& fullName)
    {
        NpcRecord rec;
        rec.editorId = editorId;
        rec.formId = 0x100;
        rec.initComponents();
        setName(rec.components, fullName);
        rec.level = 42;
        rec.health = 111;
        rec.race = 0x7;
        return rec;
    }

private slots:
    // The whole record travels, not just the fields a JSON map would list.
    void copyCarriesEveryField()
    {
        Collection<NpcRecord> collection;
        collection.add(makeNpc(QStringLiteral("Source"), QStringLiteral("Long Name")));

        std::unique_ptr<BaseRecord> copy;
        QVERIFY(copyRecordForPaste(&collection, 0, QStringLiteral("Copy"), 0x200, copy));
        QVERIFY(copy);

        auto* typed = static_cast<Record<NpcRecord>*>(copy.get());
        const NpcRecord& record = typed->get();
        QCOMPARE(record.editorId, QStringLiteral("Copy"));
        QCOMPARE(record.formId, 0x200u);
        QCOMPARE(record.level, 42u);
        QCOMPARE(record.health, 111u);
        QCOMPARE(record.race, 0x7u);
        // The component set travelled too, which a JSON field map could not do.
        QCOMPARE(nameOfRecord(record), QStringLiteral("Long Name"));
        // ...and the source is untouched.
        QCOMPARE(collection.getRecord(0).get().editorId, QStringLiteral("Source"));
    }

    // The copy is independent of the source.
    void theCopyIsIndependent()
    {
        Collection<NpcRecord> collection;
        collection.add(makeNpc(QStringLiteral("Source"), QStringLiteral("Long Name")));

        std::unique_ptr<BaseRecord> copy;
        QVERIFY(copyRecordForPaste(&collection, 0, QStringLiteral("Copy"), 0x200, copy));
        auto* typed = static_cast<Record<NpcRecord>*>(copy.get());
        typed->get().level = 999;
        typed->get().editorId = QStringLiteral("Mutated");

        const NpcRecord& live = collection.getRecord(0).get();
        QCOMPARE(live.editorId, QStringLiteral("Source"));
        QCOMPARE(live.level, 42u);
    }

    // A pasted record is new, so it must not inherit the source's flags.
    void theCopyIsMarkedModifiedOnly()
    {
        Collection<NpcRecord> collection;
        collection.add(makeNpc(QStringLiteral("Source"), QStringLiteral("Long Name")));
        // Make the source look like a pristine base record.
        collection.getRecord(0).state = State_Base;

        std::unique_ptr<BaseRecord> copy;
        QVERIFY(copyRecordForPaste(&collection, 0, QStringLiteral("Copy"), 0x200, copy));
        QCOMPARE(copy->state, State(State_ModifiedOnly));
    }

    void copyRejectsAnOutOfRangeIndex()
    {
        Collection<NpcRecord> collection;
        collection.add(makeNpc(QStringLiteral("Source"), QStringLiteral("Long Name")));

        std::unique_ptr<BaseRecord> copy;
        QVERIFY(!copyRecordForPaste(&collection, 5, QStringLiteral("Copy"), 0x200, copy));
        QVERIFY(!copyRecordForPaste(&collection, -1, QStringLiteral("Copy"), 0x200, copy));
    }

    void addThroughUndoAppendsAndIsUndoable()
    {
        Collection<NpcRecord> collection;
        collection.add(makeNpc(QStringLiteral("Source"), QStringLiteral("Long Name")));
        IdTable table(&collection);
        UndoStack stack;

        QVERIFY(addRecordCopyThroughUndo(&collection, 0, QStringLiteral("Copy"), 0x200,
                                        &table, &stack, QStringLiteral("Paste record: Copy")));
        QCOMPARE(stack.undoCount(), 1);
        QCOMPARE(collection.size(), 2);
        QCOMPARE(stack.currentDescription(), QStringLiteral("Paste record: Copy"));

        const NpcRecord& pasted = collection.getRecord(collection.size() - 1).get();
        QCOMPARE(pasted.editorId, QStringLiteral("Copy"));
        QCOMPARE(pasted.formId, 0x200u);
        QCOMPARE(pasted.level, 42u);
        QCOMPARE(pasted.health, 111u);
        // The component set travelled too, which the old JSON map could not do.
        QCOMPARE(nameOf(collection, collection.size() - 1), QStringLiteral("Long Name"));

        stack.undo();
        QCOMPARE(collection.size(), 1);
        QCOMPARE(collection.getRecord(0).get().editorId, QStringLiteral("Source"));

        stack.redo();
        QCOMPARE(collection.size(), 2);
    }

    void addFailsWithoutAnUndoStack()
    {
        Collection<NpcRecord> collection;
        collection.add(makeNpc(QStringLiteral("Source"), QStringLiteral("Long Name")));
        IdTable table(&collection);

        QVERIFY(!addRecordCopyThroughUndo(&collection, 0, QStringLiteral("Copy"), 0x200,
                                         &table, nullptr));
        QCOMPARE(collection.size(), 1);
    }

    void addFailsWithoutATableModel()
    {
        Collection<NpcRecord> collection;
        collection.add(makeNpc(QStringLiteral("Source"), QStringLiteral("Long Name")));
        UndoStack stack;

        QVERIFY(!addRecordCopyThroughUndo(&collection, 0, QStringLiteral("Copy"), 0x200,
                                         nullptr, &stack));
        QCOMPARE(collection.size(), 1);
    }

    // Types other than NPC_ go through the same generic path.
    void worksForOtherRecordTypes()
    {
        {
            RaceRecord rec;
            rec.editorId = QStringLiteral("RaceA");
            rec.raceFlags = 0x20;
            rec.npcVariables = { 1u, 2u, 3u };
            Collection<RaceRecord> collection;
            collection.add(rec);
            IdTable table(&collection);
            UndoStack stack;

            QVERIFY(addRecordCopyThroughUndo(&collection, 0, QStringLiteral("RaceB"), 0x300,
                                            &table, &stack));
            QCOMPARE(collection.size(), 2);
            const RaceRecord& pasted = collection.getRecord(1).get();
            QCOMPARE(pasted.raceFlags, 0x20u);
            QCOMPARE(pasted.npcVariables.size(), 3);
        }
        {
            SounRecord rec;
            rec.editorId = QStringLiteral("SoundA");
            rec.soundFile = QStringLiteral("Fx\\a.wav");
            rec.flags = 5;
            Collection<SounRecord> collection;
            collection.add(rec);
            IdTable table(&collection);
            UndoStack stack;

            QVERIFY(addRecordCopyThroughUndo(&collection, 0, QStringLiteral("SoundB"), 0x400,
                                            &table, &stack));
            const SounRecord& pasted = collection.getRecord(1).get();
            QCOMPARE(pasted.soundFile, QStringLiteral("Fx\\a.wav"));
            QCOMPARE(pasted.flags, 5u);
        }
        {
            WthrRecord rec;
            rec.editorId = QStringLiteral("WeatherA");
            rec.sunTexture = QStringLiteral("textures\\sun.dds");
            Collection<WthrRecord> collection;
            collection.add(rec);
            IdTable table(&collection);
            UndoStack stack;

            QVERIFY(addRecordCopyThroughUndo(&collection, 0, QStringLiteral("WeatherB"), 0x500,
                                            &table, &stack));
            QCOMPARE(collection.getRecord(1).get().sunTexture,
                     QStringLiteral("textures\\sun.dds"));
        }
    }
};

QTEST_MAIN(TestRecordPaste)
#include "test_recordpaste.moc"

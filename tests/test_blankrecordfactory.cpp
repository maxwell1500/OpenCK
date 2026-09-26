// Tests for BlankRecordFactory.
//
// The factory used to be a hand-written switch naming six types, so every other
// Object Window category reported "not supported yet" from Add Record, and a
// new record type had to be remembered in two places. It now dispatches by
// dynamic_cast over the shared record-type list, so the interesting property is
// breadth: the same path must produce a usable, correctly stamped record for a
// wide spread of types, not just the original six.

#include <QtTest>

#include "../../src/model/tools/blankrecordfactory.hpp"
#include "../../src/model/tools/addrecordcommand.hpp"
#include "../../src/model/tools/undostack.hpp"
#include "../../src/model/world/collection.hpp"
#include "../../src/model/world/idtable.hpp"
#include "../../src/model/world/record.hpp"

#include "../../libs/files/esm/actirecord.hpp"
#include "../../libs/files/esm/alchrecord.hpp"
#include "../../libs/files/esm/ammorecord.hpp"
#include "../../libs/files/esm/armorrecord.hpp"
#include "../../libs/files/esm/bookrecord.hpp"
#include "../../libs/files/esm/classrecord.hpp"
#include "../../libs/files/esm/dialrecord.hpp"
#include "../../libs/files/esm/factrecord.hpp"
#include "../../libs/files/esm/glob.hpp"
#include "../../libs/files/esm/gmst.hpp"
#include "../../libs/files/esm/ingrrecord.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/esm/packagerecord.hpp"
#include "../../libs/files/esm/questrecord.hpp"
#include "../../libs/files/esm/racerecord.hpp"
#include "../../libs/files/esm/sounrecord.hpp"
#include "../../libs/files/esm/spellrecord.hpp"
#include "../../libs/files/esm/weaprecord.hpp"
#include "../../libs/files/esm/wthrrecord.hpp"
#include "../../libs/components/component.hpp"
#include "../../libs/components/tesfullname.hpp"

class TestBlankRecordFactory : public QObject
{
    Q_OBJECT

private:
    // Every type the factory claims must produce a record that is stamped,
    // blank, new, and — for component-based types — carries its component set.
    template <typename RecordType>
    void checkCreatable()
    {
        Collection<RecordType> collection;
        QVERIFY(BlankRecordFactory::supports(&collection));

        const QString editorId = QStringLiteral("NewThing");
        const quint32 formId = 0x800;
        std::unique_ptr<BaseRecord> made =
            BlankRecordFactory::create(&collection, editorId, formId);
        QVERIFY(made != nullptr);
        if (!made) return;

        QCOMPARE(made->state, State(State_ModifiedOnly));
        QVERIFY(!made->isErased());

        auto* typed = static_cast<Record<RecordType>*>(made.get());
        const RecordType& record = typed->get();
        QCOMPARE(record.editorId, editorId);
        QCOMPARE(record.formId, formId);
    }

private slots:
    void createsTheOriginalSix()
    {
        Collection<GlobalVariable> globals;   checkCreatable<GlobalVariable>();
        Collection<GameSetting> settings;    checkCreatable<GameSetting>();
        Collection<NpcRecord> npcs;           checkCreatable<NpcRecord>();
        Collection<RaceRecord> races;         checkCreatable<RaceRecord>();
        Collection<ClassRecord> classes;      checkCreatable<ClassRecord>();
        Collection<FactRecord> factions;      checkCreatable<FactRecord>();
        Q_UNUSED(globals); Q_UNUSED(settings); Q_UNUSED(npcs);
        Q_UNUSED(races); Q_UNUSED(classes); Q_UNUSED(factions);
    }

    // The point of the rewrite: types that previously reported "not supported".
    void createsManyMoreTypes()
    {
        checkCreatable<WeaponRecord>();
        checkCreatable<ArmorRecord>();
        checkCreatable<SpellRecord>();
        checkCreatable<AlchRecord>();
        checkCreatable<IngrRecord>();
        checkCreatable<AmmoRecord>();
        checkCreatable<BookRecord>();
        checkCreatable<ActiRecord>();
        checkCreatable<QuestRecord>();
        checkCreatable<DialRecord>();
        checkCreatable<SounRecord>();
        checkCreatable<WthrRecord>();
        checkCreatable<PackageRecord>();
    }

    // A blank component-based record must have a usable component set, or the
    // form dialog would render an empty grid.
    void blankRecordsCarryTheirComponents()
    {
        Collection<NpcRecord> npcs;
        auto made = BlankRecordFactory::create(&npcs, QStringLiteral("N"), 0x801);
        QVERIFY(made != nullptr);
        auto* typed = static_cast<Record<NpcRecord>*>(made.get());
        QVERIFY(typed->get().components.all().size() > 0);
        QVERIFY(typed->get().components.findByName(QStringLiteral("TESFullName"))
                != nullptr);
    }

    // Two blanks of the same type must be independent, or editing one would
    // change the other before either is committed.
    void blanksAreIndependent()
    {
        Collection<NpcRecord> npcs;
        auto a = BlankRecordFactory::create(&npcs, QStringLiteral("A"), 0x802);
        auto b = BlankRecordFactory::create(&npcs, QStringLiteral("B"), 0x803);
        QVERIFY(a && b);

        auto* typedA = static_cast<Record<NpcRecord>*>(a.get());
        auto* typedB = static_cast<Record<NpcRecord>*>(b.get());
        typedA->get().editorId = QStringLiteral("Changed");
        QCOMPARE(typedB->get().editorId, QStringLiteral("B"));
        QCOMPARE(typedA->get().formId, 0x802u);
        QCOMPARE(typedB->get().formId, 0x803u);
    }

    void aNullCollectionIsNotCreatable()
    {
        QVERIFY(!BlankRecordFactory::supports(nullptr));
        QVERIFY(BlankRecordFactory::create(nullptr, QStringLiteral("X"), 1) == nullptr);
    }

    // The created record must survive AddRecordCommand, which is how the
    // Object Window actually inserts it.
    void aBlankRecordCanBeAddedThroughUndo()
    {
        Collection<SpellRecord> spells;
        IdTable table(&spells);
        UndoStack stack;

        auto record = BlankRecordFactory::create(&spells, QStringLiteral("NewSpell"), 0x804);
        QVERIFY(record != nullptr);
        stack.push(new AddRecordCommand(&table, &spells,
            spells.getAppendIndex(QStringLiteral("NewSpell")), *record,
            QStringLiteral("Add record: NewSpell")));

        QCOMPARE(spells.size(), 1);
        QCOMPARE(spells.getRecord(0).get().editorId, QStringLiteral("NewSpell"));
        QCOMPARE(spells.getRecord(0).get().formId, 0x804u);

        stack.undo();
        QCOMPARE(spells.size(), 0);
    }
};

QTEST_MAIN(TestBlankRecordFactory)
#include "test_blankrecordfactory.moc"

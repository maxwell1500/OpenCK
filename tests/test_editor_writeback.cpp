#include <QtTest>
#include <QFile>
#include <QFileInfo>

#include "model/world/data.hpp"
#include "model/world/collection.hpp"
#include "model/tools/editrecordcommand.hpp"
#include "model/tools/undostack.hpp"
#include "libs/files/esm/statrecord.hpp"
#include "libs/files/esm/glob.hpp"
#include "libs/files/esm/cellrecord.hpp"
#include "libs/files/esm/worldspacerecord.hpp"
#include "libs/files/esm/npcrecord.hpp"
#include "libs/files/esm/Packagerecord.hpp"
#include "libs/files/esm/refrecord.hpp"
#include "libs/files/esm/common.hpp"
#include "libs/files/esm/gameformat.hpp"
#include "libs/files/filepaths.hpp"
#include "logger.hpp"

class TestEditorWriteback : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testStatEditorUndoable();
    void testGlobEditorUndoable();
    void testCellEditorUndoable();
    void testWorldspaceEditorUndoable();
    void testNpcEditorUndoable();
    void testPackEditorUndoable();
    void testRefrTransformUndoable();
    void testCurrentGameDetection();
    void testNoChangeDoesNotPush();
};

void TestEditorWriteback::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QString());
}

void TestEditorWriteback::testStatEditorUndoable()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& col = data.getStatCollection();
    auto* stack = data.getUndoStack();

    StatRecord rec;
    rec.editorId = "smoke_stat";
    rec.formId = 0x00000800;
    rec.modelPath = "models\\original.nif";
    col.add(rec);

    StatRecord original = col.getRecord(0).get();
    StatRecord modified = original;
    modified.modelPath = "models\\edited.nif";

    EditRecordCommand<StatRecord> cmd(&col, 0, original, modified, "Edit Stat");
    QVERIFY(cmd.hasChanged());
    stack->push(new EditRecordCommand<StatRecord>(&col, 0, original, modified, "Edit Stat"));

    QCOMPARE(stack->undoCount(), 1);
    QCOMPARE(col.getRecord(0).get().modelPath, QString("models\\edited.nif"));

    stack->undo();
    QCOMPARE(col.getRecord(0).get().modelPath, QString("models\\original.nif"));

    stack->redo();
    QCOMPARE(col.getRecord(0).get().modelPath, QString("models\\edited.nif"));
}

void TestEditorWriteback::testGlobEditorUndoable()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& col = data.getGlobCollection();
    auto* stack = data.getUndoStack();

    GlobalVariable rec;
    rec.editorId = "smoke_glob";
    rec.constant = false;
    col.add(rec);

    GlobalVariable original = col.getRecord(0).get();
    GlobalVariable modified = original;
    modified.constant = true;

    EditRecordCommand<GlobalVariable> probe(&col, 0, original, modified);
    QVERIFY(probe.hasChanged());
    stack->push(new EditRecordCommand<GlobalVariable>(&col, 0, original, modified, "Edit GLOB"));

    QCOMPARE(stack->undoCount(), 1);
    QCOMPARE(col.getRecord(0).get().constant, true);

    stack->undo();
    QCOMPARE(col.getRecord(0).get().constant, false);

    stack->redo();
    QCOMPARE(col.getRecord(0).get().constant, true);
}

void TestEditorWriteback::testCellEditorUndoable()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& col = data.getCellCollection();
    auto* stack = data.getUndoStack();

    CellRecord rec;
    rec.editorId = "smoke_cell";
    rec.formId = 0x00000801;
    rec.hasWaterHeight = true;
    rec.waterHeight = 50.0f;
    col.add(rec);

    CellRecord original = col.getRecord(0).get();
    CellRecord modified = original;
    modified.waterHeight = 75.5f;

    EditRecordCommand<CellRecord> probe(&col, 0, original, modified);
    QVERIFY(probe.hasChanged());
    stack->push(new EditRecordCommand<CellRecord>(&col, 0, original, modified, "Edit Water"));

    QCOMPARE(stack->undoCount(), 1);
    QCOMPARE(col.getRecord(0).get().waterHeight, 75.5f);

    stack->undo();
    QCOMPARE(col.getRecord(0).get().waterHeight, 50.0f);

    stack->redo();
    QCOMPARE(col.getRecord(0).get().waterHeight, 75.5f);
}

void TestEditorWriteback::testWorldspaceEditorUndoable()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& col = data.getWorldspaceCollection();
    auto* stack = data.getUndoStack();

    WorldspaceRecord rec;
    rec.editorId = "smoke_world";
    rec.formId = 0x00000802;
    rec.name = "Original World";
    col.add(rec);

    WorldspaceRecord original = col.getRecord(0).get();
    WorldspaceRecord modified = original;
    modified.name = "Edited World";

    EditRecordCommand<WorldspaceRecord> probe(&col, 0, original, modified);
    QVERIFY(probe.hasChanged());
    stack->push(new EditRecordCommand<WorldspaceRecord>(&col, 0, original, modified, "Edit Worldspace"));

    QCOMPARE(stack->undoCount(), 1);
    QCOMPARE(col.getRecord(0).get().name, QString("Edited World"));

    stack->undo();
    QCOMPARE(col.getRecord(0).get().name, QString("Original World"));

    stack->redo();
    QCOMPARE(col.getRecord(0).get().name, QString("Edited World"));
}

void TestEditorWriteback::testNpcEditorUndoable()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& col = data.getNpcCollection();
    auto* stack = data.getUndoStack();

    NpcRecord rec;
    rec.editorId = "smoke_npc";
    rec.formId = 0x00000803;
    rec.level = 10;
    rec.health = 100;
    col.add(rec);

    NpcRecord original = col.getRecord(0).get();
    NpcRecord modified = original;
    modified.level = 25;

    EditRecordCommand<NpcRecord> probe(&col, 0, original, modified);
    QVERIFY(probe.hasChanged());
    stack->push(new EditRecordCommand<NpcRecord>(&col, 0, original, modified, "Edit NPC Level"));

    QCOMPARE(stack->undoCount(), 1);
    QCOMPARE(col.getRecord(0).get().level, 25u);

    stack->undo();
    QCOMPARE(col.getRecord(0).get().level, 10u);

    stack->redo();
    QCOMPARE(col.getRecord(0).get().level, 25u);
}

void TestEditorWriteback::testPackEditorUndoable()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& col = data.getPackCollection();
    auto* stack = data.getUndoStack();

    PackageRecord rec;
    rec.editorId = "smoke_pack";
    rec.formId = 0x00000804;
    col.add(rec);

    PackageRecord original = col.getRecord(0).get();
    PackageRecord modified = original;
    modified.editorId = "smoke_pack_edited";

    EditRecordCommand<PackageRecord> probe(&col, 0, original, modified);
    QVERIFY(probe.hasChanged());

    stack->push(new EditRecordCommand<PackageRecord>(&col, 0, original, modified, "Edit Package"));

    QCOMPARE(stack->undoCount(), 1);
    QCOMPARE(col.getRecord(0).get().editorId, QString("smoke_pack_edited"));

    stack->undo();
    QCOMPARE(col.getRecord(0).get().editorId, QString("smoke_pack"));
}

void TestEditorWriteback::testRefrTransformUndoable()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& col = data.getRefrCollection();
    auto* stack = data.getUndoStack();

    RefrRecord rec;
    rec.editorId = "smoke_refr";
    rec.formId = 0x00000806;
    rec.baseId = 0x00000007;
    rec.applyTransform(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    col.add(rec);

    // Mirrors the viewport drag-end commit path.
    RefrRecord original = col.getRecord(0).get();
    RefrRecord edited = original;
    edited.applyTransform(10.0f, 20.0f, 30.0f, 0.5f, 0.25f, 1.5f, 2.0f);

    EditRecordCommand<RefrRecord> probe(&col, 0, original, edited);
    QVERIFY(probe.hasChanged());
    stack->push(new EditRecordCommand<RefrRecord>(&col, 0, original, edited,
                                                  "Transform Reference"));

    QCOMPARE(stack->undoCount(), 1);
    QCOMPARE(col.getRecord(0).get().posX, 10.0f);
    QCOMPARE(col.getRecord(0).get().posY, 20.0f);
    QCOMPARE(col.getRecord(0).get().posZ, 30.0f);
    QCOMPARE(col.getRecord(0).get().rotX, 0.5f);
    QCOMPARE(col.getRecord(0).get().rotZ, 1.5f);
    QCOMPARE(col.getRecord(0).get().scale, 2.0f);

    stack->undo();
    QCOMPARE(col.getRecord(0).get().posX, 0.0f);
    QCOMPARE(col.getRecord(0).get().scale, 1.0f);

    stack->redo();
    QCOMPARE(col.getRecord(0).get().posX, 10.0f);
    QCOMPARE(col.getRecord(0).get().scale, 2.0f);

    // A transform that changes nothing must not register as a change.
    RefrRecord same = original;
    same.applyTransform(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    EditRecordCommand<RefrRecord> noChange(&col, 0, original, same);
    QVERIFY(!noChange.hasChanged());
}

void TestEditorWriteback::testCurrentGameDetection()
{
    const QString starfield = qEnvironmentVariable(
        "OPENCK_TEST_STARFIELD_ESM",
        QStringLiteral("C:/XboxGames/Starfield/Content/Data/Starfield.esm"));
    if (!QFile::exists(starfield))
    {
        QSKIP("Starfield.esm fixture not available (set OPENCK_TEST_STARFIELD_ESM)");
    }

    FilePaths paths(QCoreApplication::applicationName());
    paths.dataDir = QDir(QFileInfo(starfield).absolutePath());
    Data data(QStringList(), paths);

    const int count = data.preload(QFileInfo(starfield).fileName(), true);
    QVERIFY2(count > 0, "preload found no records");
    QCOMPARE(data.currentGame(), GameFormat::Game::Starfield);
    QVERIFY(data.isGameSpecificRecord(NAME('SHOU')));
    QVERIFY(!data.isGameSpecificRecord(NAME('PGRD')));
}

void TestEditorWriteback::testNoChangeDoesNotPush()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& col = data.getStatCollection();
    auto* stack = data.getUndoStack();

    StatRecord rec;
    rec.editorId = "smoke_nochange";
    rec.formId = 0x00000805;
    rec.modelPath = "models\\same.nif";
    col.add(rec);

    StatRecord original = col.getRecord(0).get();
    StatRecord same = original;

    EditRecordCommand<StatRecord> cmd(&col, 0, original, same);
    QVERIFY(!cmd.hasChanged());

    stack->push(new EditRecordCommand<StatRecord>(&col, 0, original, same));
    QCOMPARE(stack->undoCount(), 1);
    QCOMPARE(col.getRecord(0).get().modelPath, QString("models\\same.nif"));
}

QTEST_MAIN(TestEditorWriteback)
#include "test_editor_writeback.moc"

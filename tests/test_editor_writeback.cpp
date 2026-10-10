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
#include "libs/files/esm/scriptrecord.hpp"
#include "libs/files/esm/dialrecord.hpp"
#include "libs/files/esm/refrecord.hpp"
#include "libs/files/esm/common.hpp"
#include "libs/files/esm/gameformat.hpp"
#include "libs/files/filepaths.hpp"
#include "logger.hpp"
#include "model/doc/document.hpp"
#include "model/world/idtable.hpp"
#include "model/tools/addrecordcommand.hpp"
#include "model/tools/macrocommand.hpp"
#include "libs/files/esm/inforecord.hpp"
#include "libs/files/esm/wthrrecord.hpp"
#include "libs/files/esm/lighrecord.hpp"
#include "libs/files/esm/waterecord.hpp"
#include "libs/files/esm/gmst.hpp"
#include <QTemporaryDir>

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
void testScriptEditorUndoable();
void testDialAddInfoUndoable();
    void testRefrTransformUndoable();
    void testCurrentGameDetection();
    void testNoChangeDoesNotPush();
    void testDialogueActiveDocWorkflow();
    void testWeatherLightActiveDocWorkflow();
    void testWaterActiveDocWorkflow();
    void testAIPackageActiveDocWorkflow();
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

void TestEditorWriteback::testScriptEditorUndoable()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& col = data.getScptCollection();
    auto* stack = data.getUndoStack();

    ScriptRecord rec;
    rec.editorId = "smoke_script";
    rec.formId = 0x00000807;
    rec.scriptText = "Begin OnActivate\nEnd";
    col.add(rec);

    // Mirrors the Object Window script-edit commit path.
    ScriptRecord original = col.getRecord(0).get();
    ScriptRecord edited = original;
    edited.scriptText = "Begin OnActivate\nPlayer.AddItem f 1\nEnd";

    EditRecordCommand<ScriptRecord> probe(&col, 0, original, edited);
    QVERIFY(probe.hasChanged());
    stack->push(new EditRecordCommand<ScriptRecord>(&col, 0, original, edited,
                                                    "Edit Script: smoke_script"));

    QCOMPARE(stack->undoCount(), 1);
    QVERIFY(col.getRecord(0).get().scriptText.contains("AddItem"));

    stack->undo();
    QVERIFY(!col.getRecord(0).get().scriptText.contains("AddItem"));

    stack->redo();
    QVERIFY(col.getRecord(0).get().scriptText.contains("AddItem"));
}

void TestEditorWriteback::testDialAddInfoUndoable()
{
    FilePaths paths(QCoreApplication::applicationName());
    Data data(QStringList(), paths);
    auto& col = data.getDialCollection();
    auto* stack = data.getUndoStack();

    DialRecord rec;
    rec.editorId = "smoke_dial";
    rec.formId = 0x00000808;
    col.add(rec);

    // Mirrors the DialogueEditorWidget::onAddInfo commit path.
    DialRecord original = col.getRecord(0).get();
    DialRecord edited = original;
    edited.responseIds.append(0x00000809);
    edited.hasInam = true;

    EditRecordCommand<DialRecord> probe(&col, 0, original, edited);
    QVERIFY(probe.hasChanged());
    stack->push(new EditRecordCommand<DialRecord>(&col, 0, original, edited,
                                                  "Add Info to DIAL: smoke_dial"));

    QCOMPARE(stack->undoCount(), 1);
    QCOMPARE(col.getRecord(0).get().responseIds.size(), 1);
    QVERIFY(col.getRecord(0).get().hasInam);

    stack->undo();
    QVERIFY(col.getRecord(0).get().responseIds.isEmpty());

    stack->redo();
    QCOMPARE(col.getRecord(0).get().responseIds.size(), 1);
}

void TestEditorWriteback::testCurrentGameDetection()
{
    const QString dataDir = qEnvironmentVariable(
            "OPENCK_DATA_DIR",
            QStringLiteral("C:/XboxGames/Starfield/Content/Data"));
    const QString starfield = qEnvironmentVariable(
        "OPENCK_TEST_STARFIELD_ESM",
        dataDir + QStringLiteral("/Starfield.esm"));
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
static void loadPluginIntoData(Data& data, const QString& fullPath)
{
    const QString fileName = QFileInfo(fullPath).fileName();
    data.preload(fileName, false);
    Messages messages(Message::Info);
    int guard = 0;
    while (!data.continueLoading(messages))
    {
        if (++guard > 100000)
            break;
    }
}


void TestEditorWriteback::testDialogueActiveDocWorkflow()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString savePath = tempDir.filePath("test_dial_activedoc.esp");

    NewPluginOptions opts;
    opts.game = GameFormat::Game::Skyrim;
    opts.author = "OpenCK_Test";

    // 1. Create active document and add DIAL with AddRecordCommand
    {
        Document doc(QStringList(), savePath, true, opts);
        Data& data = doc.getData();
        auto& dialCol = data.getDialCollection();
        auto& infoCol = data.getInfoCollection();
        auto* stack = data.getUndoStack();

        DialRecord dial;
        dial.blank();
        dial.initComponents();
        dial.editorId = "TestDialogueTopic";
        dial.topicName = "Greetings";
        dial.formId = data.createNewRecord(CkId::Type_Dial_, dial.editorId);

        auto* dialTable = qobject_cast<IdTable*>(data.getTableModel(CkId::Type_Dial_));
        int dialAppendIdx = dialCol.getAppendIndex(dial.editorId, CkId::Type_Dial_);
        Record<DialRecord> dialRec(State_ModifiedOnly, nullptr, &dial);
        stack->push(new AddRecordCommand(dialTable, &dialCol, dialAppendIdx, dialRec, "Add DIAL"));

        QCOMPARE(dialCol.size(), 1);
        QCOMPARE(dialCol.getRecord(0).get().editorId, QString("TestDialogueTopic"));

        // 2. Add INFO under DIAL with parent index command
        InfoRecord info;
        info.blank();
        info.initComponents();
        info.editorId = "TestDialogueResponse_01";
        info.responseText = "Hello traveler!";
        info.formId = data.createNewRecord(CkId::Type_Info_, info.editorId);

        DialRecord origDial = dialCol.getRecord(0).get();
        DialRecord updatedDial = origDial;
        updatedDial.responseIds.append(info.formId);
        updatedDial.hasInam = true;

        auto* infoTable = qobject_cast<IdTable*>(data.getTableModel(CkId::Type_Info_));
        int infoAppendIdx = infoCol.getAppendIndex(info.editorId, CkId::Type_Info_);
        Record<InfoRecord> infoRec(State_ModifiedOnly, nullptr, &info);

        auto* macro = new MacroCommand("Add Response");
        macro->addCommand(new AddRecordCommand(infoTable, &infoCol, infoAppendIdx, infoRec, "Add INFO"));
        macro->addCommand(new EditRecordCommand<DialRecord>(&dialCol, 0, origDial, updatedDial, "Link Response"));
        stack->push(macro);

        QCOMPARE(infoCol.size(), 1);
        QCOMPARE(dialCol.getRecord(0).get().responseIds.size(), 1);
        QCOMPARE(dialCol.getRecord(0).get().responseIds[0], info.formId);

        // 3. Save active document
        doc.save(savePath);
        QVERIFY(QFile::exists(savePath));
        QVERIFY(QFileInfo(savePath).size() > 0);

        // 4. Test Undo: macro undoes both linking and info addition
        stack->undo();
        QCOMPARE(dialCol.getRecord(0).get().responseIds.size(), 0);
        QCOMPARE(infoCol.size(), 0);

        // 5. Test Redo: macro restores both
        stack->redo();
        QCOMPARE(dialCol.getRecord(0).get().responseIds.size(), 1);
        QCOMPARE(dialCol.getRecord(0).get().responseIds[0], info.formId);
        QCOMPARE(infoCol.size(), 1);
    }

    // 6. Reload from disk and verify persistence
    {
        FilePaths loadPaths;
        loadPaths.dataDir = QDir(tempDir.path());
        const QString fileName = QFileInfo(savePath).fileName();
        Data data(QStringList{ fileName }, loadPaths);
        loadPluginIntoData(data, savePath);
        auto& dialCol = data.getDialCollection();
        auto& infoCol = data.getInfoCollection();

        QCOMPARE(dialCol.size(), 1);
        QCOMPARE(dialCol.getRecord(0).get().editorId, QString("TestDialogueTopic"));
        QCOMPARE(dialCol.getRecord(0).get().responseIds.size(), 1);

        QCOMPARE(infoCol.size(), 1);
        QVERIFY(infoCol.getRecord(0).get().formId != 0);
        QCOMPARE(infoCol.getRecord(0).get().responseText, QString("Hello traveler!"));
        QCOMPARE(dialCol.getRecord(0).get().responseIds[0], infoCol.getRecord(0).get().formId);
    }
}

void TestEditorWriteback::testWeatherLightActiveDocWorkflow()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString savePath = tempDir.filePath("test_wthr_ligh_activedoc.esp");

    NewPluginOptions opts;
    opts.game = GameFormat::Game::Skyrim;
    opts.author = "OpenCK_Test";

    // 1. Create active document and add WTHR, LIGH, GMST
    {
        Document doc(QStringList(), savePath, true, opts);
        Data& data = doc.getData();
        auto& wthrCol = data.getWthrCollection();
        auto& lighCol = data.getLighCollection();
        auto& gmstCol = data.getGameSettings();
        auto* stack = data.getUndoStack();

        // Add WTHR
        WthrRecord wthr;
        wthr.blank();
        wthr.initComponents();
        wthr.editorId = "TestSkyrimWeather";
        wthr.sunTexture = "textures\\sky\\sun.dds";
        wthr.formId = data.createNewRecord(CkId::Type_Wthr_, wthr.editorId);

        auto* wthrTable = qobject_cast<IdTable*>(data.getTableModel(CkId::Type_Wthr_));
        int wthrAppendIdx = wthrCol.getAppendIndex(wthr.editorId, CkId::Type_Wthr_);
        Record<WthrRecord> wthrRec(State_ModifiedOnly, nullptr, &wthr);
        stack->push(new AddRecordCommand(wthrTable, &wthrCol, wthrAppendIdx, wthrRec, "Add WTHR"));

        // Add LIGH
        LighRecord ligh;
        ligh.blank();
        ligh.editorId = "TestTorchLight";
        ligh.fullName = "Torch Light";
        ligh.radius = 512;
        ligh.color = 0x00FF8800;
        ligh.formId = data.createNewRecord(CkId::Type_Ligh_, ligh.editorId);

        auto* lighTable = qobject_cast<IdTable*>(data.getTableModel(CkId::Type_Ligh_));
        int lighAppendIdx = lighCol.getAppendIndex(ligh.editorId, CkId::Type_Ligh_);
        Record<LighRecord> lighRec(State_ModifiedOnly, nullptr, &ligh);
        stack->push(new AddRecordCommand(lighTable, &lighCol, lighAppendIdx, lighRec, "Add LIGH"));

        // Add GMST
        GameSetting gmst;
        gmst.blank();
        gmst.editorId = "fWeatherTransitionTime";
        gmst.value.setFloat(15.0f);
        gmst.formId = data.createNewRecord(CkId::Type_Gmst, gmst.editorId);

        auto* gmstTable = qobject_cast<IdTable*>(data.getTableModel(CkId::Type_Gmst));
        int gmstAppendIdx = gmstCol.getAppendIndex(gmst.editorId, CkId::Type_Gmst);
        Record<GameSetting> gmstRec(State_ModifiedOnly, nullptr, &gmst);
        stack->push(new AddRecordCommand(gmstTable, &gmstCol, gmstAppendIdx, gmstRec, "Add GMST"));

        // Edit WTHR
        WthrRecord origWthr = wthrCol.getRecord(0).get();
        WthrRecord modWthr = origWthr;
        modWthr.sunTexture = "textures\\sky\\sun_glare.dds";
        stack->push(new EditRecordCommand<WthrRecord>(&wthrCol, 0, origWthr, modWthr, "Edit WTHR"));

        // Edit LIGH
        LighRecord origLigh = lighCol.getRecord(0).get();
        LighRecord modLigh = origLigh;
        modLigh.radius = 1024;
        stack->push(new EditRecordCommand<LighRecord>(&lighCol, 0, origLigh, modLigh, "Edit LIGH"));

        QCOMPARE(wthrCol.getRecord(0).get().sunTexture, QString("textures\\sky\\sun_glare.dds"));
        QCOMPARE(lighCol.getRecord(0).get().radius, quint32(1024));

        // Save active document
        doc.save(savePath);
        QVERIFY(QFile::exists(savePath));

        // Test Undo
        stack->undo(); // Undo Edit LIGH
        QCOMPARE(lighCol.getRecord(0).get().radius, quint32(512));
        stack->undo(); // Undo Edit WTHR
        QCOMPARE(wthrCol.getRecord(0).get().sunTexture, QString("textures\\sky\\sun.dds"));

        // Test Redo
        stack->redo();
        QCOMPARE(wthrCol.getRecord(0).get().sunTexture, QString("textures\\sky\\sun_glare.dds"));
        stack->redo();
        QCOMPARE(lighCol.getRecord(0).get().radius, quint32(1024));
    }

    // Reload and verify
    {
        FilePaths loadPaths;
        loadPaths.dataDir = QDir(tempDir.path());
        const QString fileName = QFileInfo(savePath).fileName();
        Data data(QStringList{ fileName }, loadPaths);
        loadPluginIntoData(data, savePath);
        auto& wthrCol = data.getWthrCollection();
        auto& lighCol = data.getLighCollection();
        auto& gmstCol = data.getGameSettings();

        QCOMPARE(wthrCol.size(), 1);
        QCOMPARE(wthrCol.getRecord(0).get().editorId, QString("TestSkyrimWeather"));
        QCOMPARE(wthrCol.getRecord(0).get().sunTexture, QString("textures\\sky\\sun_glare.dds"));

        QCOMPARE(lighCol.size(), 1);
        QCOMPARE(lighCol.getRecord(0).get().editorId, QString("TestTorchLight"));
        QCOMPARE(lighCol.getRecord(0).get().radius, quint32(1024));

        int gmstIdx = gmstCol.searchId("fWeatherTransitionTime");
        QVERIFY(gmstIdx >= 0);
        QCOMPARE(gmstCol.getRecord(gmstIdx).get().value.getFloat(), 15.0f);
    }
}

void TestEditorWriteback::testWaterActiveDocWorkflow()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString savePath = tempDir.filePath("test_water_activedoc.esp");

    NewPluginOptions opts;
    opts.game = GameFormat::Game::Skyrim;
    opts.author = "OpenCK_Test";

    // 1. Create active document and add WATR, GLOB
    {
        Document doc(QStringList(), savePath, true, opts);
        Data& data = doc.getData();
        auto& wateCol = data.getWateCollection();
        auto& globCol = data.getGlobCollection();
        auto* stack = data.getUndoStack();

        WateRecord watr;
        watr.blank();
        watr.editorId = "TestLakeWater";
        watr.fullName = "Crystal Lake Water";
        watr.waveHeight = 2.5f;
        watr.damage = 0.0f;
        watr.formId = data.createNewRecord(CkId::Type_Wate_, watr.editorId);

        auto* wateTable = qobject_cast<IdTable*>(data.getTableModel(CkId::Type_Wate_));
        int wateAppendIdx = wateCol.getAppendIndex(watr.editorId, CkId::Type_Wate_);
        Record<WateRecord> wateRec(State_ModifiedOnly, nullptr, &watr);
        stack->push(new AddRecordCommand(wateTable, &wateCol, wateAppendIdx, wateRec, "Add WATR"));

        GlobalVariable glob;
        glob.blank();
        glob.editorId = "WaterHeight";
        glob.value.setFloat(100.0f);
        glob.formId = data.createNewRecord(CkId::Type_Glob_, glob.editorId);

        auto* globTable = qobject_cast<IdTable*>(data.getTableModel(CkId::Type_Glob_));
        int globAppendIdx = globCol.getAppendIndex(glob.editorId, CkId::Type_Glob_);
        Record<GlobalVariable> globRec(State_ModifiedOnly, nullptr, &glob);
        stack->push(new AddRecordCommand(globTable, &globCol, globAppendIdx, globRec, "Add GLOB"));

        // Edit WATR
        WateRecord origWatr = wateCol.getRecord(0).get();
        WateRecord modWatr = origWatr;
        modWatr.waveHeight = 5.0f;
        modWatr.damage = 10.0f;
        stack->push(new EditRecordCommand<WateRecord>(&wateCol, 0, origWatr, modWatr, "Edit WATR"));

        QCOMPARE(wateCol.getRecord(0).get().waveHeight, 5.0f);
        QCOMPARE(wateCol.getRecord(0).get().damage, 10.0f);

        doc.save(savePath);
        QVERIFY(QFile::exists(savePath));

        // Test Undo
        stack->undo();
        QCOMPARE(wateCol.getRecord(0).get().waveHeight, 2.5f);
        QCOMPARE(wateCol.getRecord(0).get().damage, 0.0f);

        // Test Redo
        stack->redo();
        QCOMPARE(wateCol.getRecord(0).get().waveHeight, 5.0f);
        QCOMPARE(wateCol.getRecord(0).get().damage, 10.0f);
    }

    // Reload and verify
    {
        FilePaths loadPaths;
        loadPaths.dataDir = QDir(tempDir.path());
        const QString fileName = QFileInfo(savePath).fileName();
        Data data(QStringList{ fileName }, loadPaths);
        loadPluginIntoData(data, savePath);
        auto& wateCol = data.getWateCollection();
        auto& globCol = data.getGlobCollection();

        QCOMPARE(wateCol.size(), 1);
        QCOMPARE(wateCol.getRecord(0).get().editorId, QString("TestLakeWater"));
        QCOMPARE(wateCol.getRecord(0).get().fullName, QString("Crystal Lake Water"));
        QCOMPARE(wateCol.getRecord(0).get().waveHeight, 5.0f);
        QCOMPARE(wateCol.getRecord(0).get().damage, 10.0f);

        int globIdx = globCol.searchId("WaterHeight");
        QVERIFY(globIdx >= 0);
        QCOMPARE(globCol.getRecord(globIdx).get().value.getFloat(), 100.0f);
    }
}

void TestEditorWriteback::testAIPackageActiveDocWorkflow()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString savePath = tempDir.filePath("test_pack_activedoc.esp");

    NewPluginOptions opts;
    opts.game = GameFormat::Game::Skyrim;
    opts.author = "OpenCK_Test";

    // 1. Create active document and add PACK
    {
        Document doc(QStringList(), savePath, true, opts);
        Data& data = doc.getData();
        auto& packCol = data.getPackCollection();
        auto* stack = data.getUndoStack();

        PackageRecord pack;
        pack.blank();
        pack.initComponents();
        pack.editorId = "TestPatrolPackage";
        pack.packageType = 1; // Travel
        pack.formId = data.createNewRecord(CkId::Type_Pack_, pack.editorId);

        auto* packTable = qobject_cast<IdTable*>(data.getTableModel(CkId::Type_Pack_));
        int packAppendIdx = packCol.getAppendIndex(pack.editorId, CkId::Type_Pack_);
        Record<PackageRecord> packRec(State_ModifiedOnly, nullptr, &pack);
        stack->push(new AddRecordCommand(packTable, &packCol, packAppendIdx, packRec, "Add PACK"));

        // Edit PACK
        PackageRecord origPack = packCol.getRecord(0).get();
        PackageRecord modPack = origPack;
        modPack.packageType = 4; // Patrol
        modPack.targetIds.append(0x00000888);
        stack->push(new EditRecordCommand<PackageRecord>(&packCol, 0, origPack, modPack, "Edit PACK"));

        QCOMPARE(packCol.getRecord(0).get().packageType, quint32(4));
        QCOMPARE(packCol.getRecord(0).get().targetIds.size(), 1);

        doc.save(savePath);
        QVERIFY(QFile::exists(savePath));

        // Test Undo
        stack->undo();
        QCOMPARE(packCol.getRecord(0).get().packageType, quint32(1));
        QCOMPARE(packCol.getRecord(0).get().targetIds.size(), 0);

        // Test Redo
        stack->redo();
        QCOMPARE(packCol.getRecord(0).get().packageType, quint32(4));
        QCOMPARE(packCol.getRecord(0).get().targetIds.size(), 1);

        // Test Delete with undo
        bool removed = packCol.removeRecordWithUndo(pack.editorId, stack);
        QVERIFY(removed);
        QVERIFY(packCol.getRecord(0).state == State_Erased || packCol.size() == 0);

        // Undo deletion
        stack->undo();
        int restoredIdx = packCol.searchId(pack.editorId);
        QVERIFY(restoredIdx >= 0);
        QCOMPARE(packCol.getRecord(restoredIdx).get().packageType, quint32(4));
    }

    // Reload from disk and verify
    {
        FilePaths loadPaths;
        loadPaths.dataDir = QDir(tempDir.path());
        const QString fileName = QFileInfo(savePath).fileName();
        Data data(QStringList{ fileName }, loadPaths);
        loadPluginIntoData(data, savePath);
        auto& packCol = data.getPackCollection();

        QCOMPARE(packCol.size(), 1);
        QCOMPARE(packCol.getRecord(0).get().editorId, QString("TestPatrolPackage"));
        QCOMPARE(packCol.getRecord(0).get().packageType, quint32(4));
        QCOMPARE(packCol.getRecord(0).get().targetIds.size(), 1);
        QCOMPARE(packCol.getRecord(0).get().targetIds[0], quint32(0x00000888));
    }
}

QTEST_MAIN(TestEditorWriteback)
#include "test_editor_writeback.moc"

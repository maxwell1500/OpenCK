#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>

#include "../../src/model/doc/documentmediator.hpp"
#include "../../src/model/doc/document.hpp"
#include "../../src/model/world/ckid.hpp"
#include "../../src/model/window/objectwindow.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/log/logger.hpp"

// Loader protocol test against the live Document/Loader/Data pipeline.
// The Loader does the heavy parsing work, so it runs on a dedicated thread
// driven by the DocumentMediator's tick timer plus its own self-resume.
class TestLoaderSinglePass : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDocumentLoadedEmittedExactlyOnce();
    void testLoaderDoesNotRespawnPreload();
    void testDocumentInsertedAfterIdleTicks();
    void testDeferredMasterMaterialization();
    void testObjectWindowDeferredCategoryFetch();
    void testRealSeydaNeenDocument();
    void testSaveRoundTripGRUP();
    void testFormIdAllocation();

private:
    static bool writeTestPlugin(const QString& path)
    {
        return writeTestPluginEx(path, "LoaderTestNPC", 0x1234, "Loader Test Character", 5);
    }

    static bool writeTestPluginEx(const QString& path, const QString& editorId,
        quint32 formId, const QString& fullName, int level)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        ESMWriter writer;
        writer.setAuthor("Loader Test");
        writer.save(file);

        NpcRecord npc;
        npc.editorId = editorId;
        npc.formId = formId;
        npc.fullName = fullName;
        npc.level = level;

        RecHeader recHeader;
        recHeader.id = formId;
        writer.startRecord('NPC_', recHeader);
        npc.save(writer);
        writer.endRecord();

        writer.close();
        file.close();
        return true;
    }

    static Document* makeDocument(DocumentMediator& mediator,
        const QString& fileName, const QString& dir)
    {
        Document* doc = mediator.makeDocument(
            QStringList{ fileName }, dir + "/" + fileName, false);
        const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dir);
        return doc;
    }
};

void TestLoaderSinglePass::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_loader_log.txt"));
}

void TestLoaderSinglePass::testDocumentLoadedEmittedExactlyOnce()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPlugin(tmp.filePath("loader_regression.esm")));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = makeDocument(mediator, "loader_regression.esm", tmp.path());
    mediator.insertDocument(doc);

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);
}

void TestLoaderSinglePass::testLoaderDoesNotRespawnPreload()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPlugin(tmp.filePath("loader_regression_a.esm")));
    QVERIFY(writeTestPlugin(tmp.filePath("loader_regression_b.esm")));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    mediator.insertDocument(makeDocument(mediator, "loader_regression_a.esm", tmp.path()));
    mediator.insertDocument(makeDocument(mediator, "loader_regression_b.esm", tmp.path()));

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 2, 30000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);
    QCOMPARE(stopped.at(1).at(1).toBool(), true);

    // A loader that re-preloads finished documents would emit further
    // loadingStopped signals during this grace period.
    QTest::qWait(750);
    QCOMPARE(stopped.count(), 2);
}

void TestLoaderSinglePass::testDocumentInsertedAfterIdleTicks()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPlugin(tmp.filePath("loader_idle.esm")));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    // Reproduces the GUI scenario that used to deadlock: the tick timer
    // fires many times with an empty queue, and only then does a document
    // arrive. The loader must pick it up instead of blocking its thread's
    // event queue (the old QWaitCondition design never processed the
    // queued loadDocument in this case).
    QTest::qWait(500);

    mediator.insertDocument(makeDocument(mediator, "loader_idle.esm", tmp.path()));

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);
}

void TestLoaderSinglePass::testDeferredMasterMaterialization()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPluginEx(tmp.filePath("master_regression.esm"), "masternpc", 0x1111, "Master Character", 1));
    QVERIFY(writeTestPluginEx(tmp.filePath("plugin_regression.esm"), "pluginnpc", 0x2222, "Plugin Character", 2));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        QStringList{ "master_regression.esm", "plugin_regression.esm" },
        tmp.path() + "/plugin_regression.esm", false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(tmp.path());
    mediator.insertDocument(doc);

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);

    Data& data = doc->getData();

    // The master is deferred: only the edited plugin's NPC was parsed.
    QCOMPARE(data.getNpcCollection().size(), 1);
    QCOMPARE(data.getNpcCollection().getId(0), QStringLiteral("pluginnpc"));
    QCOMPARE(data.masterIndexCount(static_cast<int>(CkId::Type_Npc_)), 1);

    // Materializing the type parses the master's NPC too.
    QCOMPARE(data.ensureTypeLoaded(static_cast<int>(CkId::Type_Npc_)), 1);
    QCOMPARE(data.getNpcCollection().size(), 2);
    QVERIFY(data.getNpcCollection().searchId(QStringLiteral("masternpc")) >= 0);

    // A second materialization is a no-op (index entries consumed).
    QCOMPARE(data.ensureTypeLoaded(static_cast<int>(CkId::Type_Npc_)), 0);
    QCOMPARE(data.getNpcCollection().size(), 2);
    QCOMPARE(data.masterIndexCount(static_cast<int>(CkId::Type_Npc_)), 0);
}

// Regression for the Object Window crash on deferred categories: expanding
// a category whose type has both parsed (plugin) and deferred (master)
// records must materialize the master records through fetchMore() without
// resetting the model mid-fetch.
void TestLoaderSinglePass::testObjectWindowDeferredCategoryFetch()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPluginEx(tmp.filePath("master_regression.esm"), "masternpc", 0x1111, "Master Character", 1));
    QVERIFY(writeTestPluginEx(tmp.filePath("plugin_regression.esm"), "pluginnpc", 0x2222, "Plugin Character", 2));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        QStringList{ "master_regression.esm", "plugin_regression.esm" },
        tmp.path() + "/plugin_regression.esm", false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(tmp.path());
    mediator.insertDocument(doc);

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);

    ObjectWindowModel model;
    model.setData(&doc->getData());

    // Locate the NPC category across groups.
    QModelIndex npcCategory;
    bool found = false;
    for (int g = 0; g < model.rowCount() && !found; ++g)
    {
        const QModelIndex groupIdx = model.index(g, 0);
        for (int c = 0; c < model.rowCount(groupIdx); ++c)
        {
            const QModelIndex catIdx = model.index(c, 0, groupIdx);
            const int flatId = model.getCategoryIndex(catIdx);
            if (model.getCategoryType(flatId) == static_cast<int>(CkId::Type_Npc_))
            {
                npcCategory = catIdx;
                found = true;
                break;
            }
        }
    }
    QVERIFY(found);

    // Before the fetch: the plugin NPC is visible, the master NPC is
    // deferred, and the category reports the combined count.
    QCOMPARE(model.rowCount(npcCategory), 1);
    QVERIFY(model.canFetchMore(npcCategory));

    model.fetchMore(npcCategory);

    // Materialization + rebuild are synchronous; the master NPC row is
    // present immediately after fetchMore returns.
    QCOMPARE(model.rowCount(npcCategory), 2);
    QVERIFY(!model.canFetchMore(npcCategory));
    QCOMPARE(doc->getData().getNpcCollection().searchId(QStringLiteral("masternpc")) >= 0, true);
}

// Real-data gate: open SeydaNeen.esp together with its masters (Starfield
// + Magnus) in the intended order â€” masters first, edited plugin last. The
// masters must be indexed (deferred), the plugin's records parsed eagerly,
// and a small master record type materializable on demand. Skips when the
// Starfield data dir is absent.
void TestLoaderSinglePass::testRealSeydaNeenDocument()
{
    const QString dir = QStringLiteral("C:/XboxGames/Starfield/Content/Data");
    if (!QFileInfo::exists(dir + QStringLiteral("/SeydaNeen.esp")))
        QSKIP("Starfield data dir not found");

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        QStringList{ QStringLiteral("Starfield.esm"),
                     QStringLiteral("The Elder Star System - Magnus.esm"),
                     QStringLiteral("SeydaNeen.esp") },
        dir + QStringLiteral("/SeydaNeen.esp"), false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dir);
    mediator.insertDocument(doc);

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 120000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);

    Data& data = doc->getData();

    // The edited plugin's own records parsed eagerly.
    QVERIFY(data.getStatCollection().size() > 0);
    QVERIFY(data.getRefrCollection().size() > 0);
    QVERIFY(data.getCellCollection().size() > 0);
    qDebug() << "SeydaNeen eager records: stat" << data.getStatCollection().size()
             << "refr" << data.getRefrCollection().size()
             << "cell" << data.getCellCollection().size();

    // Masters are deferred: their NPCs/weather are in the index, not parsed.
    QVERIFY(data.masterIndexCount(static_cast<int>(CkId::Type_Npc_)) > 0);
    QVERIFY(data.masterIndexCount(static_cast<int>(CkId::Type_Wthr_)) > 0);

    // Materialize a small master type end-to-end (weather records).
    const int before = data.getWthrCollection().size();
    const int loaded = data.ensureTypeLoaded(static_cast<int>(CkId::Type_Wthr_));
    QVERIFY(loaded > 0);
    QCOMPARE(data.getWthrCollection().size(), before + loaded);
    qDebug() << "materialized master weather records:" << loaded;

    // Deferred master records of other types remain untouched.
    QVERIFY(data.masterIndexCount(static_cast<int>(CkId::Type_Npc_)) > 0);

    // Materialize the record types whose subrecord parsers were recently
    // fixed (CNTO/SPIT/CTDA/PTDT) from the real master, end-to-end. A
    // misaligned parser here is what made expanding Object Window
    // categories crash or hang.
    const struct { CkId::Type type; const char* label; } materialize[] = {
        { CkId::Type_Cont_, "cont" },
        { CkId::Type_Npc_, "npc" },
        { CkId::Type_Spel_, "spel" },
        { CkId::Type_Info_, "info" },
        { CkId::Type_Pack_, "pack" },
    };
    for (const auto& m : materialize)
    {
        const int n = data.ensureTypeLoaded(static_cast<int>(m.type));
        qDebug() << "materialized master" << m.label << "records:" << n;
        QVERIFY(n > 0);
    }

    // ==== User-reported Object Window click path on real data ====
    // Expanding the Worldspace category must materialize Starfield's
    // worldspaces (not just the plugin's single one), rebuild the model,
    // and a record click (currentChanged -> getFormComponentsForIndex)
    // must resolve components without crashing.
    ObjectWindowModel model;
    model.setData(&data);

    QModelIndex wrldCategory;
    bool found = false;
    for (int g = 0; g < model.rowCount() && !found; ++g)
    {
        const QModelIndex groupIdx = model.index(g, 0);
        for (int c = 0; c < model.rowCount(groupIdx); ++c)
        {
            const QModelIndex catIdx = model.index(c, 0, groupIdx);
            if (model.getCategoryType(model.getCategoryIndex(catIdx))
                == static_cast<int>(CkId::Type_WRLD_))
            {
                wrldCategory = catIdx;
                found = true;
                break;
            }
        }
    }
    QVERIFY(found);
    QCOMPARE(model.rowCount(wrldCategory), 1);
    QVERIFY(model.canFetchMore(wrldCategory));

    model.fetchMore(wrldCategory);

    QVERIFY(model.rowCount(wrldCategory) > 1);
    QVERIFY(!model.canFetchMore(wrldCategory));
    qDebug() << "worldspaces after expand:" << model.rowCount(wrldCategory);

    // Click every worldspace row the way the Inspector wiring does.
    for (int r = 0; r < model.rowCount(wrldCategory); ++r)
    {
        const QModelIndex recIdx = model.index(r, 0, wrldCategory);
        QVERIFY(model.isRecord(recIdx));
        const int cat = model.getCategoryIndex(recIdx);
        const int rec = model.getRecordIndex(recIdx);
        QVERIFY(!model.getRecordEditorId(cat, rec).isEmpty());
        QVERIFY(!model.getRecordFormId(cat, rec).isEmpty());
    }
    qDebug() << "worldspace click-path simulation OK";
}

// Real-data save gate: loading SeydaNeen with its masters and saving must
// emit a structurally valid plugin (TES4 header with real masters, top-level
// GRUPs, cell-children GRUPs) that reloads to the same record counts.
void TestLoaderSinglePass::testSaveRoundTripGRUP()
{
    const QString dir = QStringLiteral("C:/XboxGames/Starfield/Content/Data");
    if (!QFileInfo::exists(dir + QStringLiteral("/SeydaNeen.esp")))
        QSKIP("Starfield data dir not found");

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        QStringList{ QStringLiteral("Starfield.esm"),
                     QStringLiteral("The Elder Star System - Magnus.esm"),
                     QStringLiteral("SeydaNeen.esp") },
        dir + QStringLiteral("/SeydaNeen.esp"), false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dir);
    mediator.insertDocument(doc);
    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 120000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);

    const int statCount = doc->getData().getStatCollection().size();
    const int refrCount = doc->getData().getRefrCollection().size();
    const int cellCount = doc->getData().getCellCollection().size();
    QVERIFY(statCount > 0 && refrCount > 0 && cellCount > 0);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString savedPath = tmp.path() + QStringLiteral("/SeydaNeen_saved.esp");
    doc->save(savedPath);
    QVERIFY(QFileInfo::exists(savedPath));

    QFile f(savedPath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray bytes = f.readAll();
    f.close();
    QVERIFY(bytes.size() > 24);
    QCOMPARE(QByteArray(bytes.constData(), 4), QByteArray("TES4"));

    // Scan for GRUP blocks: 'GRUP' + size(u32) + label(4) + groupType(u32).
    bool foundStatGrup = false;
    bool foundCellChildrenGrup = false;
    for (int i = 0; i + 24 <= bytes.size(); ++i)
    {
        if (QByteArray(bytes.constData() + i, 4) != QByteArray("GRUP"))
            continue;
        const quint32 label = static_cast<quint8>(bytes.at(i + 8));
        const quint32 type = static_cast<quint8>(bytes.at(i + 12));
        if (label == static_cast<quint8>('S') && type == 0)
            foundStatGrup = true;
        if (type == 6)
            foundCellChildrenGrup = true;
        i += 23; // next potential header start
    }
    QVERIFY(foundStatGrup);
    QVERIFY(foundCellChildrenGrup);

    // Walk the saved file and confirm every record still parses, and the
    // TES4 record count matches what was written.
    ESMReader reader(savedPath);
    reader.open();
    int totalSeen = 0, seenStat = 0, seenCell = 0, seenRefr = 0;
    while (reader.isLeft())
    {
        const NAME name = reader.readName();
        if (name == static_cast<NAME>('GRUP'))
        {
            reader.skipGrupHeader();
            continue;
        }
        reader.readHeader();
        ++totalSeen;
        if (name == static_cast<NAME>('STAT')) ++seenStat;
        else if (name == static_cast<NAME>('CELL')) ++seenCell;
        else if (name == static_cast<NAME>('REFR')) ++seenRefr;
        reader.skip(static_cast<int>(reader.recLeft()));
    }
    QCOMPARE(reader.getHeader().numRecords, totalSeen);
    QCOMPARE(seenStat, statCount);
    QCOMPARE(seenCell, cellCount);
    QCOMPARE(seenRefr, refrCount);
    qDebug() << "saved plugin round-trips: stat" << seenStat
             << "refr" << seenRefr << "cell" << seenCell;
}

// New records must be allocated in the edited plugin's form-id space
// (load-order index in the high byte), not a fabricated range.
void TestLoaderSinglePass::testFormIdAllocation()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPluginEx(tmp.filePath("master_formid.esm"), "masternpc", 0x1111, "Master", 1));
    QVERIFY(writeTestPluginEx(tmp.filePath("plugin_formid.esp"), "pluginnpc", 0x2222, "Plugin", 2));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    Document* doc = mediator.makeDocument(
        QStringList{ "master_formid.esm", "plugin_formid.esp" },
        tmp.path() + "/plugin_formid.esp", false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(tmp.path());
    mediator.insertDocument(doc);
    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);

    // plugin_formid.esp is index 1 in the load order -> high byte 0x01.
    const quint32 a = doc->getData().createNewRecord(CkId::Type_Npc_, QStringLiteral("NewA"));
    QCOMPARE(a >> 24, static_cast<quint32>(0x01));
    QVERIFY((a & 0xFFFFFF) >= 0x800);
    qDebug() << "allocated form id" << QString::number(a, 16);
}

QTEST_GUILESS_MAIN(TestLoaderSinglePass)
#include "test_loader.moc"

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

// Loader protocol test against the live Document/Loader/Data pipeline.
// The Loader does the heavy parsing work, so it runs on a dedicated thread
// driven by the DocumentMediator's tick timer plus its own self-resume.
class TestLoaderSinglePass : public QObject
{
    Q_OBJECT

private slots:
    void testDocumentLoadedEmittedExactlyOnce();
    void testLoaderDoesNotRespawnPreload();
    void testDocumentInsertedAfterIdleTicks();
    void testDeferredMasterMaterialization();
    void testObjectWindowDeferredCategoryFetch();

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

QTEST_GUILESS_MAIN(TestLoaderSinglePass)
#include "test_loader.moc"
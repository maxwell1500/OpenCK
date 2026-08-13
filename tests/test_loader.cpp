#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>

#include "../../src/model/doc/documentmediator.hpp"
#include "../../src/model/doc/document.hpp"
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

private:
    static bool writeTestPlugin(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        ESMWriter writer;
        writer.setAuthor("Loader Test");
        writer.save(file);

        NpcRecord npc;
        npc.editorId = "LoaderTestNPC";
        npc.formId = 0x1234;
        npc.fullName = "Loader Test Character";
        npc.level = 5;

        RecHeader recHeader;
        recHeader.id = 0x1234;
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

QTEST_GUILESS_MAIN(TestLoaderSinglePass)
#include "test_loader.moc"
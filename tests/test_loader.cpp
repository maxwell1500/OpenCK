#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>

#include "../../src/model/doc/documentmediator.hpp"
#include "../../src/model/doc/document.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/npcrecord.hpp"

// Loader protocol test against the live Document/Loader/Data pipeline.
// The Loader blocks inside its timer slot (QWaitCondition) whenever its
// document queue is empty, so it must run on a dedicated thread. The
// DocumentMediator does exactly that, so these tests drive the loader
// through the mediator instead of constructing the Loader directly.
class TestLoaderSinglePass : public QObject
{
    Q_OBJECT

private slots:
    void testDocumentLoadedEmittedExactlyOnce();
    void testLoaderDoesNotRespawnPreload();

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

QTEST_GUILESS_MAIN(TestLoaderSinglePass)
#include "test_loader.moc"
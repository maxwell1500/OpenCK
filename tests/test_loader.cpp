#include <QtTest>
#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDataStream>
#include <QByteArray>
#include <QEventLoop>
#include <QTimer>
#include <QSignalSpy>
#include <QStringList>
#include <QDir>

#include "../../src/model/doc/document.hpp"
#include "../../src/model/doc/loader.hpp"
#include "../../libs/files/filepaths.hpp"

class TestLoaderSinglePass : public QObject
{
    Q_OBJECT

private slots:
    void testDocumentLoadedEmittedExactlyOnce();
    void testLoaderDoesNotRespawnPreload();
};

static QByteArray buildEs4RecordHeader(quint32 bodySize)
{
    QByteArray header;
    QDataStream ds(&header, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.writeRawData("TES4", 4);
    ds << bodySize;
    ds << quint32(0);
    ds << quint32(0);
    ds << quint32(0);
    ds << quint16(0);
    ds << quint16(0);
    return header;
}

static QByteArray buildHedrSubrecord(float version, qint32 numRecords, quint32 nextObjectId)
{
    QByteArray sub;
    QDataStream ds(&sub, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.writeRawData("HEDR", 4);
    ds << quint16(12);
    ds << version;
    ds << numRecords;
    ds << nextObjectId;
    return sub;
}

static QByteArray buildGmstRecord(quint32 formId, const QByteArray& editorId, quint32 intValue)
{
    QByteArray body;
    QDataStream bs(&body, QIODevice::WriteOnly);
    bs.setByteOrder(QDataStream::LittleEndian);
    bs.writeRawData("EDID", 4);
    bs << quint16(static_cast<quint16>(editorId.size() + 1));
    bs.writeRawData(editorId.constData(), editorId.size());
    bs << quint8(0);
    bs.writeRawData("DATA", 4);
    bs << quint16(4);
    bs << intValue;

    QByteArray rec;
    QDataStream rs(&rec, QIODevice::WriteOnly);
    rs.setByteOrder(QDataStream::LittleEndian);
    rs.writeRawData("GMST", 4);
    rs << quint32(static_cast<quint32>(body.size()));
    rs << quint32(0);
    rs << formId;
    rs << quint32(0);
    rs << quint16(0);
    rs << quint16(0);
    rs.writeRawData(body.constData(), body.size());
    return rec;
}

static bool writeSyntheticEsm(const QString& path)
{
    QByteArray hedr = buildHedrSubrecord(0.94f, 2, 0x00000800);
    QByteArray tes4Body = hedr;
    QByteArray tes4 = buildEs4RecordHeader(static_cast<quint32>(tes4Body.size())) + tes4Body;

    QByteArray gmst1 = buildGmstRecord(0x00000800, "iTest1", 1);
    QByteArray gmst2 = buildGmstRecord(0x00000801, "iTest2", 2);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    bool ok = f.write(tes4) == tes4.size()
        && f.write(gmst1) == gmst1.size()
        && f.write(gmst2) == gmst2.size();
    f.close();
    return ok;
}

void TestLoaderSinglePass::testDocumentLoadedEmittedExactlyOnce()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString baseName = "loader_regression.esm";
    const QString filePath = tmpDir.filePath(baseName);
    QVERIFY(writeSyntheticEsm(filePath));

    Document* document = new Document(QStringList{ baseName }, filePath, false);

    const_cast<FilePaths&>(document->getData().getPaths()).dataDir.setPath(tmpDir.path());

    Loader loader;
    QSignalSpy loadedSpy(&loader, &Loader::documentLoaded);
    QSignalSpy failedSpy(&loader, &Loader::documentNotLoaded);
    QVERIFY(loadedSpy.isValid());
    QVERIFY(failedSpy.isValid());

    loader.loadDocument(document);

    QEventLoop loop;
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    QObject::connect(&loader, &Loader::documentLoaded, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    QObject::connect(&loader, &Loader::documentNotLoaded, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    loop.exec();

    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(loadedSpy.count(), 1);

    delete document;
}

void TestLoaderSinglePass::testLoaderDoesNotRespawnPreload()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    const QString baseName = "loader_regression_2.esm";
    const QString filePath = tmpDir.filePath(baseName);
    QVERIFY(writeSyntheticEsm(filePath));

    auto makeDoc = [&]() {
        Document* d = new Document(QStringList{ baseName }, filePath, false);
        const_cast<FilePaths&>(d->getData().getPaths()).dataDir.setPath(tmpDir.path());
        return d;
    };

    Document* doc1 = makeDoc();
    Document* doc2 = makeDoc();

    Loader loader;
    QSignalSpy loadedSpy(&loader, &Loader::documentLoaded);
    QSignalSpy failedSpy(&loader, &Loader::documentNotLoaded);
    QVERIFY(loadedSpy.isValid());
    QVERIFY(failedSpy.isValid());

    loader.loadDocument(doc1);
    loader.loadDocument(doc2);

    QEventLoop loop;
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    QObject::connect(&loader, &Loader::documentLoaded, &loop, [&]() {
        if (loadedSpy.count() >= 2) loop.quit();
    }, Qt::QueuedConnection);
    QObject::connect(&loader, &Loader::documentNotLoaded, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    loop.exec();

    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(loadedSpy.count(), 2);

    delete doc1;
    delete doc2;
}

QTEST_MAIN(TestLoaderSinglePass)
#include "test_loader.moc"

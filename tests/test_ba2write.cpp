#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStringList>

#include "ba2archive.hpp"
#include "logger.hpp"

// Round-trips the real BTDX v2 GNRL writer: create() writes a genuine
// Bethesda BTDX v2 archive that open()/extract() read back, for both
// compressed and stored payloads.
class TestBa2Write : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testWriteReadCompressed();
    void testWriteReadStored();
};

void TestBa2Write::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_ba2write_log.txt"));
}

namespace {
QString writeTempInput(const QString& name, const QByteArray& bytes)
{
    const QString dir = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2write");
    QDir().mkpath(dir);
    const QString path = dir + "/" + name;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(bytes);
    f.close();
    return path;
}
}

void TestBa2Write::testWriteReadCompressed()
{
    const QString archive = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2write/test_gnrl.ba2");
    QFile::remove(archive);

    const QByteArray textData = QByteArray("hello world hello world hello world hello world\n").repeated(64);
    QStringList inputs;
    inputs << writeTempInput("meshes/foo/bar.nif", QByteArray("NIFDATA").repeated(500))
           << writeTempInput("meshes/foo/baz.nif", textData)
           << writeTempInput("sound/vo/test.fuz", QByteArray("FUZE-data").repeated(300));

    Ba2Archive writer;
    QVERIFY(writer.create(inputs, archive, /*compress=*/true, "GNRL"));

    QVERIFY(QFileInfo::exists(archive));
    Ba2Archive reader;
    QVERIFY(reader.open(archive));
    QCOMPARE(reader.fileCount(), static_cast<quint32>(3));

    const QString outDir = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2write/out");
    QDir().mkpath(outDir);
    for (quint32 i = 0; i < reader.fileCount(); ++i)
    {
        const auto& e = reader.entries().at(static_cast<int>(i));
        const QString out = outDir + "/" + QFileInfo(e.relativePath).fileName();
        QVERIFY(reader.extract(i, out));
        QFile f(out);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QByteArray got = f.readAll();
        f.close();
        QByteArray want;
        if (e.relativePath.endsWith("bar.nif")) want = QByteArray("NIFDATA").repeated(500);
        else if (e.relativePath.endsWith("baz.nif")) want = textData;
        else want = QByteArray("FUZE-data").repeated(300);
        QCOMPARE(got, want);
        QVERIFY(e.compressed);
    }
    QFile::remove(archive);
}

void TestBa2Write::testWriteReadStored()
{
    const QString archive = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2write/test_stored.ba2");
    QFile::remove(archive);

    QStringList inputs;
    inputs << writeTempInput("meshes/foo/bar.nif", QByteArray("RAW").repeated(1000))
           << writeTempInput("textures/foo/test.dds", QByteArray("DDS").repeated(800));

    Ba2Archive writer;
    QVERIFY(writer.create(inputs, archive, /*compress=*/false, "GNRL"));

    QVERIFY(QFileInfo::exists(archive));
    Ba2Archive reader;
    QVERIFY(reader.open(archive));
    QCOMPARE(reader.fileCount(), static_cast<quint32>(2));

    const QString outDir = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/ba2write/out2");
    QDir().mkpath(outDir);
    for (quint32 i = 0; i < reader.fileCount(); ++i)
    {
        const auto& e = reader.entries().at(static_cast<int>(i));
        const QString out = outDir + "/" + QFileInfo(e.relativePath).fileName();
        QVERIFY(reader.extract(i, out));
        QFile f(out);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QByteArray got = f.readAll();
        f.close();
        const QByteArray want = (e.relativePath.endsWith("bar.nif"))
            ? QByteArray("RAW").repeated(1000) : QByteArray("DDS").repeated(800);
        QCOMPARE(got, want);
        QVERIFY(!e.compressed);
    }
    QFile::remove(archive);
}

QTEST_MAIN(TestBa2Write)
#include "test_ba2write.moc"

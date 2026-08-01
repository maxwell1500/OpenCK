#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QByteArray>

#include "../../libs/files/data/zipwriter.hpp"
#include "../../libs/files/log/logger.hpp"

// Minimal ZIP reader: parses local file headers + central directory enough
// to extract stored/deflated entries. Uses zlib for deflate.
#include <zlib.h>

class TestZipWriter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testWriteAndReadBack();
    void testMultipleEntries();
    void testFileFromDisk();
    void testOpenFailure();
};

void TestZipWriter::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_zipwriter_log.txt"));
}

// Extracts the named entry from a ZIP file's bytes. Returns empty if not found.
static QByteArray extractEntry(const QByteArray& zipBytes, const QByteArray& wantedName)
{
    quint32 pos = 0;
    // Scan local file headers, tracking each name and the data that follows.
    struct Entry { QByteArray name; QByteArray data; };
    QVector<Entry> entries;

    while (pos + 4 <= static_cast<quint32>(zipBytes.size())) {
        quint32 sig;
        memcpy(&sig, zipBytes.constData() + pos, 4);
        if (sig != 0x04034b50) break; // local file header signature

        quint16 method, nameLen, extraLen;
        quint32 compSize, uncompSize;
        memcpy(&method, zipBytes.constData() + pos + 8, 2);
        memcpy(&nameLen, zipBytes.constData() + pos + 26, 2);
        memcpy(&extraLen, zipBytes.constData() + pos + 28, 2);
        memcpy(&compSize, zipBytes.constData() + pos + 18, 4);
        memcpy(&uncompSize, zipBytes.constData() + pos + 22, 4);

        const quint32 nameStart = pos + 30;
        const quint32 dataStart = nameStart + nameLen + extraLen;
        if (dataStart + compSize > static_cast<quint32>(zipBytes.size())) break;

        Entry e;
        e.name = zipBytes.mid(nameStart, nameLen);
        const QByteArray raw = zipBytes.mid(dataStart, compSize);
        if (method == 0) {
            e.data = raw;
        } else {
            QByteArray out(uncompSize, Qt::Uninitialized);
            uLongf outLen = static_cast<uLongf>(uncompSize);
            int res = uncompress(reinterpret_cast<Bytef*>(out.data()), &outLen,
                reinterpret_cast<const Bytef*>(raw.constData()), static_cast<uLong>(raw.size()));
            if (res == Z_OK) {
                out.truncate(static_cast<int>(outLen));
                e.data = out;
            }
        }
        entries.append(e);
        pos = dataStart + compSize;
    }

    for (const Entry& e : entries) {
        if (e.name == wantedName) return e.data;
    }
    return QByteArray();
}

void TestZipWriter::testWriteAndReadBack()
{
    QTemporaryFile tmpFile;
    tmpFile.open();
    QString path = tmpFile.fileName();
    tmpFile.close();
    QFile::remove(path);

    {
        ZipWriter zip;
        QVERIFY(zip.open(path));
        zip.addFile(QStringLiteral("hello.txt"), QByteArray("Hello, ZIP!"));
        zip.close();
    }

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray bytes = f.readAll();
    f.close();

    QVERIFY(bytes.startsWith("PK\x03\x04"));
    QVERIFY(bytes.contains("PK\x01\x02"));   // central directory
    QVERIFY(bytes.contains("PK\x05\x06"));   // end of central directory

    const QByteArray extracted = extractEntry(bytes, "hello.txt");
    QCOMPARE(extracted, QByteArray("Hello, ZIP!"));
}

void TestZipWriter::testMultipleEntries()
{
    QTemporaryFile tmpFile;
    tmpFile.open();
    QString path = tmpFile.fileName();
    tmpFile.close();
    QFile::remove(path);

    {
        ZipWriter zip;
        QVERIFY(zip.open(path));
        zip.addFile(QStringLiteral("a.txt"), QByteArray("AAAA"));
        zip.addFile(QStringLiteral("sub/b.bin"), QByteArray("BBBBBBBB"));
        zip.addFile(QStringLiteral("c.txt"), QByteArray("CC"));
        zip.close();
    }

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray bytes = f.readAll();
    f.close();

    QCOMPARE(extractEntry(bytes, "a.txt"), QByteArray("AAAA"));
    QCOMPARE(extractEntry(bytes, "sub/b.bin"), QByteArray("BBBBBBBB"));
    QCOMPARE(extractEntry(bytes, "c.txt"), QByteArray("CC"));
}

void TestZipWriter::testFileFromDisk()
{
    QTemporaryFile srcFile;
    QVERIFY(srcFile.open());
    srcFile.write("disk contents here");
    srcFile.close();

    QTemporaryFile tmpFile;
    tmpFile.open();
    QString path = tmpFile.fileName();
    tmpFile.close();
    QFile::remove(path);

    {
        ZipWriter zip;
        QVERIFY(zip.open(path));
        QVERIFY(zip.addFileFromDisk(srcFile.fileName(), QStringLiteral("disk.txt")));
        zip.close();
    }

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray bytes = f.readAll();
    f.close();

    QCOMPARE(extractEntry(bytes, "disk.txt"), QByteArray("disk contents here"));
}

void TestZipWriter::testOpenFailure()
{
    ZipWriter zip;
    QVERIFY(!zip.open(QStringLiteral("Z:/definitely/not/a/real/dir/out.zip")));
}

QTEST_MAIN(TestZipWriter)
#include "test_zipwriter.moc"

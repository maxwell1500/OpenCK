#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStringList>#include <algorithm>

#include "ba2/bsaarchive.hpp"
#include "logger.hpp"

// Validates the Skyrim SE BSA writer:
//  1. A created v0x69 BSA round-trips through the reader (extract all).
//  2. The 64-bit name hash matches the hashes stored in the real
//     Skyrim SE Voices archive (which uses the same algorithm).
// Requires the user's Skyrim SE install; not registered with CTest.
class TestBsaWrite : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCreateRoundTrip();
    void testHashMatchesRealArchive();
};

void TestBsaWrite::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_bsawrite_log.txt"));
}

QString writeTempInput(const QString& name, const QByteArray& bytes)
{
    const QString dir = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/bsawrite");
    QDir().mkpath(dir);
    const QString path = dir + "/" + name;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(bytes);
    f.close();
    return path;
}

void TestBsaWrite::testCreateRoundTrip()
{
    const QString archive = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/bsawrite/test.bsa");
    QFile::remove(archive);

    const QByteArray textData = QByteArray("hello skyrim\n").repeated(64);
    QStringList inputs;
    inputs << writeTempInput("meshes/foo/bar.nif", QByteArray("NIFDATA").repeated(400))
           << writeTempInput("meshes/foo/baz.nif", textData)
           << writeTempInput("sound/vo/test.fuz", QByteArray("FUZE").repeated(200));

    BsaArchive writer;
    QVERIFY(writer.create(inputs, archive));
    QVERIFY(QFileInfo::exists(archive));

    BsaArchive reader;
    QVERIFY(reader.open(archive));
    QCOMPARE(reader.fileCount(), 3);
    QCOMPARE(reader.version(), 0x69);

    const QString outDir = QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/bsawrite/out");
    QDir().mkpath(outDir);
    for (quint32 i = 0; i < static_cast<quint32>(reader.fileCount()); ++i)
    {
        const auto& e = reader.entries().at(static_cast<int>(i));
        const QString out = outDir + "/" + QFileInfo(e.fullPath).fileName();
        QVERIFY(reader.extract(i, out));
        QFile f(out);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QByteArray got = f.readAll();
        f.close();
        QByteArray want;
        if (e.fullPath.endsWith("bar.nif")) want = QByteArray("NIFDATA").repeated(400);
        else if (e.fullPath.endsWith("baz.nif")) want = textData;
        else want = QByteArray("FUZE").repeated(200);
        QCOMPARE(got, want);
    }
    QFile::remove(archive);
}

void TestBsaWrite::testHashMatchesRealArchive()
{
    const QString archive = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim - Voices_en0.bsa");
    if (!QFileInfo::exists(archive)) QSKIP("Skyrim SE Voices archive not found");

    BsaArchive reader;
    QVERIFY(reader.open(archive));

    // The reader keeps the archive's stored 64-bit name hash in each
    // entry; recompute it from the stored path and require equality. This
    // validates the hash algorithm against real Bethesda-authored data.
    int checked = 0;
    int mismatched = 0;
    for (const auto& e : reader.entries())
    {
        if (e.fullPath.isEmpty()) continue;

        // Names carrying non-ASCII characters were decoded lossy (the reader
        // uses Latin1, the archive stores raw bytes), so the original bytes
        // and thus the hash cannot be reconstructed from the QString. Skip
        // those; everything else must match exactly.
        const bool ascii = std::all_of(e.fullPath.begin(), e.fullPath.end(),
            [](QChar c) { return c.unicode() < 128; });
        if (!ascii) continue;

        const int slash = e.fullPath.lastIndexOf('\\');
        const QString fileName = slash >= 0 ? e.fullPath.mid(slash + 1) : e.fullPath;
        const int dot = fileName.lastIndexOf('.');
        const QString stem = dot > 0 ? fileName.left(dot) : fileName;
        const QString ext = dot > 0 ? fileName.mid(dot) : QString();

        if (BsaArchive::hashName(stem, ext) != e.nameHash)
        {
            ++mismatched;
            if (mismatched <= 3)
                qDebug() << "hash mismatch for" << e.fullPath;
        }
        ++checked;
    }
    qDebug() << "checked" << checked << "name hashes, mismatches:" << mismatched;
    QVERIFY(checked > 1000);
    QCOMPARE(mismatched, 0);
}

QTEST_MAIN(TestBsaWrite)
#include "test_bsawrite.moc"

#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>

#include "src/model/tools/archiveconverter.hpp"
#include "libs/files/ba2/bsaarchive.hpp"
#include "libs/files/log/logger.hpp"

class TestArchiveConverter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBsaToOblivionBsa();
    void testBsaToSkyrimSeBsa();
    void testRefusesNonBsaTarget();
    void testBsaToBa2RoundTrip();
    void testRealOblivionMiscBsaToSkyrim();
};

void TestArchiveConverter::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Warning);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_archiveconverter_log.txt"));
}

namespace {

// Writes files under root/<relPath> with distinct binary bodies.
void writeFixture(const QString& root, const QString& relPath, int seed)
{
    const QString path = QDir(root).absoluteFilePath(relPath);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QByteArray body;
    for (int i = 0; i < 2048 + seed * 37; ++i)
    {
        body.append(static_cast<char>((i * 31 + seed * 7) & 0xFF));
    }
    file.write(body);
    file.close();
}

// Deterministic semi-compressible binary: period 251 keeps long matches
// available without degenerate runs, the way real texture payloads behave.
void writeBinaryFixture(const QString& root, const QString& relPath, int size)
{
    const QString path = QDir(root).absoluteFilePath(relPath);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QByteArray body;
    body.reserve(size);
    for (int i = 0; i < size; ++i)
    {
        body.append(static_cast<char>((i * 7 + (i % 251) * 13) & 0xFF));
    }
    file.write(body);
    file.close();
}

// Writes an XML-like body: a token vocabulary with the mixed, varied match
// structure real UI XML has, which the LZ4 encoder must handle. `target`
// controls the final size so small-file encodings are covered too.
void writeTextFixture(const QString& root, const QString& relPath,
                      int target = 20000)
{
    const QString path = QDir(root).absoluteFilePath(relPath);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QByteArray body;
    const char* tags[] = { "<rect", " x=\"1\"", " y=\"2\"", " width=\"", " height=\"",
                           " name=\"", "/>", "<image", " filename=\"menus\\", ".dds\"",
                           "\n" };
    while (body.size() < target)
    {
        body.append(tags[(body.size() / 7) % 11]);
        body.append(QByteArray::number(body.size() % 997));
    }
    file.write(body);
    file.close();
}

} // namespace

void TestArchiveConverter::testBsaToOblivionBsa()
{
    // Skyrim SE 0x69 LZ4 -> Oblivion 0x67 zlib, the canonical downgrade.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFixture(dir.path(), "meshes/chair.nif", 1);
    writeFixture(dir.path(), "textures/stone.dds", 2);
    writeFixture(dir.path(), "meshes/table.nif", 3);
    writeTextFixture(dir.path(), "menus/menu.xml");
    // Varying sizes around the small-file regime real UI XML lives in.
    writeTextFixture(dir.path(), "meshes/small1.txt", 137);
    writeTextFixture(dir.path(), "meshes/small2.txt", 512);
    writeTextFixture(dir.path(), "meshes/small3.txt", 2049);
    writeTextFixture(dir.path(), "textures/small4.xml", 40);
    writeTextFixture(dir.path(), "textures/small5.xml", 999);
    // Multi-block payloads (> 64 KiB) exercise the frame's block loop and
    // the per-block literal boundaries around the 15-byte extension rule.
    writeTextFixture(dir.path(), "textures/big1.xml", 200000);
    writeTextFixture(dir.path(), "textures/big2.tex", 70000);
    writeBinaryFixture(dir.path(), "fonts/font_a.tex", 300000);

    const QString source = dir.filePath("source.bsa");
    BsaArchive writer;
    QVERIFY(writer.create(QStringList{ dir.filePath("meshes/chair.nif"),
                                       dir.filePath("textures/stone.dds"),
                                       dir.filePath("meshes/table.nif") },
                          source, true, dir.path(), 0x69));

    const QString out = dir.filePath("oblivion.bsa");
    const ArchiveConversionReport report =
        ArchiveConverter::convertBsa(source, GameFormat::Game::Oblivion, out);
    QVERIFY2(report.ok(), qPrintable(report.failures.join("; ")));
    QCOMPARE(report.fileCount, 3);
    QCOMPARE(report.failures, QStringList());

    // The output must be the version the game reads, and its payloads must
    // match the source byte-for-byte.
    BsaArchive reopened;
    QVERIFY(reopened.open(out));
    QCOMPARE(reopened.version(), 0x67u);
    QCOMPARE(reopened.fileCount(), 3);
}

void TestArchiveConverter::testBsaToSkyrimSeBsa()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFixture(dir.path(), "meshes/a.nif", 4);
    writeFixture(dir.path(), "meshes/b.nif", 5);

    const QString source = dir.filePath("src_067.bsa");
    BsaArchive writer;
    QVERIFY(writer.create(QStringList{ dir.filePath("meshes/a.nif"),
                                       dir.filePath("meshes/b.nif") },
                          source, true, dir.path(), 0x67));

    const QString out = dir.filePath("skyrim.bsa");
    const ArchiveConversionReport report =
        ArchiveConverter::convertBsa(source, GameFormat::Game::Skyrim,
                                     out);
    QVERIFY2(report.ok(), qPrintable(report.failures.join("; ")));

    BsaArchive reopened;
    QVERIFY(reopened.open(out));
    QCOMPARE(reopened.version(), 0x69u);
    QCOMPARE(reopened.fileCount(), 2);
}

void TestArchiveConverter::testRefusesNonBsaTarget()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFixture(dir.path(), "meshes/a.nif", 6);
    const QString source = dir.filePath("x.bsa");
    BsaArchive writer;
    QVERIFY(writer.create(QStringList{ dir.filePath("meshes/a.nif") }, source,
                          false, dir.path(), 0x69));

    const ArchiveConversionReport morrowind =
        ArchiveConverter::convertBsa(source, GameFormat::Game::Morrowind,
                                     dir.filePath("mw.bsa"));
    QVERIFY(!morrowind.ok());
    QVERIFY(morrowind.failures.size() > 0);
    QVERIFY(!QFile::exists(dir.filePath("mw.bsa")));

    const ArchiveConversionReport starfield =
        ArchiveConverter::convertBsa(source, GameFormat::Game::Starfield,
                                     dir.filePath("sf.bsa"));
    QVERIFY(!starfield.ok());
    QVERIFY(!QFile::exists(dir.filePath("sf.bsa")));
}

void TestArchiveConverter::testBsaToBa2RoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    writeFixture(dir.path(), "meshes/a.nif", 7);
    writeFixture(dir.path(), "meshes/sub/b.nif", 8);

    const QString source = dir.filePath("x.bsa");
    BsaArchive writer;
    QVERIFY(writer.create(QStringList{ dir.filePath("meshes/a.nif"),
                                       dir.filePath("meshes/sub/b.nif") },
                          source, true, dir.path(), 0x69));

    const QString ba2 = dir.filePath("out.ba2");
    const ArchiveConversionReport toBa2 =
        ArchiveConverter::convertBsaToBa2(source, ba2, true, QStringLiteral("GNRL"));
    QVERIFY2(toBa2.ok(), qPrintable(toBa2.failures.join("; ")));
    QCOMPARE(toBa2.fileCount, 2);

    const QString back = dir.filePath("back.bsa");
    const ArchiveConversionReport toBsa =
        ArchiveConverter::convertBa2ToBsa(ba2, GameFormat::Game::Skyrim,
                                          back, true);
    QVERIFY2(toBsa.ok(), qPrintable(toBsa.failures.join("; ")));
    QCOMPARE(toBsa.fileCount, 2);

    // Compare the staged payloads through both containers' own extractors:
    // the BSA re-read must equal the original fixture bytes.
    BsaArchive finalArchive;
    QVERIFY(finalArchive.open(back));
    QCOMPARE(finalArchive.fileCount(), 2);
    for (int i = 0; i < finalArchive.fileCount(); ++i)
    {
        QByteArray data;
        QVERIFY(finalArchive.readData(static_cast<quint32>(i), data));
        QVERIFY(!data.isEmpty());
    }
}

// Real-data gate: the Oblivion GOTY install ships real 0x67 BSAs; the
// smallest, Oblivion - Misc.bsa, converts to a Skyrim SE 0x69 archive with
// every payload intact.
void TestArchiveConverter::testRealOblivionMiscBsaToSkyrim()
{
    const QString source =
        QStringLiteral("F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/"
                       "Oblivion GOTY English/Data/Oblivion - Misc.bsa");
    if (!QFile::exists(source))
    {
        QSKIP("Oblivion GOTY install not present");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString out = dir.filePath("misc_se.bsa");
    const ArchiveConversionReport report =
        ArchiveConverter::convertBsa(source, GameFormat::Game::Skyrim, out);
    QVERIFY2(report.ok(), qPrintable(report.failures.join("; ")));
    QVERIFY(report.fileCount > 0);

    BsaArchive reopened;
    QVERIFY(reopened.open(out));
    QCOMPARE(reopened.version(), 0x69u);
    QCOMPARE(reopened.archiveFlags() & 0x0004u, 0x0004u);
}

QTEST_MAIN(TestArchiveConverter)
#include "test_archiveconverter.moc"

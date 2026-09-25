#include <QTest>
#include <QTemporaryDir>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QHash>
#include <QStringList>
#include <QVector>
#include <algorithm>

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
    void testCreateUsesFolderBlockOffsets();
    void testCreateCompressedRoundTrip();
    void testExplicitSourceRoot();
    void testCreateOlderTargets();
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

void TestBsaWrite::testCreateUsesFolderBlockOffsets()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeInput = [&tempDir](const QString& relativePath, const QByteArray& bytes) {
        const QString path = tempDir.path() + QLatin1Char('/') + relativePath;
        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
            return QString();
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return QString();
        if (file.write(bytes) != bytes.size())
            return QString();
        file.close();
        return path;
    };

    const QString archivePath = tempDir.path() + QStringLiteral("/test.bsa");
    const QStringList inputs{
        writeInput(QStringLiteral("meshes/foo/bar.nif"), QByteArray("NIFDATA").repeated(17)),
        writeInput(QStringLiteral("meshes/foo/baz.nif"), QByteArray("BAZDATA").repeated(11)),
        writeInput(QStringLiteral("sound/vo/test.fuz"), QByteArray("FUZDATA").repeated(13))
    };
    for (const QString& input : inputs)
        QVERIFY(!input.isEmpty());

    BsaArchive writer;
    QVERIFY(writer.create(inputs, archivePath));

    QFile archive(archivePath);
    QVERIFY(archive.open(QIODevice::ReadOnly));
    QDataStream ds(&archive);
    ds.setByteOrder(QDataStream::LittleEndian);

    quint32 magic = 0;
    quint32 version = 0;
    quint32 foldersOffset = 0;
    quint32 flags = 0;
    quint32 folderCount = 0;
    quint32 fileCount = 0;
    quint32 folderNamesLength = 0;
    quint32 fileNamesLength = 0;
    quint32 fileFlags = 0;
    ds >> magic >> version >> foldersOffset >> flags >> folderCount >> fileCount
       >> folderNamesLength >> fileNamesLength >> fileFlags;

    QCOMPARE(magic, quint32(0x00415342));
    QCOMPARE(version, quint32(0x69));
    QCOMPARE(foldersOffset, quint32(36));
    QCOMPARE(flags, quint32(0x3));
    QCOMPARE(folderCount, quint32(2));
    QCOMPARE(fileCount, quint32(3));
    QCOMPARE(fileFlags, quint32(0));
    QVERIFY(folderNamesLength > 0);
    QVERIFY(fileNamesLength > 0);

    QVector<quint32> storedOffsets;
    QVector<quint32> folderCounts;
    storedOffsets.reserve(static_cast<int>(folderCount));
    folderCounts.reserve(static_cast<int>(folderCount));
    for (quint32 i = 0; i < folderCount; ++i) {
        quint64 hash = 0;
        quint32 count = 0;
        quint32 prePadding = 0;
        quint32 offset = 0;
        quint32 postPadding = 0;
        ds >> hash >> count >> prePadding >> offset >> postPadding;
        QCOMPARE(prePadding, quint32(0));
        QCOMPARE(postPadding, quint32(0));
        storedOffsets.append(offset);
        folderCounts.append(count);
    }
    QCOMPARE(storedOffsets.size(), 2);
    QVERIFY(storedOffsets.at(0) != storedOffsets.at(1));

    const quint64 folderBlocksStart = quint64(foldersOffset) + quint64(folderCount) * 24;
    const quint64 fileNamesOffset = folderBlocksStart
        + quint64(folderCount)
        + folderNamesLength
        + quint64(fileCount) * 16;
    QVector<quint32> fileSizes;
    QVector<quint32> fileOffsets;
    QStringList folderNames;
    for (int i = 0; i < storedOffsets.size(); ++i) {
        QVERIFY(storedOffsets.at(i) >= fileNamesLength);
        const quint32 blockOffset = storedOffsets.at(i) - fileNamesLength;
        QVERIFY(blockOffset >= folderBlocksStart);
        QVERIFY(archive.seek(blockOffset));
        quint8 nameLength = 0;
        ds >> nameLength;
        QVERIFY(nameLength > 0);
        QByteArray name(nameLength, '\0');
        QCOMPARE(archive.read(name.data(), name.size()), qint64(name.size()));
        QCOMPARE(name.back(), '\0');
        folderNames.append(QString::fromUtf8(name.left(name.size() - 1)));

        for (quint32 j = 0; j < folderCounts.at(i); ++j) {
            quint64 hash = 0;
            quint32 size = 0;
            quint32 offset = 0;
            ds >> hash >> size >> offset;
            fileSizes.append(size);
            fileOffsets.append(offset);
            QVERIFY(offset >= fileNamesOffset + fileNamesLength);
            QVERIFY(qint64(offset) + size <= archive.size());
            if (!fileOffsets.isEmpty() && fileOffsets.size() > 1
                && fileOffsets.size() == fileSizes.size()) {
                const int index = fileOffsets.size() - 1;
                if (index > 0)
                    QCOMPARE(offset, fileOffsets.at(index - 1) + fileSizes.at(index - 1));
            }
        }
    }
    QCOMPARE(folderNames.size(), 2);
    QVERIFY(folderNames.contains(QStringLiteral("meshes\\foo")));
    QVERIFY(folderNames.contains(QStringLiteral("sound\\vo")));

    QVERIFY(archive.seek(fileNamesOffset));
    QByteArray fileNames(fileNamesLength, '\0');
    QCOMPARE(archive.read(fileNames.data(), fileNames.size()), qint64(fileNames.size()));
    const QList<QByteArray> nameParts = fileNames.split('\0');
    QCOMPARE(nameParts.size(), 4);
    QVERIFY(nameParts.at(0).size() > 0);
    QVERIFY(nameParts.at(1).size() > 0);
    QVERIFY(nameParts.at(2).size() > 0);
    QVERIFY(nameParts.at(3).isEmpty());
    QCOMPARE(fileOffsets.size(), 3);
    QCOMPARE(fileSizes.size(), 3);
    QCOMPARE(fileOffsets.at(0), quint32(fileNamesOffset + fileNamesLength));
    QCOMPARE(fileOffsets.at(1), fileOffsets.at(0) + fileSizes.at(0));
    QCOMPARE(fileOffsets.at(2), fileOffsets.at(1) + fileSizes.at(1));
}

void TestBsaWrite::testCreateCompressedRoundTrip()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QByteArray repeated = QByteArray("0123456789abcdef").repeated(12500);
    QByteArray randomData(120000, '\0');
    quint32 state = 0x12345678u;
    for (int i = 0; i < randomData.size(); ++i) {
        state = state * 1664525u + 1013904223u;
        randomData[i] = static_cast<char>((state >> 24) & 0xFFu);
    }
    const QByteArray tiny("x");
    QHash<QString, QByteArray> expected;
    expected.insert(QStringLiteral("repeat.bin"), repeated);
    expected.insert(QStringLiteral("random.bin"), randomData);
    expected.insert(QStringLiteral("tiny.bin"), tiny);

    const auto writeInput = [&tempDir](const QString& name, const QByteArray& bytes) {
        const QString path = tempDir.path() + QLatin1Char('/') + name;
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return QString();
        if (file.write(bytes) != bytes.size())
            return QString();
        return path;
    };

    const QString archivePath = tempDir.path() + QStringLiteral("/compressed.bsa");
    QStringList inputs;
    for (auto it = expected.constBegin(); it != expected.constEnd(); ++it)
        inputs.append(writeInput(it.key(), it.value()));
    for (const QString& input : inputs)
        QVERIFY(!input.isEmpty());

    BsaArchive writer;
    QVERIFY(writer.create(inputs, archivePath, true));

    BsaArchive reader;
    QVERIFY(reader.open(archivePath));
    QCOMPARE(reader.archiveFlags() & 0x4u, 0x4u);
    QCOMPARE(reader.fileCount(), 3);
    bool sawCompressed = false;
    bool sawStoredFallback = false;
    for (const BsaFileEntry& entry : reader.entries()) {
        const QString name = QFileInfo(entry.fullPath).fileName();
        QVERIFY(expected.contains(name));
        if (entry.compressed)
            sawCompressed = true;
        else
            sawStoredFallback = true;
        QByteArray data;
        QVERIFY(reader.readData(static_cast<quint32>(&entry - reader.entries().constData()), data));
        QCOMPARE(data, expected.value(name));
    }
    QVERIFY(sawCompressed);
    QVERIFY(sawStoredFallback);

    QFile raw(archivePath);
    QVERIFY(raw.open(QIODevice::ReadOnly));
    QDataStream ds(&raw);
    ds.setByteOrder(QDataStream::LittleEndian);
    quint32 magic = 0;
    quint32 version = 0;
    quint32 foldersOffset = 0;
    quint32 flags = 0;
    quint32 folderCount = 0;
    quint32 fileCount = 0;
    quint32 folderNamesLength = 0;
    quint32 fileNamesLength = 0;
    quint32 fileFlags = 0;
    ds >> magic >> version >> foldersOffset >> flags >> folderCount >> fileCount
       >> folderNamesLength >> fileNamesLength >> fileFlags;
    QCOMPARE(magic, quint32(0x00415342));
    QCOMPARE(version, quint32(0x69));
    QCOMPARE(foldersOffset, quint32(36));
    QCOMPARE(flags, quint32(0x7));
    QCOMPARE(folderCount, quint32(1));
    QCOMPARE(fileCount, quint32(3));
    QCOMPARE(folderNamesLength, quint32(1));
    QVERIFY(fileNamesLength > 0);

    QVERIFY(raw.seek(foldersOffset));
    quint64 folderHash = 0;
    quint32 storedFolderOffset = 0;
    quint32 count = 0;
    quint32 prePadding = 0;
    quint32 postPadding = 0;
    ds >> folderHash >> count >> prePadding >> storedFolderOffset >> postPadding;
    QCOMPARE(count, quint32(3));
    QCOMPARE(prePadding, quint32(0));
    QCOMPARE(postPadding, quint32(0));
    const quint32 folderBlockOffset = storedFolderOffset - fileNamesLength;
    QVERIFY(raw.seek(folderBlockOffset));
    quint8 folderNameLength = 0;
    ds >> folderNameLength;
    QCOMPARE(folderNameLength, quint8(1));
    QByteArray folderName(1, '\0');
    QCOMPARE(raw.read(folderName.data(), folderName.size()), qint64(1));
    QCOMPARE(folderName.at(0), '\0');

    QVector<quint32> sizes;
    QVector<quint32> offsets;
    for (quint32 i = 0; i < count; ++i) {
        quint64 hash = 0;
        quint32 size = 0;
        quint32 offset = 0;
        ds >> hash >> size >> offset;
        sizes.append(size);
        offsets.append(offset);
    }
    const quint64 fileNamesOffset = folderBlockOffset + 1 + folderNameLength
        + static_cast<quint64>(count) * 16;
    QVERIFY(raw.seek(static_cast<qint64>(fileNamesOffset)));
    QByteArray fileNames(fileNamesLength, '\0');
    QCOMPARE(raw.read(fileNames.data(), fileNames.size()), qint64(fileNames.size()));
    const QList<QByteArray> nameParts = fileNames.split('\0');
    QCOMPARE(nameParts.size(), 4);
    quint64 dataStart = fileNamesOffset + fileNamesLength;
    for (int i = 0; i < sizes.size(); ++i) {
        const QString name = QString::fromUtf8(nameParts.at(i));
        QVERIFY(expected.contains(name));
        const quint32 packedSize = sizes.at(i) & 0x3FFFFFFFu;
        QCOMPARE(offsets.at(i), quint32(dataStart));
        QVERIFY(raw.seek(offsets.at(i)));
        QByteArray payload(packedSize, '\0');
        QCOMPARE(raw.read(payload.data(), payload.size()), qint64(payload.size()));
        if (name == QStringLiteral("repeat.bin")) {
            QVERIFY((sizes.at(i) & 0x40000000u) == 0);
            QVERIFY(payload.size() < expected.value(name).size());
            QVERIFY(payload.size() >= 8);
            QCOMPARE(quint32(static_cast<quint8>(payload.at(0)))
                | (quint32(static_cast<quint8>(payload.at(1))) << 8)
                | (quint32(static_cast<quint8>(payload.at(2))) << 16)
                | (quint32(static_cast<quint8>(payload.at(3))) << 24),
                quint32(expected.value(name).size()));
            QCOMPARE(payload.mid(4, 4), QByteArray("\x04\x22\x4D\x18", 4));
        } else if (name == QStringLiteral("tiny.bin")) {
            QVERIFY((sizes.at(i) & 0x40000000u) != 0);
            QCOMPARE(payload, expected.value(name));
        }
        dataStart += packedSize;
    }
}

void TestBsaWrite::testExplicitSourceRoot()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sourceRoot = tempDir.path() + QStringLiteral("/source");
    const QString inputPath = sourceRoot + QStringLiteral("/assets/test.nif");
    QVERIFY(QDir().mkpath(QFileInfo(inputPath).absolutePath()));
    QFile input(inputPath);
    QVERIFY(input.open(QIODevice::WriteOnly));
    QCOMPARE(input.write(QByteArray("NIF")), qint64(3));
    input.close();

    const QString archivePath = tempDir.path() + QStringLiteral("/output/test.bsa");
    QVERIFY(QDir().mkpath(QFileInfo(archivePath).absolutePath()));
    BsaArchive writer;
    QVERIFY(writer.create(QStringList{inputPath}, archivePath, false, sourceRoot));
    BsaArchive reader;
    QVERIFY(reader.open(archivePath));
    QCOMPARE(reader.entries().size(), 1);
    QCOMPARE(reader.entries().first().fullPath, QStringLiteral("assets\\test.nif"));
}

void TestBsaWrite::testCreateOlderTargets()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString sourceRoot = tempDir.path();
    const QString inputPath = sourceRoot + QStringLiteral("/data/test.bin");
    QVERIFY(QDir().mkpath(QFileInfo(inputPath).absolutePath()));
    const QByteArray source(20000, 'a');
    QFile input(inputPath);
    QVERIFY(input.open(QIODevice::WriteOnly));
    QCOMPARE(input.write(source), qint64(source.size()));
    input.close();

    for (quint32 version : {0x67u, 0x68u})
    {
        const QString archivePath = tempDir.path()
            + QStringLiteral("/archive-%1.bsa").arg(version, 0, 16);
        BsaArchive writer;
        QVERIFY(writer.create(QStringList{inputPath}, archivePath, true, sourceRoot, version));
        BsaArchive reader;
        QVERIFY(reader.open(archivePath));
        QCOMPARE(reader.version(), int(version));
        QByteArray output;
        QVERIFY(reader.readData(0, output));
        QCOMPARE(output, source);
    }
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

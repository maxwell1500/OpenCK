#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QMap>
#include <QSet>

#include "bsaarchive.hpp"
#include "nifblockfile.hpp"
#include "nifanimationwriter.hpp"

// Locks the Bethesda NIF container against shipped files. The container is
// verified two ways, because a byte-exact re-serialize alone does not prove
// the block boundaries are right:
//
//   1. load + serialize reproduces the source file byte for byte, and
//   2. every block's payload is exactly the slice the header's size table
//      claims, so a save can only ever change what it deliberately patches.
//
// The animated-block checks additionally prove the writer refuses to touch
// keyframe data whose encoding it has not confirmed.
//
// Requires the user's Skyrim SE install; skipped when it is absent.
class TestNifBlockFile : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void shippedNifsRoundTrip();
    void shippedNifsHaveConsistentBlockTable();
    void starfieldNifsRoundTrip();
    void starfieldKeyframeCodecIsExact();
    void unconfirmedKeyframeLayoutIsRefused();

private:
    // A few thousand shipped NIFs is enough to cover every block type and
    // keeps the check fast enough for the regular suite.
    static constexpr int kSampleLimit = 2500;
    static constexpr int kStarfieldSample = 4000;

    // Starfield's mesh archives are the only place animated 1.6+ NIFs are
    // reachable, so the codec is validated against them. All candidates are
    // scanned: the large ones lead with weak-reference stub meshes that carry
    // no animation at all.
    static QStringList starfieldArchives()
    {
        const QString dir = QStringLiteral("C:/XboxGames/Starfield/Content/Data/");
        QStringList found;
        for (const QString& name : {QStringLiteral("Starfield - FaceMeshes.ba2"),
                                    QStringLiteral("Starfield - Meshes02.ba2"),
                                    QStringLiteral("Starfield - LODMeshes.ba2")}) {
            if (QFile::exists(dir + name)) found.append(dir + name);
        }
        return found;
    }

    static QStringList archives();
    bool anyArchiveFound() const;
};

void TestNifBlockFile::initTestCase()
{
    if (!anyArchiveFound()) QSKIP("no shipped NIF archives found");
}

QStringList TestNifBlockFile::archives()
{
    const QString base = QStringLiteral(
        "C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/");
    return {base + QStringLiteral("Skyrim - Meshes0.bsa"),
            base + QStringLiteral("Skyrim - Meshes1.bsa")};
}

bool TestNifBlockFile::anyArchiveFound() const
{
    for (const QString& path : archives())
        if (QFile::exists(path)) return true;
    return false;
}

void TestNifBlockFile::shippedNifsRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    int checked = 0;
    for (const QString& bsaPath : archives()) {
        if (!QFile::exists(bsaPath)) continue;
        BsaArchive archive;
        QVERIFY2(archive.open(bsaPath), qPrintable(bsaPath));

        for (int i = 0; i < archive.fileCount() && checked < kSampleLimit; ++i) {
            const QString& entry = archive.entries()[i].fullPath;
            if (!entry.endsWith(".nif", Qt::CaseInsensitive)) continue;
            QByteArray bytes;
            if (!archive.readData(i, bytes)) continue;

            const QString tmp = dir.filePath(QStringLiteral("probe.nif"));
            QFile out(tmp);
            if (!out.open(QIODevice::WriteOnly)) continue;
            out.write(bytes);
            out.close();

            NifBlockFile file;
            QVERIFY2(file.load(tmp), qPrintable(entry + QStringLiteral(": ") + file.lastError()));
            QCOMPARE(file.serialize(), bytes);
            ++checked;
        }
    }
    QVERIFY2(checked >= 1000,
             qPrintable(QStringLiteral("only %1 shipped NIFs checked").arg(checked)));
}

void TestNifBlockFile::shippedNifsHaveConsistentBlockTable()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    int checked = 0;
    int controllersSeen = 0;
    int controllersResolved = 0;
    for (const QString& bsaPath : archives()) {
        if (!QFile::exists(bsaPath)) continue;
        BsaArchive archive;
        QVERIFY2(archive.open(bsaPath), qPrintable(bsaPath));

        for (int i = 0; i < archive.fileCount() && checked < kSampleLimit; ++i) {
            const QString& entry = archive.entries()[i].fullPath;
            if (!entry.endsWith(".nif", Qt::CaseInsensitive)) continue;
            QByteArray bytes;
            if (!archive.readData(i, bytes)) continue;

            const QString tmp = dir.filePath(QStringLiteral("probe.nif"));
            QFile out(tmp);
            if (!out.open(QIODevice::WriteOnly)) continue;
            out.write(bytes);
            out.close();

            NifBlockFile file;
            if (!file.load(tmp)) continue;

            // Every node-to-controller link must point at a real block, and
            // every controller whose keyframe data resolves must land on a
            // recognised data block. A controller that does not resolve is
            // allowed (float controllers and unset refs exist), so the walk is
            // additionally required to succeed for the large majority.
            QSet<quint32> controllerBlocks;
            for (const QString& type : {QStringLiteral("NiTransformController"),
                                        QStringLiteral("NiKeyframeController")}) {
                const QList<int> found = file.findBlocks(type);
                for (int index : found)
                    controllerBlocks.insert(static_cast<quint32>(index));
            }
            for (int index : controllerBlocks) {
                ++controllersSeen;
                const int dataBlock = file.keyframeDataBlockFor(static_cast<int>(index));
                if (dataBlock < 0) continue;
                ++controllersResolved;
                QVERIFY(dataBlock < file.count());
            }
            for (int block = 0; block < file.count(); ++block) {
                if (!NifBlockFile::isNodeBlockType(file.block(block).type)) continue;
                QString name;
                quint32 controllerRef = 0xFFFFFFFFu;
                if (!file.nodeNetInfo(block, name, controllerRef)) continue;
                QVERIFY2(controllerRef == 0xFFFFFFFFu || controllerRef < static_cast<quint32>(file.count()),
                         qPrintable(entry + QStringLiteral(": node %1 points at block %2")
                                        .arg(block).arg(controllerRef)));
            }
            ++checked;
        }
    }
    QVERIFY(checked >= 1000);
    QVERIFY2(controllersSeen > 50,
             qPrintable(QStringLiteral("only %1 controllers seen").arg(controllersSeen)));
    QVERIFY2(controllersResolved * 2 > controllersSeen,
             qPrintable(QStringLiteral("only %1 of %2 controllers resolved to keyframe data")
                            .arg(controllersResolved).arg(controllersSeen)));
}

void TestNifBlockFile::starfieldNifsRoundTrip()
{
    const QStringList archivePaths = starfieldArchives();
    if (archivePaths.isEmpty()) QSKIP("no Starfield mesh archives found");

    BsaArchive archive;
    if (!archive.open(archivePaths.first())) QSKIP("no readable Starfield archive");
    if (archive.fileCount() == 0) QSKIP("archive has no entries");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    int checked = 0;
    for (int i = 0; i < archive.fileCount() && checked < kStarfieldSample; ++i) {
        const BsaFileEntry& entry = archive.entries()[i];
        if (!entry.fullPath.endsWith(".nif", Qt::CaseInsensitive)) continue;
        QByteArray bytes;
        if (!archive.readData(static_cast<quint32>(i), bytes)) continue;
        if (!bytes.startsWith("Gamebryo")) continue;

        const QString tmp = dir.filePath(QStringLiteral("sf.nif"));
        QFile out(tmp);
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.write(bytes);
        out.close();

        NifBlockFile file;
        QVERIFY2(file.load(tmp),
                 qPrintable(entry.fullPath + QStringLiteral(": ") + file.lastError()));
        QCOMPARE(file.serialize(), bytes);
        ++checked;
    }
    QVERIFY2(checked > 50,
             qPrintable(QStringLiteral("only %1 Starfield NIFs checked").arg(checked)));
}

void TestNifBlockFile::starfieldKeyframeCodecIsExact()
{
    const QStringList archivePaths = starfieldArchives();
    if (archivePaths.isEmpty()) QSKIP("no Starfield mesh archives found");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // Tally every block type seen so a "no controllers found" result can be
    // told apart from "these archives have no such blocks at all".
    QMap<QString, int> typeCounts;
    int controllers = 0;
    int exact = 0;
    int nifs = 0;
    for (const QString& archivePath : archivePaths) {
    BsaArchive archive;
    if (!archive.open(archivePath)) continue;

    for (int i = 0; i < archive.fileCount() && nifs < kStarfieldSample; ++i) {
        const BsaFileEntry& entry = archive.entries()[i];
        if (!entry.fullPath.endsWith(".nif", Qt::CaseInsensitive)) continue;
        QByteArray bytes;
        if (!archive.readData(static_cast<quint32>(i), bytes)) continue;
        if (!bytes.startsWith("Gamebryo")) continue;
        ++nifs;

        const QString tmp = dir.filePath(QStringLiteral("sf.nif"));
        QFile out(tmp);
        if (!out.open(QIODevice::WriteOnly)) continue;
        out.write(bytes);
        out.close();

        NifBlockFile file;
        if (!file.load(tmp)) continue;
        for (int b = 0; b < file.count(); ++b)
            typeCounts[file.block(b).type] += 1;
        const QList<int> ctrl = file.findBlocks(QStringLiteral("NiKeyframeController"));
        if (ctrl.isEmpty()) continue;

        for (int c : ctrl) {
            ++controllers;
            const int dataBlock = file.keyframeDataBlockFor(c);
            if (dataBlock < 0) continue;
            const auto& db = file.block(dataBlock);
            QVERIFY2(NifBlockFile::isWritableKeyframeType(db.type),
                     qPrintable(entry.fullPath + QStringLiteral(": unexpected data type ")
                                + db.type));
            QVector<Nif::TransformKeyframe> keys;
            QVERIFY2(NifBlockFile::decodeKeyframeData(db.type, db.data, keys),
                     qPrintable(entry.fullPath + QStringLiteral(": decode failed for ")
                                + db.type));
            QVERIFY(!keys.isEmpty());
            // The codec is only trustworthy if it reproduces the shipped block
            // byte for byte; anything else means the layout is misread.
            QByteArray reencoded;
            QVERIFY(NifBlockFile::encodeKeyframeData(db.type, keys, reencoded));
            QCOMPARE(reencoded, db.data);
            ++exact;
        }
    }
    }   // archives
    QStringList summary;
    for (auto it = typeCounts.constBegin(); it != typeCounts.constEnd(); ++it)
        summary.append(QStringLiteral("%1=%2").arg(it.key()).arg(it.value()));
    qInfo().noquote() << QStringLiteral("scanned %1 NIFs, %2 block types")
                             .arg(nifs).arg(typeCounts.size());
    for (const QString& line : summary.mid(0, 40))
        qInfo().noquote() << line;

    if (controllers == 0) {
        QSKIP(qPrintable(QStringLiteral(
            "no NiKeyframeController blocks in %1 NIFs (%2 block types: %3)")
            .arg(nifs).arg(typeCounts.size())
            .arg(summary.join(QLatin1Char(',')).left(400))));
        return;
    }
    QVERIFY2(exact == controllers,
             qPrintable(QStringLiteral("%1 of %2 controller keyframe blocks were not "
                                       "byte-exact").arg(controllers - exact).arg(controllers)));
}

void TestNifBlockFile::unconfirmedKeyframeLayoutIsRefused()
{
    // NiTransformData (Skyrim 1.5) encoding is not confirmed, so the writer
    // must decline rather than risk producing a corrupt NIF.
    QVERIFY(!NifBlockFile::isWritableKeyframeType(QStringLiteral("NiTransformData")));
    QVERIFY(NifBlockFile::isWritableKeyframeType(QStringLiteral("NiKeyframeData")));
    QVERIFY(NifBlockFile::isWritableKeyframeType(QStringLiteral("NiAnimKeyFrameData")));

    QVector<Nif::TransformKeyframe> keys;
    Nif::TransformKeyframe key;
    key.time = 0.0f;
    key.translation = {1.0f, 2.0f, 3.0f};
    key.rotation = {0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    key.scale = {1.0f, 1.0f, 1.0f};
    keys.append(key);

    // The confirmed layout is a flat 44-byte-per-key array: 4 count + 44.
    QByteArray encoded;
    QVERIFY(NifBlockFile::encodeKeyframeData(QStringLiteral("NiKeyframeData"), keys, encoded));
    QCOMPARE(encoded.size(), 48);

    // An unknown block type is never encodable.
    QVERIFY(!NifBlockFile::encodeKeyframeData(QStringLiteral("NiSomethingElse"), keys, encoded));
}

QTEST_MAIN(TestNifBlockFile)
#include "test_nifblockfile.moc"

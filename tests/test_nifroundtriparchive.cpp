// Byte-exact round-trip validation of NifBlockFile over a whole shipped
// mesh archive.
//
// NifBlockFile used to accept exactly one header line, "Gamebryo File Format,
// Version 20.2.0.7", which is what Skyrim writes. Oblivion meshes use 20.0.0.4
// and the same archive also holds 20.0.0.5, 10.1.0.101, 10.1.0.106 and
// 10.2.0.0, so every one of them was rejected and OpenCK could not read a
// single Oblivion NIF. It also assumed the 20.2.x field set, which turned out
// to be wrong for every one of those older generations: the endian_type byte,
// the header string table and the per-block size table were all added
// part-way through the Gamebryo line, and an older file has none of them.
//
// This walks an entire archive and requires that a load followed by a save
// reproduces the original bytes exactly, for every Gamebryo file in it.
// Anything less is a real fidelity failure, not a tolerated difference.
//
// The archive is on a normal local drive, so this is fully reproducible and has
// none of the on-demand eviction problems the Skyrim archives have.

#include <QTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QSet>
#include <QTemporaryDir>

#include "../../libs/files/ba2/bsaarchive.hpp"
#include "../../libs/files/nif/nifblockfile.hpp"
#include "../../libs/files/log/logger.hpp"

class TestNifRoundTripArchive : public QObject
{
    Q_OBJECT

private:
    static QString archivePath()
    {
        return QStringLiteral(
            "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/"
            "Oblivion GOTY English/Data/Oblivion - Meshes.bsa");
    }

    // Set OPENCK_TEST_NIF_DUMP_FAILURE to a path to capture the first NIF that
    // fails to parse and stop the walk, or OPENCK_TEST_NIF_DUMP_SMALLEST to a
    // path that ends up holding the smallest NIF in the archive. The archive is
    // a build input that only exists on a machine with the game installed, so
    // when a layout question comes up the bytes have to be taken away and
    // decoded offline rather than guessed at. OPENCK_TEST_NIF_DUMP_NAME narrows
    // the capture to entries whose path contains that text, which is how one
    // particular failure class gets pulled out of the archive.
    static bool maybeStopAtFailure(const QString& name, const QByteArray& bytes)
    {
        const QByteArray target = qgetenv("OPENCK_TEST_NIF_DUMP_FAILURE");
        if (target.isEmpty()) return false;
        const QByteArray wanted = qgetenv("OPENCK_TEST_NIF_DUMP_NAME");
        if (!wanted.isEmpty() && !name.contains(QString::fromLocal8Bit(wanted)))
            return false;
        QFile out(QString::fromLocal8Bit(target));
        if (!out.open(QIODevice::WriteOnly)) return false;
        out.write(bytes);
        out.close();
        qWarning().noquote() << "captured first failing NIF" << name
                             << "to" << out.fileName();
        return true;
    }

    static void keepSmallest(const QByteArray& bytes, int& smallestSoFar)
    {
        if (smallestSoFar != 0 && bytes.size() >= smallestSoFar) return;
        const QByteArray target = qgetenv("OPENCK_TEST_NIF_DUMP_SMALLEST");
        if (target.isEmpty()) return;
        smallestSoFar = bytes.size();
        QFile out(QString::fromLocal8Bit(target));
        if (!out.open(QIODevice::WriteOnly)) return;
        out.write(bytes);
    }

public:
    void initTestCase()
    {
        OpenCK::Logging::Logger::instance().setMinLevel(
            OpenCK::Logging::LogLevel::Error);
    }

private slots:
    void roundTripsEveryNifInTheArchive()
    {
        const QString path = archivePath();
        if (!QFileInfo::exists(path))
            QSKIP("Oblivion meshes archive not found");

        BsaArchive archive;
        QVERIFY2(archive.open(path), "could not open the archive");

        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString probe = temp.filePath(QStringLiteral("probe.nif"));

        int attempted = 0;
        int roundTripped = 0;
        int opaqueBlocks = 0;
        int failedToParse = 0;
        int gamebryo = 0;
        int netImmerse = 0;
        int smallestNifSize = 0;
        QMap<QString, int> versionCounts;
        QMap<QString, int> failureReasons;
        QMap<QString, QString> firstFailure;
        QSet<QString> extensions;

        for (int i = 0; i < archive.fileCount(); ++i) {
            const BsaFileEntry& entry = archive.entries()[i];
            if (!entry.fullPath.endsWith(".nif", Qt::CaseInsensitive))
                continue;
            ++attempted;
            const int dot = entry.fullPath.lastIndexOf('.');
            extensions.insert(entry.fullPath.mid(dot + 1).toLower());

            QByteArray bytes;
            if (!archive.readData(i, bytes)) {
                ++failedToParse;
                const QString readFailure = QStringLiteral("readData failed");
                failureReasons[readFailure] += 1;
                if (!firstFailure.contains(readFailure))
                    firstFailure[readFailure] = entry.fullPath;
                continue;
            }

            // Two containers share the .nif extension. Gamebryo is the one
            // NifBlockFile reads; the NetImmerse one is a different format from
            // the Morrowind era and is counted separately rather than being
            // allowed to hide inside a general failure count.
            if (bytes.startsWith("NetImmerse File Format")) {
                ++netImmerse;
                continue;
            }
            ++gamebryo;
            if (!bytes.startsWith("Gamebryo File Format")) {
                ++failedToParse;
                const QString reason = QStringLiteral("unrecognised container");
                failureReasons[reason] += 1;
                if (!firstFailure.contains(reason))
                    firstFailure[reason] = entry.fullPath;
                continue;
            }

            keepSmallest(bytes, smallestNifSize);

            QFile out(probe);
            if (!out.open(QIODevice::WriteOnly)) {
                QFAIL("could not write the probe file");
            }
            out.write(bytes);
            out.close();

            NifBlockFile file;
            if (!file.load(probe)) {
                ++failedToParse;
                // Bucket on the shape of the complaint, not the numbers in it:
                // every file reports different counts, so splitting on the full
                // text would list thousands of near-identical reasons.
                const QString reason =
                    file.lastError().section(':', 0, 0).section(';', 0, 0).trimmed();
                failureReasons[reason] += 1;
                if (!firstFailure.contains(reason))
                    firstFailure[reason] = entry.fullPath;
                if (maybeStopAtFailure(entry.fullPath, bytes)) return;
                continue;
            }
            versionCounts[file.headerVersion()] += 1;
            if (!file.hasIndividualBlocks()) ++opaqueBlocks;

            if (!file.save(probe)) {
                ++failedToParse;
                const QString saveFailure = QStringLiteral("save failed");
                failureReasons[saveFailure] += 1;
                if (!firstFailure.contains(saveFailure))
                    firstFailure[saveFailure] = entry.fullPath;
                continue;
            }
            QFile reread(probe);
            if (!reread.open(QIODevice::ReadOnly)) {
                QFAIL("could not re-read the saved file");
            }
            const QByteArray again = reread.readAll();
            reread.close();
            if (again != bytes) {
                ++failedToParse;
                const QString reason = QStringLiteral("round-trip differs (in %1 out %2)")
                    .arg(bytes.size()).arg(again.size());
                failureReasons[reason] += 1;
                if (!firstFailure.contains(reason))
                    firstFailure[reason] = entry.fullPath;
                continue;
            }
            ++roundTripped;
        }

        qInfo().noquote() << "attempted" << attempted
                          << "gamebryo" << gamebryo
                          << "netimmerse" << netImmerse
                          << "round-tripped" << roundTripped
                          << "failed" << failedToParse
                          << "opaque block region" << opaqueBlocks
                          << "extensions" << extensions.values();
        for (auto it = versionCounts.begin(); it != versionCounts.end(); ++it)
            qInfo().noquote() << "header version" << it.key() << it.value();
        for (auto it = failureReasons.begin(); it != failureReasons.end(); ++it)
            qInfo().noquote() << "failure:" << it.key() << it.value()
                              << "first:" << firstFailure.value(it.key());

        QVERIFY2(gamebryo > 1000,
            qPrintable(QStringLiteral("only %1 Gamebryo NIFs in the archive")
                           .arg(gamebryo)));

        // Every Gamebryo file in the archive must survive a load and save
        // byte for byte. Anything else is a real fidelity failure.
        QVERIFY2(failedToParse == 0,
            qPrintable(QStringLiteral("%1 Gamebryo NIFs did not round-trip; first was %2")
                           .arg(failedToParse)
                           .arg(failureReasons.isEmpty()
                                    ? QString()
                                    : firstFailure.value(failureReasons.firstKey()))));

        // The NetImmerse files are a separate, older container that is not
        // supported yet. They are reported, not asserted on, so that this test
        // says plainly which of the two formats is covered.
        qInfo().noquote() << "NetImmerse NIFs not covered by this test:" << netImmerse;
    }
};

QTEST_MAIN(TestNifRoundTripArchive)
#include "test_nifroundtriparchive.moc"

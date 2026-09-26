// Tests for the per-game BSA target presets.
//
// The archive writer took a raw version, so every caller had to remember which
// on-disk format each game expects. The UI in particular hard-coded
// "Skyrim SE archive" and always wrote 0x69, which is silently wrong for
// Skyrim LE. These tests pin the mapping, and — more importantly — that the
// resulting archives really do round-trip and really do carry the version that
// was asked for.

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "ba2/bsaarchive.hpp"

using GameFormat::Game;

class TestBsaTargets : public QObject
{
    Q_OBJECT

private:
    static QString writeSample(const QString& dir, const QString& name, int size)
    {
        if (!QDir().mkpath(QFileInfo(dir + "/" + name).absolutePath()))
            return QString();
        const QString path = dir + "/" + name;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly))
            return QString();
        f.write(QByteArray(size, 'z'));
        f.close();
        return path;
    }

    // Writes an archive at `version` and confirms it reads back at that version
    // with the original bytes intact.
    static void roundTripsAt(quint32 version)
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QByteArray source(20000, 'a');
        const QString inputPath = temp.path() + QStringLiteral("/data/x.bin");
        QVERIFY(QDir().mkpath(QFileInfo(inputPath).absolutePath()));
        QFile input(inputPath);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write(source), qint64(source.size()));
        input.close();

        const QString archivePath = temp.path()
            + QStringLiteral("/a-%1.bsa").arg(version, 0, 16);
        BsaArchive writer;
        QVERIFY(writer.create(QStringList{inputPath}, archivePath, true,
                              temp.path(), version));
        BsaArchive reader;
        QVERIFY(reader.open(archivePath));
        QCOMPARE(reader.version(), int(version));
        QByteArray out;
        QVERIFY(reader.readData(0, out));
        QCOMPARE(out, source);
    }

private slots:
    void oblivionUsesV67()
    {
        const auto targets = BsaArchive::targetsForGame(Game::Oblivion);
        QCOMPARE(targets.size(), 1);
        QCOMPARE(targets.first().version, 0x67u);
        QVERIFY(!targets.first().lz4);
        QVERIFY(targets.first().isDefault);
        QCOMPARE(BsaArchive::defaultVersionForGame(Game::Oblivion), 0x67u);
    }

    void fallout4UsesV68()
    {
        const auto targets = BsaArchive::targetsForGame(Game::Fallout4);
        QCOMPARE(targets.size(), 1);
        QCOMPARE(targets.first().version, 0x68u);
        QVERIFY(!targets.first().lz4);
        QCOMPARE(BsaArchive::defaultVersionForGame(Game::Fallout4), 0x68u);
    }

    // Skyrim LE and SE share the Game enum, so both must be offered with the
    // SE format as the default.
    void skyrimOffersBothLeAndSe()
    {
        const auto targets = BsaArchive::targetsForGame(Game::Skyrim);
        QCOMPARE(targets.size(), 2);
        QCOMPARE(targets.at(0).version, 0x69u);
        QVERIFY(targets.at(0).lz4);
        QVERIFY(targets.at(0).isDefault);
        QCOMPARE(targets.at(1).version, 0x68u);
        QVERIFY(!targets.at(1).lz4);
        QVERIFY(!targets.at(1).isDefault);
        QCOMPARE(BsaArchive::defaultVersionForGame(Game::Skyrim), 0x69u);
        // Exactly one target may claim to be the default.
        int defaults = 0;
        for (const auto& t : targets) if (t.isDefault) ++defaults;
        QCOMPARE(defaults, 1);
        // Every target must be labelled, or the format picker shows blanks.
        for (const auto& t : targets) QVERIFY(!t.label.isEmpty());
    }

    void morrowindAndStarfieldHaveNoBsaTarget()
    {
        // Morrowind archives are MWSA and Starfield archives are BA2.
        QVERIFY(BsaArchive::targetsForGame(Game::Morrowind).isEmpty());
        QCOMPARE(BsaArchive::defaultVersionForGame(Game::Morrowind), 0u);
        QVERIFY(BsaArchive::targetsForGame(Game::Starfield).isEmpty());
        QCOMPARE(BsaArchive::defaultVersionForGame(Game::Starfield), 0u);
        QVERIFY(BsaArchive::targetsForGame(Game::Unknown).isEmpty());
        QCOMPARE(BsaArchive::defaultVersionForGame(Game::Unknown), 0u);
    }

    void everyPresetRoundTrips()
    {
        roundTripsAt(0x67);
        roundTripsAt(0x68);
        roundTripsAt(0x69);
    }

    void createForGameUsesTheGamesOwnVersion()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString inputPath = writeSample(temp.path(),
            QStringLiteral("meshes/a/b.nif"), 4096);
        QVERIFY(!inputPath.isEmpty());

        struct Case { Game game; int expected; };
        const Case cases[] = {
            { Game::Oblivion, 0x67 },
            { Game::Fallout4, 0x68 },
            { Game::Skyrim,   0x69 },
        };
        for (const Case& c : cases)
        {
            const QString archivePath = temp.path()
                + QStringLiteral("/preset-%1.bsa").arg(c.expected, 0, 16);
            BsaArchive writer;
            QVERIFY(writer.createForGame(QStringList{inputPath}, archivePath,
                                        true, temp.path(), c.game));
            BsaArchive reader;
            QVERIFY(reader.open(archivePath));
            QCOMPARE(reader.version(), c.expected);
        }
    }

    void createForGameRefusesGamesWithoutBsa()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString inputPath = writeSample(temp.path(),
            QStringLiteral("meshes/a/b.nif"), 64);
        BsaArchive writer;
        QVERIFY(!writer.createForGame(QStringList{inputPath},
            temp.path() + QStringLiteral("/nope.bsa"), false, temp.path(),
            Game::Morrowind));
        QVERIFY(!QFileInfo::exists(temp.path() + QStringLiteral("/nope.bsa")));
        QVERIFY(!writer.createForGame(QStringList{inputPath},
            temp.path() + QStringLiteral("/nope2.bsa"), false, temp.path(),
            Game::Starfield));
    }
};

QTEST_MAIN(TestBsaTargets)
#include "test_bsatargets.moc"

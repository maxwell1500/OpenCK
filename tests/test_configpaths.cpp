#include <QtTest>
#include <QCoreApplication>

#include "../../libs/files/filepaths.hpp"

class TestConfigPaths : public QObject
{
    Q_OBJECT

private slots:
    void testGameName();
    void testDetectGameFromVersion();
    void testDataDirKey();
    void testDetectGamesAreGroupedByGame();
};

void TestConfigPaths::testGameName()
{
    QCOMPARE(FilePaths::gameName(Game_Morrowind), QString("Morrowind"));
    QCOMPARE(FilePaths::gameName(Game_Oblivion), QString("Oblivion"));
    QCOMPARE(FilePaths::gameName(Game_Skyrim), QString("Skyrim"));
    QCOMPARE(FilePaths::gameName(Game_SkyrimSpecialEdition), QString("Skyrim Special Edition"));
    QCOMPARE(FilePaths::gameName(Game_SkyrimAnniversaryEdition), QString("Skyrim Anniversary Edition"));
    QCOMPARE(FilePaths::gameName(Game_Fallout3), QString("Fallout 3"));
    QCOMPARE(FilePaths::gameName(Game_FalloutNewVegas), QString("Fallout: New Vegas"));
    QCOMPARE(FilePaths::gameName(Game_Fallout4), QString("Fallout 4"));
    QCOMPARE(FilePaths::gameName(Game_Starfield), QString("Starfield"));
    QCOMPARE(FilePaths::gameName(Game_NumGames), QString("Unknown"));
}

void TestConfigPaths::testDetectGameFromVersion()
{
    QCOMPARE(FilePaths::detectGameFromVersion(0.94f, 0), Game_Skyrim);
    QCOMPARE(FilePaths::detectGameFromVersion(1.0f, 0), Game_Skyrim);
    QCOMPARE(FilePaths::detectGameFromVersion(1.6f, 17), Game_SkyrimSpecialEdition);
    QCOMPARE(FilePaths::detectGameFromVersion(1.8f, 17), Game_SkyrimAnniversaryEdition);
    QCOMPARE(FilePaths::detectGameFromVersion(3.0f, 0), Game_Morrowind);
    QCOMPARE(FilePaths::detectGameFromVersion(4.0f, 0), Game_Oblivion);
    QCOMPARE(FilePaths::detectGameFromVersion(0.5f, 0), Game_None);
}

void TestConfigPaths::testDataDirKey()
{
    QCOMPARE(FilePaths::dataDirKey(Game_Skyrim), QString("SkyrimDataDirectory"));
    QCOMPARE(FilePaths::dataDirKey(Game_SkyrimSpecialEdition), QString("SkyrimSEDataDirectory"));
    QCOMPARE(FilePaths::dataDirKey(Game_SkyrimAnniversaryEdition), QString("SkyrimSEDataDirectory"));
    QCOMPARE(FilePaths::dataDirKey(Game_Fallout4), QString("Fallout4DataDirectory"));
    QCOMPARE(FilePaths::dataDirKey(Game_Starfield), QString("StarfieldDataDirectory"));
    QCOMPARE(FilePaths::dataDirKey(Game_None), QString("DataDirectory"));
}

// The list drives a picker the user reads top to bottom, so installs of the
// same title must not be interleaved with unrelated ones.
void TestConfigPaths::testDetectGamesAreGroupedByGame()
{
    const auto found = FilePaths::detectGames();

    QString previousGame;
    QString previousPath;
    for (const auto& entry : found)
    {
        const QString game = FilePaths::gameName(entry.gameId);
        QVERIFY(!game.isEmpty() && game != QLatin1String("Unknown"));

        const int byGame = QString::compare(game, previousGame);
        if (byGame < 0)
            QFAIL(qPrintable(QStringLiteral("unexpected game order: %1 after %2")
                    .arg(game, previousGame)));
        if (byGame > 0)
        {
            previousGame = game;
            previousPath.clear();
        }
        else if (QString::compare(entry.dataPath, previousPath, Qt::CaseInsensitive) < 0)
        {
            QFAIL(qPrintable(QStringLiteral("unexpected path order: %1 after %2 in %3")
                    .arg(entry.dataPath, previousPath, game)));
        }
        previousPath = entry.dataPath;
    }
}

#include "test_configpaths.moc"
QTEST_MAIN(TestConfigPaths)

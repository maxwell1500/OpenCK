#include <QTest>

#include "gameformat.hpp"

using namespace GameFormat;

class TestGameFormat : public QObject
{
    Q_OBJECT

private slots:
    void testDetectByMaster();
    void testDetectLightMaster();
    void testUnknown();
    void testGameNames();
    void testStarfieldRecords();
    void testOblivionRecords();
    void testSkyrimRecords();
    void testFallout4Records();
    void testMorrowindRecords();
    void testSupportsRecord();
};

void TestGameFormat::testDetectByMaster()
{
    QCOMPARE(detectGame({"Starfield.esm"}, quint16(0)), Game::Starfield);
    QCOMPARE(detectGame({"Skyrim.esm"}, quint16(0)), Game::Skyrim);
    QCOMPARE(detectGame({"Oblivion.esm"}, quint16(0)), Game::Oblivion);
    QCOMPARE(detectGame({"Morrowind.esm"}, quint16(0)), Game::Morrowind);
    QCOMPARE(detectGame({"FO4.se"}, quint16(0)), Game::Fallout4);
    // case-insensitive, extension stripped
    QCOMPARE(detectGame({"skyrim.ESM"}, quint16(0)), Game::Skyrim);
}

void TestGameFormat::testDetectLightMaster()
{
    // LightMaster flag with no recognizable master -> Starfield.
    QCOMPARE(detectGame({"SomeMod.esp"}, quint16(0x200)), Game::Starfield);
    // Without LightMaster and an unknown master -> Unknown.
    QCOMPARE(detectGame({"SomeMod.esp"}, quint16(0x01)), Game::Unknown);
}

void TestGameFormat::testUnknown()
{
    QCOMPARE(detectGame({"foo.esp"}, quint16(0)), Game::Unknown);
}

void TestGameFormat::testGameNames()
{
    QCOMPARE(gameName(Game::Starfield), QString("Starfield"));
    QCOMPARE(gameName(Game::Morrowind), QString("Morrowind"));
    QCOMPARE(gameName(Game::Unknown).isEmpty(), false);
}

void TestGameFormat::testStarfieldRecords()
{
    const QVector<NAME> recs = gameSpecificRecords(Game::Starfield);
    QVERIFY(recs.contains(NAME('SHOU')));
    QVERIFY(recs.contains(NAME('HDPT')));
    QVERIFY(recs.contains(NAME('XEZN')));
    QVERIFY(recs.contains(NAME('XWEM')));
    QVERIFY(gameSpecificRecords(Game::Unknown).isEmpty());
    QVERIFY(!recs.contains(NAME('MATT')));
}

void TestGameFormat::testOblivionRecords()
{
    const QVector<NAME> recs = gameSpecificRecords(Game::Oblivion);
    QVERIFY(recs.contains(NAME('PGRD')));
    QVERIFY(recs.contains(NAME('SPGD')));
    QVERIFY(recs.contains(NAME('LSPM')));
}

void TestGameFormat::testSkyrimRecords()
{
    const QVector<NAME> recs = gameSpecificRecords(Game::Skyrim);
    QVERIFY(recs.contains(NAME('MATT')));
    QVERIFY(recs.contains(NAME('CLMT')));
    QVERIFY(recs.contains(NAME('LAIF')));
    QVERIFY(recs.contains(NAME('SNIP')));
}

void TestGameFormat::testFallout4Records()
{
    const QVector<NAME> recs = gameSpecificRecords(Game::Fallout4);
    QVERIFY(recs.contains(NAME('ASRC')));
    QVERIFY(recs.contains(NAME('LTEX')));
}

void TestGameFormat::testMorrowindRecords()
{
    const QVector<NAME> recs = gameSpecificRecords(Game::Morrowind);
    QVERIFY(recs.contains(NAME('CLOT')));
    QVERIFY(recs.contains(NAME('CREA')));
    QVERIFY(recs.contains(NAME('LEVC')));
    QVERIFY(recs.contains(NAME('REGN')));
    QVERIFY(recs.contains(NAME('PGRD')));
    QVERIFY(recs.contains(NAME('SNDG')));
    QVERIFY(!recs.contains(NAME('LEVL')));
}

void TestGameFormat::testSupportsRecord()
{
    QVERIFY(supportsRecord(Game::Starfield, NAME('SHOU')));
    QVERIFY(!supportsRecord(Game::Skyrim, NAME('SHOU')));
    QVERIFY(supportsRecord(Game::Oblivion, NAME('PGRD')));
    QVERIFY(!supportsRecord(Game::Starfield, NAME('PGRD')));
    QVERIFY(supportsRecord(Game::Morrowind, NAME('CLOT')));
    QVERIFY(!supportsRecord(Game::Skyrim, NAME('CLOT')));
}

QTEST_MAIN(TestGameFormat)
#include "test_gameformat.moc"

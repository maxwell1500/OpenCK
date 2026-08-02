#include <QTest>
#include <QJsonObject>
#include <QJsonArray>

#include "../../src/model/tools/btdfile.hpp"

class TestBtdFile : public QObject
{
    Q_OBJECT

private slots:
    void testBuild();
    void testBuildInvalid();
    void testRoundTrip();
    void testTextureForQuad();
};

void TestBtdFile::testBuild()
{
    const QStringList textures = { QStringLiteral("Grass.dds"), QStringLiteral("Dirt.dds") };
    QVector<quint16> quads(4 * 4, 0);
    quads[0] = 0;
    quads[1] = 1;
    quads[4] = 1;

    BtdFile file;
    QVERIFY(BtdFile::build(4, textures, quads, file));
    QCOMPARE(file.gridSize, 4);
    QCOMPARE(file.textureNames, textures);
    QCOMPARE(file.textureCount, 2);
    QCOMPARE(file.quadIndices.size(), 16);
}

void TestBtdFile::testBuildInvalid()
{
    BtdFile file;
    // Size mismatch: grid 4x4 needs 16 quads.
    QVERIFY(!BtdFile::build(4, { QStringLiteral("A") }, QVector<quint16>(4), file));
    // Empty textures.
    QVERIFY(!BtdFile::build(2, {}, QVector<quint16>(4), file));
    // Non-positive grid.
    QVERIFY(!BtdFile::build(0, { QStringLiteral("A") }, {}, file));
}

void TestBtdFile::testRoundTrip()
{
    const QStringList textures = { QStringLiteral("Rock.dds"), QStringLiteral("Snow.dds") };
    QVector<quint16> quads(4, 0);
    quads[1] = 1;

    BtdFile file;
    QVERIFY(BtdFile::build(2, textures, quads, file));
    file.fileName = QStringLiteral("cell_4_4.btd");

    const QJsonObject json = file.toJson();
    const BtdFile loaded = BtdFile::fromJson(json);

    QCOMPARE(loaded.fileName, QStringLiteral("cell_4_4.btd"));
    QCOMPARE(loaded.gridSize, 2);
    QCOMPARE(loaded.textureCount, 2);
    QCOMPARE(loaded.textureNames, textures);
    QCOMPARE(loaded.quadIndices, quads);
}

void TestBtdFile::testTextureForQuad()
{
    const QStringList textures = { QStringLiteral("A.dds"), QStringLiteral("B.dds") };
    QVector<quint16> quads(9, 0);
    quads[1] = 1;

    BtdFile file;
    QVERIFY(BtdFile::build(3, textures, quads, file));
    QCOMPARE(file.textureForQuad(0, 0), QStringLiteral("A.dds"));
    QCOMPARE(file.textureForQuad(1, 0), QStringLiteral("B.dds"));
    QVERIFY(file.textureForQuad(5, 5).isEmpty());  // out of range
}

QTEST_MAIN(TestBtdFile)
#include "test_btdfile.moc"

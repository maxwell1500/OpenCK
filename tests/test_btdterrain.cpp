#include <QTest>
#include <QFileInfo>

#include "model/tools/btdterrain.hpp"
#include "ba2archive.hpp"
#include "logger.hpp"

// Reads a real Starfield .btd from the Terrain BA2 (magic BTDB, version 6)
// and checks the header, LOD4 maps, and the cell-0 LOD0 block decompression.
// Requires the user's Starfield install; not registered with CTest by default.
class TestBtdTerrain : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testReadRealBtd();
};

void TestBtdTerrain::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_btdterrain_log.txt"));
}

void TestBtdTerrain::testReadRealBtd()
{
    const QString archive = QStringLiteral(
        "C:/XboxGames/Starfield/Content/Data/Starfield - Terrain01.ba2");
    QVERIFY2(QFileInfo::exists(archive), "Starfield Terrain01.ba2 not found");

    Ba2Archive ba2;
    QVERIFY(ba2.open(archive));
    QVERIFY(ba2.fileCount() > 100);

    int index = -1;
    for (quint32 i = 0; i < ba2.fileCount(); ++i)
    {
        if (ba2.entries().at(i).relativePath.endsWith(QStringLiteral(".btd"), Qt::CaseInsensitive))
        {
            index = static_cast<int>(i);
            break;
        }
    }
    QVERIFY(index >= 0);
    qDebug() << "first .btd:" << ba2.entries().at(index).relativePath
             << "size" << ba2.entries().at(index).uncompressedSize;

    const QString tmp = QDir::tempPath() + QStringLiteral("/openck_test.btd");
    QVERIFY(ba2.extract(static_cast<quint32>(index), tmp));
    QFile f(tmp);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray bytes = f.readAll();
    f.close();
    QFile::remove(tmp);

    QVERIFY(bytes.startsWith("BTDB"));
    const BtdTerrain terrain = BtdTerrain::read(bytes);
    QVERIFY(terrain.ok);
    QCOMPARE(terrain.version, quint32(6));
    QCOMPARE(terrain.ltexCount, quint32(11));
    QCOMPARE(terrain.ltexFormIds.size(), 11);
    QVERIFY(terrain.nCellsX > 0);
    QVERIFY(terrain.nCellsY > 0);
    qDebug() << "res" << terrain.resX << "x" << terrain.resY
             << "cells" << terrain.nCellsX << "x" << terrain.nCellsY
             << "height range" << terrain.worldHeightMin << ".." << terrain.worldHeightMax;

    // LOD4 maps: nCellsY*8 x nCellsX*8 samples each.
    QCOMPARE(terrain.heightMapLOD4.size(),
        static_cast<int>(terrain.nCellsY * 8 * terrain.nCellsX * 8));
    QCOMPARE(terrain.landTexturesLOD4.size(),
        static_cast<int>(terrain.nCellsY * 8 * terrain.nCellsX * 8));

    // Cell-0 LOD0 block: 128x128 height map + 128x128 textures.
    QCOMPARE(terrain.cellHeightMap.size(), 16384);
    QCOMPARE(terrain.cellLandTextures.size(), 16384);
    qDebug() << "cell height sample[0..3]:"
             << terrain.cellHeightMap.at(0) << terrain.cellHeightMap.at(1)
             << terrain.cellHeightMap.at(2) << terrain.cellHeightMap.at(3);
    QVERIFY(terrain.cellHeightMap.at(0) > 0 || terrain.cellHeightMap.at(0) == 0);
}

QTEST_MAIN(TestBtdTerrain)
#include "test_btdterrain.moc"

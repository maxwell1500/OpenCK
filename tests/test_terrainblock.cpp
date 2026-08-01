#include <QTest>
#include <QRect>

#include "../../src/model/tools/terrainblock.hpp"

class TestTerrainBlock : public QObject
{
    Q_OBJECT

private slots:
    void testCut();
    void testCutClamped();
    void testInsertOverwrite();
    void testInsertBlend();
    void testAlignToGrid();
};

void TestTerrainBlock::testCut()
{
    // 4x4 heightmap with distinct values.
    QVector<float> map;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            map.append(static_cast<float>(y * 10 + x));

    TerrainBlock::Block block;
    QVERIFY(TerrainBlock::cut(map, 4, QRect(1, 1, 2, 2), block));
    QCOMPARE(block.width(), 2);
    QCOMPARE(block.height(), 2);
    QCOMPARE(block.rect, QRect(1, 1, 2, 2));
    QCOMPARE(block.heights.size(), 4);
    QCOMPARE(block.heights[0], 11.0f);  // (1,1)
    QCOMPARE(block.heights[1], 12.0f);  // (2,1)
    QCOMPARE(block.heights[2], 21.0f);  // (1,2)
    QCOMPARE(block.heights[3], 22.0f);  // (2,2)
}

void TestTerrainBlock::testCutClamped()
{
    QVector<float> map(16, 5.0f);
    TerrainBlock::Block block;
    // Off-grid rectangle clamps to the valid area.
    QVERIFY(TerrainBlock::cut(map, 4, QRect(3, 3, 5, 5), block));
    QCOMPARE(block.rect, QRect(3, 3, 1, 1));
    QCOMPARE(block.heights.size(), 1);

    // Fully outside -> empty.
    TerrainBlock::Block none;
    QVERIFY(!TerrainBlock::cut(map, 4, QRect(10, 10, 2, 2), none));
}

void TestTerrainBlock::testInsertOverwrite()
{
    QVector<float> map(16, 0.0f);
    TerrainBlock::Block block;
    QVERIFY(TerrainBlock::cut(map, 4, QRect(0, 0, 2, 2), block));
    block.heights.fill(9.0f);

    TerrainBlock::insert(map, 4, block, QPoint(2, 2), 0);
    QCOMPARE(map[2 * 4 + 2], 9.0f);
    QCOMPARE(map[3 * 4 + 3], 9.0f);
    QCOMPARE(map[0 * 4 + 0], 0.0f);  // outside the block stays
}

void TestTerrainBlock::testInsertBlend()
{
    QVector<float> map(16, 0.0f);
    TerrainBlock::Block block;
    QVERIFY(TerrainBlock::cut(map, 4, QRect(0, 0, 4, 4), block));
    block.heights.fill(100.0f);

    // Paste the full map with a blend ring: corners blend fully toward 0,
    // the interior stays near 100.
    TerrainBlock::insert(map, 4, block, QPoint(0, 0), 2);

    // Interior cell (2,2) should be near 100.
    QVERIFY(map[2 * 4 + 2] > 90.0f);
    // Corner (0,0) is the outer edge -> heavily blended toward 0.
    QVERIFY(map[0 * 4 + 0] < 60.0f);
    QVERIFY(map[0 * 4 + 0] > 0.0f);
}

void TestTerrainBlock::testAlignToGrid()
{
    const QRect aligned = TerrainBlock::alignToGrid(QRect(3, 7, 10, 5), 8);
    QCOMPARE(aligned.left(), 0);
    QCOMPARE(aligned.top(), 0);
    QCOMPARE(aligned.width(), 16);
    QCOMPARE(aligned.height(), 8);

    // Grid of 1 is the identity.
    QCOMPARE(TerrainBlock::alignToGrid(QRect(3, 7, 10, 5), 1), QRect(3, 7, 10, 5));
}

QTEST_MAIN(TestTerrainBlock)
#include "test_terrainblock.moc"

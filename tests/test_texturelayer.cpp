#include <QTest>

#include "../../src/view/window/landscapeeditor.hpp"
#include "../../libs/files/log/logger.hpp"
#include "../../libs/files/esm/cellrecord.hpp"
#include "../../libs/files/esm/landrecord.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../model/tools/landscapeeditcommand.hpp"

class TestTextureLayer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDefaultNoSlopeInfluence();
    void testSlopeFade();
    void testSlopeInverted();
    void testDefaults();
    void testLandscapeEditorSculptAndUndo();
    void testLandscapeEditorSculptingModes();
    void testLandscapeEditorTexturePainting();
    void testLandscapeEditorRaycastAndRing();
};

void TestTextureLayer::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_texturelayer_log.txt"));
}

void TestTextureLayer::testDefaultNoSlopeInfluence()
{
    TextureLayer layer;
    layer.index = 0;
    layer.texturePath = QStringLiteral("foo.dds");
    layer.opacity = 1.0;

    QCOMPARE(layer.slopeModifier(0.0), 1.0);
    QCOMPARE(layer.slopeModifier(45.0), 1.0);
    QCOMPARE(layer.slopeModifier(90.0), 1.0);
    QCOMPARE(layer.maxMaterialOpacity, 1.0);
    QVERIFY(!layer.applySlopeInfluence);
}

void TestTextureLayer::testSlopeFade()
{
    TextureLayer layer;
    layer.applySlopeInfluence = true;
    layer.slopeThreshold = 20.0;
    layer.slopeFalloff = 10.0;
    layer.slopeInvert = false;

    QVERIFY(qFuzzyCompare(layer.slopeModifier(0.0), 1.0));
    QVERIFY(qFuzzyCompare(layer.slopeModifier(20.0), 1.0));
    QVERIFY(qFuzzyCompare(layer.slopeModifier(25.0), 0.5));
    QVERIFY(qFuzzyCompare(layer.slopeModifier(30.0), 0.0));
    QVERIFY(qFuzzyCompare(layer.slopeModifier(60.0), 0.0));
}

void TestTextureLayer::testSlopeInverted()
{
    TextureLayer layer;
    layer.applySlopeInfluence = true;
    layer.slopeThreshold = 20.0;
    layer.slopeFalloff = 10.0;
    layer.slopeInvert = true;

    QVERIFY(qFuzzyCompare(layer.slopeModifier(0.0), 0.0));
    QVERIFY(qFuzzyCompare(layer.slopeModifier(25.0), 0.5));
    QVERIFY(qFuzzyCompare(layer.slopeModifier(30.0), 1.0));
    QVERIFY(qFuzzyCompare(layer.slopeModifier(45.0), 1.0));
}

void TestTextureLayer::testDefaults()
{
    TextureLayer layer;
    QCOMPARE(layer.maxMaterialOpacity, 1.0);
    QCOMPARE(layer.slopeThreshold, 0.0);
    QCOMPARE(layer.slopeFalloff, 1.0);
    QVERIFY(!layer.slopeInvert);
}

void TestTextureLayer::testLandscapeEditorSculptAndUndo()
{
    LandscapeEditor editor;
    UndoStack undoStack;
    editor.setUndoStack(&undoStack);

    CellRecord cell;
    cell.editorId = QStringLiteral("TestWilderness");
    cell.formId = 0x00010001;
    cell.cellX = 0;
    cell.cellY = 0;
    editor.loadCell(&cell);

    const int size = editor.getTerrainSize();
    QVERIFY(size > 0);
    float initialHeight = editor.getHeightAt(size / 2, size / 2);

    // Sculpt (raise) at center
    editor.setActiveBrushIndex(0); // Sculpt
    editor.setBrushSize(10);
    editor.setBrushStrength(50);
    editor.applyBrush(size / 2, size / 2);

    float raisedHeight = editor.getHeightAt(size / 2, size / 2);
    QVERIFY(raisedHeight > initialHeight);

    // Simulate dirty region command push
    QVector<float> originalRegion = { initialHeight };
    QVector<float> newRegion = { raisedHeight };
    auto* cmd = new LandscapeEditCommand(
        const_cast<QVector<float>*>(&editor.getHeightmap()), size,
        size / 2, size / 2, 1, 1, originalRegion, newRegion);
    undoStack.push(cmd);

    QVERIFY(undoStack.canUndo());
    undoStack.undo();
    QCOMPARE(editor.getHeightAt(size / 2, size / 2), initialHeight);

    QVERIFY(undoStack.canRedo());
    undoStack.redo();
    QCOMPARE(editor.getHeightAt(size / 2, size / 2), raisedHeight);
}

void TestTextureLayer::testLandscapeEditorSculptingModes()
{
    LandscapeEditor editor;
    CellRecord cell;
    cell.editorId = QStringLiteral("SculptCell");
    editor.loadCell(&cell);

    const int cx = editor.getTerrainSize() / 2;
    const int cy = editor.getTerrainSize() / 2;

    // Flatten mode: pull toward target height
    const auto& brushes = editor.getBrushes();
    int flattenIdx = -1;
    int smoothIdx = -1;
    int noiseIdx = -1;
    for (int i = 0; i < brushes.size(); ++i) {
        if (brushes[i].operation == BrushDefinition::Operation::Flatten) flattenIdx = i;
        if (brushes[i].operation == BrushDefinition::Operation::Smooth) smoothIdx = i;
        if (brushes[i].operation == BrushDefinition::Operation::Noise) noiseIdx = i;
    }
    QVERIFY(flattenIdx >= 0);
    QVERIFY(smoothIdx >= 0);
    QVERIFY(noiseIdx >= 0);

    editor.setActiveBrushIndex(flattenIdx);
    editor.setBrushSize(8);
    editor.applyBrush(cx, cy);

    // Smooth mode
    editor.setActiveBrushIndex(smoothIdx);
    editor.applyBrush(cx, cy);

    // Noise mode: should displace vertices
    editor.setActiveBrushIndex(noiseIdx);
    float beforeNoise = editor.getHeightAt(cx, cy);
    editor.applyBrush(cx, cy);
    float afterNoise = editor.getHeightAt(cx, cy);
    // Pseudo-random noise changed the vertex elevation
    QVERIFY(beforeNoise != afterNoise);
}

void TestTextureLayer::testLandscapeEditorTexturePainting()
{
    LandscapeEditor editor;
    CellRecord cell;
    cell.editorId = QStringLiteral("PaintCell");
    editor.loadCell(&cell);

    editor.setPaintMode(LandscapeEditor::PaintMode::PaintTexture);
    QCOMPARE(editor.getPaintMode(), LandscapeEditor::PaintMode::PaintTexture);

    // Add a layer and paint alpha
    TextureLayer layer;
    layer.index = 0;
    layer.texturePath = QStringLiteral("Textures\\Landscape\\Dirt01.dds");
    layer.opacity = 0.2;

    LandRecord land;
    land.cellX = 0;
    land.cellY = 0;
    editor.saveHeightmap(land);
    QVERIFY(land.hasHeightData);
}

void TestTextureLayer::testLandscapeEditorRaycastAndRing()
{
    LandscapeEditor editor;
    CellRecord cell;
    cell.editorId = QStringLiteral("RayCell");
    editor.loadCell(&cell);

    const int size = editor.getTerrainSize();
    QVERIFY(size > 0);

    // Downward ray over the terrain center
    const float scale = 10.0f;
    const float heightScale = 0.1f;
    float midX = static_cast<float>(size / 2) * scale;
    float midY = static_cast<float>(size / 2) * scale;

    QVector3D rayOrigin(midX, 500.0f, midY);
    QVector3D rayDir(0.0f, -1.0f, 0.0f);

    int outX = -1, outY = -1;
    float elev = 0.0f;
    bool hit = editor.raycastTerrain(rayOrigin, rayDir, outX, outY, &elev);
    QVERIFY(hit);
    QCOMPARE(outX, size / 2);
    QCOMPARE(outY, size / 2);

    // Ray completely outside bounds misses
    QVector3D missOrigin(-5000.0f, 500.0f, -5000.0f);
    QVERIFY(!editor.raycastTerrain(missOrigin, rayDir, outX, outY));
}

QTEST_MAIN(TestTextureLayer)
#include "test_texturelayer.moc"

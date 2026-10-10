#include <QTest>
#include <QSignalSpy>

#include "../../libs/files/filepaths.hpp"
#include "../../src/model/world/data.hpp"
#include "../../src/view/window/cellsdialog.hpp"
#include "../../libs/files/esm/cellrecord.hpp"
#include "../../libs/files/esm/refrecord.hpp"
#include "../../libs/files/esm/statrecord.hpp"
#include "../../libs/files/esm/worldspacerecord.hpp"
#include "../../libs/files/log/logger.hpp"

class TestCellViewPanel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCellTableModelInteriorsAndExteriors();
    void testRefrTableModelColumnsAndResolution();
    void testCellViewPanelSelectionAndDuplication();
};

void TestCellViewPanel::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_cellviewpanel_log.txt"));
}

void TestCellViewPanel::testCellTableModelInteriorsAndExteriors()
{
    const FilePaths paths;
    Data data(QStringList(), paths);

    // Add an interior cell (flags & 1)
    CellRecord interior;
    interior.editorId = QStringLiteral("TestInteriorCell");
    interior.formId = 0x00010001;
    interior.flags = 1;
    interior.cellName = QStringLiteral("Tavern Interior");
    data.getCellCollection().add(interior);

    // Add an exterior cell (flags & 1 == 0)
    CellRecord exterior;
    exterior.editorId = QStringLiteral("TestExteriorCell");
    exterior.formId = 0x00010002;
    exterior.flags = 0;
    exterior.cellX = 2;
    exterior.cellY = -3;
    exterior.cellName = QStringLiteral("Wilderness Plains");
    exterior.hasWaterHeight = true;
    exterior.waterHeight = 128.5f;
    data.getCellCollection().add(exterior);

    CellViewPanel panel(&data);
    CellTableModel* model = panel.cellModel();
    QVERIFY(model != nullptr);

    // Default mode is Interiors
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0, 0)).toString(), QStringLiteral("TestInteriorCell"));
    QCOMPARE(model->data(model->index(0, 2)).toString(), QStringLiteral("Interior"));
    QCOMPARE(model->data(model->index(0, 3)).toString(), QStringLiteral("Tavern Interior"));

    // Switch to All Cells
    model->setWorldspace(nullptr, false);
    QCOMPARE(model->rowCount(), 2);

    // Test columns of exterior row
    int extRow = (model->data(model->index(0, 0)).toString() == QStringLiteral("TestExteriorCell")) ? 0 : 1;
    QCOMPARE(model->data(model->index(extRow, 0)).toString(), QStringLiteral("TestExteriorCell"));
    QCOMPARE(model->data(model->index(extRow, 1)).toString(), QStringLiteral("0x00010002"));
    QCOMPARE(model->data(model->index(extRow, 2)).toString(), QStringLiteral("2, -3"));
    QCOMPARE(model->data(model->index(extRow, 3)).toString(), QStringLiteral("Wilderness Plains"));
    QVERIFY(model->data(model->index(extRow, 5)).toString().startsWith(QStringLiteral("Yes")));

    // Test filtering
    model->setFilter(QStringLiteral("Tavern"));
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0, 0)).toString(), QStringLiteral("TestInteriorCell"));

    model->setFilter(QString());
    QCOMPARE(model->rowCount(), 2);
}

void TestCellViewPanel::testRefrTableModelColumnsAndResolution()
{
    const FilePaths paths;
    Data data(QStringList(), paths);

    // Base Static object
    StatRecord baseStat;
    baseStat.editorId = QStringLiteral("DungeonPillar");
    baseStat.formId = 0x00020005;
    data.getStatCollection().add(baseStat);

    // Cell
    CellRecord cell;
    cell.editorId = QStringLiteral("DungeonRoom01");
    cell.formId = 0x00010010;
    cell.flags = 1; // interior
    data.getCellCollection().add(cell);

    // Placed Reference
    RefrRecord ref;
    ref.editorId = QStringLiteral("PillarRef01");
    ref.formId = 0x00030020;
    ref.baseId = 0x00020005;
    ref.posX = 120.0f;
    ref.posY = 240.0f;
    ref.posZ = 360.0f;
    ref.rotX = 0.0f;
    ref.rotY = 45.0f;
    ref.rotZ = 90.0f;
    ref.scale = 1.25f;
    data.getRefrCollection().add(ref);
    data.setRefrParentCell(ref.formId, cell.formId);

    CellViewPanel panel(&data);
    RefrTableModel* refrModel = panel.refrModel();
    QVERIFY(refrModel != nullptr);

    QCOMPARE(refrModel->columnCount(), 9);

    // Set cell
    refrModel->setCell(&cell);
    QCOMPARE(refrModel->rowCount(), 1);

    // Verify column data
    QCOMPARE(refrModel->data(refrModel->index(0, 0)).toString(), QStringLiteral("0x00030020"));
    QCOMPARE(refrModel->data(refrModel->index(0, 1)).toString(), QStringLiteral("PillarRef01"));
    QVERIFY(refrModel->data(refrModel->index(0, 2)).toString().contains(QStringLiteral("DungeonPillar")));
    QCOMPARE(refrModel->data(refrModel->index(0, 3)).toString(), QStringLiteral("Static"));
    QCOMPARE(refrModel->data(refrModel->index(0, 6)).toString(), QStringLiteral("1.25"));

    // Test filter
    refrModel->setFilter(QStringLiteral("Pillar"));
    QCOMPARE(refrModel->rowCount(), 1);

    refrModel->setFilter(QStringLiteral("NonExistent"));
    QCOMPARE(refrModel->rowCount(), 0);

    refrModel->setFilter(QString());
    QCOMPARE(refrModel->rowCount(), 1);
}

void TestCellViewPanel::testCellViewPanelSelectionAndDuplication()
{
    const FilePaths paths;
    Data data(QStringList(), paths);

    CellRecord cell;
    cell.editorId = QStringLiteral("CryptCell");
    cell.formId = 0x00010050;
    cell.flags = 1;
    data.getCellCollection().add(cell);

    RefrRecord ref;
    ref.editorId = QStringLiteral("CryptTorchRef");
    ref.formId = 0x00030050;
    ref.baseId = 0x00020005;
    data.getRefrCollection().add(ref);
    data.setRefrParentCell(ref.formId, cell.formId);

    CellViewPanel panel(&data);
    panel.cellModel()->setWorldspace(nullptr, false);

    QSignalSpy cellSpy(&panel, &CellViewPanel::cellSelected);
    panel.selectCellByFormId(cell.formId);
    QCOMPARE(cellSpy.count(), 1);
    QCOMPARE(cellSpy.first().at(0).toUInt(), cell.formId);
    QVERIFY(panel.currentCell() != nullptr);
    QCOMPARE(panel.currentCell()->editorId, QStringLiteral("CryptCell"));

    // Select reference
    QSignalSpy refSpy(&panel, &CellViewPanel::refSelected);
    panel.selectRefByFormId(ref.formId);
    QCOMPARE(refSpy.count(), 1);
    QVERIFY(panel.currentRef() != nullptr);
    QCOMPARE(panel.currentRef()->editorId, QStringLiteral("CryptTorchRef"));
}

QTEST_MAIN(TestCellViewPanel)
#include "test_cellviewpanel.moc"

#include <QTest>
#include <QAction>
#include <QDir>
#include <QOpenGLWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>

#include "../../libs/files/esm/refrecord.hpp"
#include "../../libs/files/filepaths.hpp"
#include "../../libs/files/log/logger.hpp"
#include "../../src/model/tools/undostack.hpp"
#include "../../src/model/world/data.hpp"
#include "../../src/view/window/mainwindow.hpp"
#include "../../src/view/window/nifviewportwidget.hpp"
#include "../../src/view/window/windowlayout.hpp"

namespace {

// The top-level menus of the real Creation Kit, in menu-bar order.
const QStringList kCanonicalMenus = {
    QStringLiteral("File"),
    QStringLiteral("Edit"),
    QStringLiteral("View"),
    QStringLiteral("Character"),
    QStringLiteral("Gameplay"),
    QStringLiteral("World"),
    QStringLiteral("ObjectWindows"),
    QStringLiteral("RenderWindows"),
    QStringLiteral("Navmesh"),
    QStringLiteral("Terrain"),
    QStringLiteral("Audio"),
    QStringLiteral("Galaxy"),
    QStringLiteral("Docks"),
    QStringLiteral("Theme"),
    QStringLiteral("Help"),
};

QStringList menuBarTitles(const QMainWindow& window)
{
    QStringList titles;
    for (QAction* action : window.menuBar()->actions())
    {
        if (QMenu* menu = action->menu())
            titles << menu->title();
    }
    return titles;
}

/// A MainWindow wired to an empty Data model, matching the state a user reaches
/// after File > Data with no plugin selected.
struct Fixture
{
    FilePaths paths;
    Data data;
    MainWindow window;

    Fixture() : data(QStringList(), paths) { window.setData(&data); }
};

} // namespace

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCanonicalMenuBar();
    void testRenderWindowIsPinnedCentralDock();
    void testDefaultLayoutPlacesDocks();
    void testViewMenuTogglesDriveViewport();
    void testCameraSpeedScalesMovement();
    void testCoordinateReadoutTracksCameraAndSelection();
    void testDocksMenuListsEveryDock();
    void testLockDocksFreezesDockFeatures();
    void testWindowRendersLayout();
    void testViewportMultiSelectionAndMarquee();
    void testViewportDropToGroundAndDuplicate();
};

void TestMainWindow::testViewportMultiSelectionAndMarquee()
{
    FilePaths paths;
    Data data(QStringList(), paths);
    auto addRef = [&data](const char* editorId, float x) {
        RefrRecord ref;
        ref.editorId = QString::fromLatin1(editorId);
        ref.formId = 0x00040000 + static_cast<quint32>(x);
        ref.posX = x;
        data.getRefrCollection().add(ref);
    };
    addRef("RefA", 0.0f);
    addRef("RefB", 10000.0f);
    addRef("RefC", 20000.0f);

    MainWindow window;
    window.setData(&data);
    auto* viewport = window.findChild<NifViewportWidget*>();
    QVERIFY(viewport);
    QCOMPARE(viewport->selectedRefIndex(), -1);

    // Shift-style membership: programmatic equivalent of Shift+Click.
    viewport->setSelectedRefIndices({ 0, 2 });
    QCOMPARE(viewport->selectedRefIndices(), QVector<int>({ 0, 2 }));
    QCOMPARE(viewport->selectedRefIndex(), 2);
    QVERIFY(viewport->isRefSelected(0));
    QVERIFY(!viewport->isRefSelected(1));

    // Compatibility: single selection clears the set.
    viewport->setSelectedRefIndex(1);
    QCOMPARE(viewport->selectedRefIndices(), QVector<int>({ 1 }));
    viewport->setSelectedRefIndex(-1);
    QVERIFY(viewport->selectedRefIndices().isEmpty());

    // Marquee: a rectangle around all three marker projections.
    viewport->setSelectedRefIndices({ 0, 1, 2 });
    viewport->setSelectedRefIndex(-1);
    viewport->selectMarqueeRect(QRect(-10000, -10000, 20000, 20000));
    QVERIFY(!viewport->selectedRefIndices().isEmpty());

    // A degenerate rectangle never selects.
    viewport->setSelectedRefIndex(-1);
    viewport->selectMarqueeRect(QRect(0, 0, 1, 1));
    QVERIFY(viewport->selectedRefIndices().isEmpty());
    QVERIFY(viewport->selectedRefIndex() == -1);
}

void TestMainWindow::testViewportDropToGroundAndDuplicate()
{
    FilePaths paths;
    Data data(QStringList(), paths);

    // Add terrain cell and land record with a known base height
    CellRecord cell;
    cell.editorId = QStringLiteral("WildernessCell");
    cell.formId = 0x00010001;
    cell.cellX = 0;
    cell.cellY = 0;
    data.getCellCollection().add(cell);

    LandRecord land;
    land.cellX = 0;
    land.cellY = 0;
    land.baseHeight = 128.0f;
    data.getLandCollection().add(land);

    // Reference floating in the air
    RefrRecord ref;
    ref.editorId = QStringLiteral("FloatingChest");
    ref.formId = 0x00020001;
    ref.posX = 100.0f;
    ref.posY = 100.0f;
    ref.posZ = 500.0f;
    data.getRefrCollection().add(ref);
    data.setRefrParentCell(ref.formId, cell.formId);

    MainWindow window;
    window.setData(&data);
    auto* viewport = window.findChild<NifViewportWidget*>();
    QVERIFY(viewport);

    // 1. Drop to Ground test
    viewport->setSelectedRefIndex(0);
    QCOMPARE(viewport->selectedRefIndex(), 0);
    window.dropViewportSelectionToGround();

    const auto& coll = data.getRefrCollection();
    QCOMPARE(coll.getRecord(0).get().posZ, 128.0f);

    // Undo drop
    QVERIFY(data.getUndoStack()->canUndo());
    data.getUndoStack()->undo();
    QCOMPARE(coll.getRecord(0).get().posZ, 500.0f);

    // Redo drop
    data.getUndoStack()->redo();
    QCOMPARE(coll.getRecord(0).get().posZ, 128.0f);

    // 2. Duplicate in Viewport test
    const int countBefore = coll.size();
    window.duplicateViewportSelection();
    QCOMPARE(coll.size(), countBefore + 1);

    const auto& copy = coll.getRecord(countBefore).get();
    QVERIFY(copy.formId != ref.formId);
    QCOMPARE(copy.posX, 164.0f); // 100 + 64 offset
    QCOMPARE(copy.posY, 164.0f);
    QCOMPARE(data.parentCellOfRefr(copy.formId), cell.formId);

    // Undo duplicate
    QVERIFY(data.getUndoStack()->canUndo());
    data.getUndoStack()->undo();
    QCOMPARE(coll.size(), countBefore);

    // Redo duplicate
    data.getUndoStack()->redo();
    QCOMPARE(coll.size(), countBefore + 1);
}

void TestMainWindow::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(
        QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_mainwindow_log.txt"));
}

void TestMainWindow::testCanonicalMenuBar()
{
    Fixture f;
    const QStringList titles = menuBarTitles(f.window);

    int previous = -1;
    for (const QString& name : kCanonicalMenus)
    {
        const int index = titles.indexOf(name);
        QVERIFY2(index >= 0, qPrintable(QStringLiteral("Missing top-level menu: %1").arg(name)));
        QVERIFY2(index > previous, qPrintable(QStringLiteral("Menu out of order: %1").arg(name)));
        previous = index;
    }

    // Internal validation sits between Theme and Help, as in the CK.
    QVERIFY(titles.indexOf(QStringLiteral("Tests")) > titles.indexOf(QStringLiteral("Theme")));
    QVERIFY(titles.indexOf(QStringLiteral("Tests")) < titles.indexOf(QStringLiteral("Help")));

    // OpenCK-only menus stay behind the CK-native set.
    QVERIFY(titles.indexOf(QStringLiteral("Plugins")) > titles.indexOf(QStringLiteral("Help")));
    QVERIFY(titles.indexOf(QStringLiteral("Tools")) > titles.indexOf(QStringLiteral("Help")));

    // Window panels are reached from their own menus, not from View.
    QMenu* view = f.window.findChild<QMenu*>(QStringLiteral("menuView"));
    QVERIFY(view);
    QStringList viewEntries;
    for (QAction* action : view->actions())
    {
        if (!action->isSeparator())
            viewEntries << action->text();
    }
    QVERIFY(!viewEntries.contains(QStringLiteral("Object Window...")));
}

void TestMainWindow::testRenderWindowIsPinnedCentralDock()
{
    Fixture f;
    auto* manager = f.window.findChild<ads::CDockManager*>();
    QVERIFY(manager);

    // The dock manager owns the QMainWindow's central area, so nothing else may
    // claim it (doing so used to schedule the manager itself for deletion).
    QCOMPARE(f.window.centralWidget(), static_cast<QWidget*>(manager));

    ads::CDockWidget* central = manager->centralWidget();
    QVERIFY(central);
    QVERIFY(qobject_cast<NifViewportWidget*>(central->widget()) != nullptr);

    QVERIFY(!central->features().testFlag(ads::CDockWidget::DockWidgetClosable));
    QVERIFY(!central->features().testFlag(ads::CDockWidget::DockWidgetMovable));
    QVERIFY(!central->features().testFlag(ads::CDockWidget::DockWidgetFloatable));
}

void TestMainWindow::testDefaultLayoutPlacesDocks()
{
    Fixture f;
    auto* manager = f.window.findChild<ads::CDockManager*>();
    QVERIFY(manager);

    WindowLayout::applyDefaultLayout(&f.window);
    f.window.resize(1400, 900);
    f.window.show();
    QTest::qWait(400);

    ads::CDockWidget* objectDock = manager->findDockWidget(QStringLiteral("Object Window"));
    QVERIFY(objectDock);
    QVERIFY(objectDock->isVisible());
    QVERIFY(objectDock->dockAreaWidget());

    ads::CDockWidget* cellDock = manager->findDockWidget(QStringLiteral("Cell View"));
    QVERIFY(cellDock);
    QVERIFY(cellDock->isVisible());

    // Object Window occupies the left column, the Render Window the centre.
    const int objectX = objectDock->dockAreaWidget()->mapToGlobal(QPoint(0, 0)).x();
    const int renderX = manager->centralWidget()->dockAreaWidget()->mapToGlobal(QPoint(0, 0)).x();
    QVERIFY2(objectX < renderX, qPrintable(QStringLiteral("Object Window x=%1, Render Window x=%2")
                                               .arg(objectX).arg(renderX)));

    // Tear down the way the application does. Closing releases the Render
    // Window's GL context; destroying a still-visible window does not.
    f.window.close();
    QTest::qWait(50);
    QFile::remove(QCoreApplication::applicationDirPath()
                  + QStringLiteral("/QtCreationKitSavedSettings.ini"));
}

void TestMainWindow::testViewMenuTogglesDriveViewport()
{
    Fixture f;
    auto* viewport = f.window.findChild<NifViewportWidget*>();
    QVERIFY(viewport);

    const auto action = [&f](const char* name) {
        return f.window.findChild<QAction*>(QString::fromLatin1(name));
    };

    QAction* grid = action("actionToggleGrid");
    QAction* wireframe = action("actionToggleWireframe");
    QAction* bounds = action("actionToggleBounds");
    QAction* sky = action("actionToggleSky");
    QAction* collision = action("actionToggleCollision");
    QVERIFY(grid && wireframe && bounds && sky && collision);

    QVERIFY(!viewport->isGridEnabled());
    QVERIFY(!viewport->isWireframeMode());
    QVERIFY(!viewport->isBoundsEnabled());
    QVERIFY(!viewport->isSkyEnabled());
    QVERIFY(!viewport->isCollisionEnabled());

    grid->setChecked(true);
    QVERIFY(viewport->isGridEnabled());
    wireframe->setChecked(true);
    QVERIFY(viewport->isWireframeMode());
    bounds->setChecked(true);
    QVERIFY(viewport->isBoundsEnabled());
    sky->setChecked(true);
    QVERIFY(viewport->isSkyEnabled());
    collision->setChecked(true);
    QVERIFY(viewport->isCollisionEnabled());

    // The Render Window's own toolbar drives the same state, and the menu
    // checkmarks follow it back.
    auto* gridBtn = viewport->findChild<QPushButton*>(QStringLiteral("gridBtn"));
    QVERIFY(gridBtn);
    QVERIFY(gridBtn->isChecked());
    gridBtn->click();
    QVERIFY(!viewport->isGridEnabled());
    QVERIFY(!grid->isChecked());
}

void TestMainWindow::testCameraSpeedScalesMovement()
{
    Fixture f;
    auto* viewport = f.window.findChild<NifViewportWidget*>();
    QVERIFY(viewport);

    QAction* normal = f.window.findChild<QAction*>("actionCameraSpeedNormal");
    QAction* fast = f.window.findChild<QAction*>("actionCameraSpeedFast");
    QVERIFY(normal && fast);
    QVERIFY(normal->isChecked());

    const auto forwardStep = [viewport]() {
        const QVector3D before = viewport->cameraPosition();
        QTest::keyClick(viewport, Qt::Key_W);
        return (viewport->cameraPosition() - before).length();
    };

    const float slowStep = forwardStep();
    QVERIFY(slowStep > 0.0f);

    fast->trigger();
    QVERIFY(fast->isChecked());
    QVERIFY(!normal->isChecked());
    QCOMPARE(viewport->cameraSpeedMultiplier(), 2.5f);

    const float fastStep = forwardStep();
    QVERIFY2(qAbs(fastStep / slowStep - 2.5f) < 0.01f,
             qPrintable(QStringLiteral("slow=%1 fast=%2").arg(slowStep).arg(fastStep)));
}

void TestMainWindow::testCoordinateReadoutTracksCameraAndSelection()
{
    FilePaths paths;
    Data data(QStringList(), paths);

    RefrRecord ref;
    ref.editorId = QStringLiteral("TestRef");
    ref.formId = 0x00040001;
    ref.posX = 100.0f;
    ref.posY = 200.0f;
    ref.posZ = 300.0f;
    data.getRefrCollection().add(ref);

    MainWindow window;
    window.setData(&data);

    auto* viewport = window.findChild<NifViewportWidget*>();
    QVERIFY(viewport);
    const QString idle = viewport->coordinateReadout();
    QVERIFY2(idle.contains(QStringLiteral("Cam (")), qPrintable(idle));
    QVERIFY2(idle.contains(QStringLiteral("Ref (-)")), qPrintable(idle));

    // Free-fly movement updates the camera half of the readout.
    QTest::keyClick(viewport, Qt::Key_S);
    const QString moved = viewport->coordinateReadout();
    QVERIFY2(moved != idle, qPrintable(moved));
    QVERIFY2(moved.contains(QStringLiteral("Ref (-)")), qPrintable(moved));

    // Focusing a placed reference reports its game-unit position. (W/E/R are
    // transform-mode shortcuts once a reference is selected, so the camera step
    // above has to run first.)
    viewport->setSelectedRefByDataIndex(0);
    const QString selected = viewport->coordinateReadout();
    QVERIFY2(selected.contains(QStringLiteral("Ref (100.0, 200.0, 300.0)")), qPrintable(selected));
}

void TestMainWindow::testDocksMenuListsEveryDock()
{
    Fixture f;
    QMenu* docksMenu = f.window.findChild<QMenu*>(QStringLiteral("menuDocks"));
    QVERIFY(docksMenu);

    // Docks are created lazily, so the list is rebuilt whenever the menu opens.
    QMetaObject::invokeMethod(docksMenu, "aboutToShow", Qt::DirectConnection);

    QStringList entries;
    for (QAction* action : docksMenu->actions())
    {
        if (action->isSeparator())
            continue;
        entries << QString(action->text()).remove(QLatin1Char('&'));
    }

    QVERIFY(entries.contains(QStringLiteral("Object Window")));
    QVERIFY(entries.contains(QStringLiteral("Cell View")));
    QVERIFY(entries.contains(QStringLiteral("Inspector")));
    QVERIFY(entries.contains(QStringLiteral("Warnings")));
    QVERIFY(entries.contains(QStringLiteral("Object Palette")));
    // The Render Window is pinned: it has no show/hide entry.
    QVERIFY(!entries.contains(QStringLiteral("Render Window")));
}

void TestMainWindow::testLockDocksFreezesDockFeatures()
{
    Fixture f;
    auto* manager = f.window.findChild<ads::CDockManager*>();
    ads::CDockWidget* objectDock = manager->findDockWidget(QStringLiteral("Object Window"));
    QVERIFY(objectDock);
    QVERIFY(objectDock->features().testFlag(ads::CDockWidget::DockWidgetMovable));
    QVERIFY(objectDock->features().testFlag(ads::CDockWidget::DockWidgetFloatable));

    QAction* lock = f.window.findChild<QAction*>("actionLockDocks");
    QVERIFY(lock);
    lock->setChecked(true);
    QVERIFY(!objectDock->features().testFlag(ads::CDockWidget::DockWidgetMovable));
    QVERIFY(!objectDock->features().testFlag(ads::CDockWidget::DockWidgetFloatable));

    lock->setChecked(false);
    QVERIFY(objectDock->features().testFlag(ads::CDockWidget::DockWidgetMovable));
    QVERIFY(objectDock->features().testFlag(ads::CDockWidget::DockWidgetFloatable));
}

void TestMainWindow::testWindowRendersLayout()
{
    Fixture f;
    f.window.resize(1400, 900);
    f.window.show();
    QTest::qWait(500);

    const QPixmap shot = f.window.grab();
    QVERIFY(!shot.isNull());
    // grab() returns device pixels; on a HiDPI-scaled display the captured
    // image is the window size multiplied by the device pixel ratio.
    const qreal dpr = f.window.devicePixelRatio();
    const QSize expected(qRound(f.window.width() * dpr),
                         qRound(f.window.height() * dpr));
    QCOMPARE(shot.size(), expected);

    QString dir = qEnvironmentVariable("OPENCK_LOG_DIR");
    if (dir.isEmpty())
        dir = QDir::tempPath();
    QDir().mkpath(dir);
    const QString path = dir + QStringLiteral("/phase3_mainwindow.png");
    QVERIFY2(shot.save(path), qPrintable(path));
    qInfo() << "Captured layout screenshot:" << path;

    // Mirror the application's close path before the window is destroyed.
    f.window.close();
    QTest::qWait(50);
    QFile::remove(QCoreApplication::applicationDirPath()
                  + QStringLiteral("/QtCreationKitSavedSettings.ini"));
}

QTEST_MAIN(TestMainWindow)
#include "test_mainwindow.moc"

#include <QtTest>
#include <QSignalSpy>
#include <QPushButton>

#include "../../src/view/window/previewwindowdock.hpp"
#include "../../src/view/window/nifviewportwidget.hpp"
#include "../../src/model/world/ckid.hpp"

class TestPreviewWindowDock : public QObject
{
    Q_OBJECT

private slots:
    void testStartsEmpty();
    void testPreviewRecordFallbackPrimitive();
    void testClear();
    void testSignalEmittedOnUpdate();
    void testShowPreviewPrimitive();
    void testResetCamera();
    void testGroundPlaneControls();
};

void TestPreviewWindowDock::testStartsEmpty()
{
    PreviewWindowDock dock;
    QVERIFY(dock.viewport() != nullptr);
    QCOMPARE(dock.infoText(), QStringLiteral("No object selected"));
    QVERIFY(dock.currentEditorId().isEmpty());
    QCOMPARE(dock.currentFormId(), 0u);
    QVERIFY(dock.currentModelPath().isEmpty());
    QCOMPARE(dock.triangleCount(), 0);
    QVERIFY(!dock.isGroundPlaneEnabled());
    QCOMPARE(dock.groundPlaneHeight(), 0.0f);
}

void TestPreviewWindowDock::testPreviewRecordFallbackPrimitive()
{
    PreviewWindowDock dock;
    dock.previewRecord(QStringLiteral("IronSword"), 0x00012EB7,
                       QStringLiteral("meshes/weapons/iron/sword.nif"),
                       QString(), CkId::Type_Weap_);

    QCOMPARE(dock.currentEditorId(), QStringLiteral("IronSword"));
    QCOMPARE(dock.currentFormId(), 0x00012EB7u);
    QCOMPARE(dock.currentModelPath(), QStringLiteral("meshes/weapons/iron/sword.nif"));

    // Creation Kit Dialog 315 format: Preview Object: 'IronSword' (00012EB7) - N triangles, M #: meshes/weapons/iron/sword.nif
    QVERIFY(dock.infoText().startsWith(QStringLiteral("Preview Object: 'IronSword' (00012EB7)")));
    QVERIFY(dock.infoText().contains(QStringLiteral("triangles")));
    QVERIFY(dock.infoText().contains(QStringLiteral("M #: meshes/weapons/iron/sword.nif")));

    QVERIFY(dock.viewport()->isShowingPreviewPrimitive());
    QVERIFY(dock.triangleCount() > 0);
}

void TestPreviewWindowDock::testClear()
{
    PreviewWindowDock dock;
    dock.previewRecord(QStringLiteral("IronSword"), 0x00012EB7,
                       QStringLiteral("meshes/weapons/iron/sword.nif"),
                       QString(), CkId::Type_Weap_);
    QCOMPARE(dock.currentEditorId(), QStringLiteral("IronSword"));

    dock.clear();
    QCOMPARE(dock.infoText(), QStringLiteral("No object selected"));
    QVERIFY(dock.currentEditorId().isEmpty());
    QCOMPARE(dock.currentFormId(), 0u);
    QVERIFY(dock.currentModelPath().isEmpty());
    QCOMPARE(dock.triangleCount(), 0);
}

void TestPreviewWindowDock::testSignalEmittedOnUpdate()
{
    PreviewWindowDock dock;
    QSignalSpy spy(&dock, &PreviewWindowDock::previewUpdated);

    dock.previewRecord(QStringLiteral("TestStatic"), 0x00054321,
                       QStringLiteral("meshes/static/rock.nif"),
                       QString(), CkId::Type_Stat_);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("TestStatic"));
    QCOMPARE(spy.at(0).at(1).toUInt(), 0x00054321u);
}

void TestPreviewWindowDock::testShowPreviewPrimitive()
{
    PreviewWindowDock dock;
    dock.showPreviewPrimitive(PrimitiveMeshGenerator::Type::Sphere, 40.0f, QStringLiteral("MagicSphere"));

    QCOMPARE(dock.currentEditorId(), QStringLiteral("MagicSphere"));
    QVERIFY(dock.infoText().startsWith(QStringLiteral("Preview File: \"MagicSphere\"")));
    QVERIFY(dock.infoText().contains(QStringLiteral("triangles")));
    QVERIFY(dock.viewport()->isShowingPreviewPrimitive());
    QVERIFY(dock.triangleCount() > 0);
}

void TestPreviewWindowDock::testResetCamera()
{
    PreviewWindowDock dock;
    dock.onResetCamera();
    dock.zoomToFit();
}

void TestPreviewWindowDock::testGroundPlaneControls()
{
    PreviewWindowDock dock;
    QVERIFY(!dock.isGroundPlaneEnabled());
    dock.setGroundPlaneEnabled(true);
    QVERIFY(dock.isGroundPlaneEnabled());

    dock.setGroundPlaneHeight(75.0f);
    QCOMPARE(dock.groundPlaneHeight(), 75.0f);

    dock.setGroundPlaneEnabled(false);
    QVERIFY(!dock.isGroundPlaneEnabled());
}

#include "test_previewwindowdock.moc"
QTEST_MAIN(TestPreviewWindowDock)

#include <QtTest>
#include <QApplication>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

#include "../../src/view/window/npcrecorddatawidget.hpp"
#include "../../src/view/window/racedatawidget.hpp"
#include "../../src/view/window/celldatawidget.hpp"
#include "../../src/view/window/questdatawidget.hpp"
#include "../../src/view/window/wthrdatawidget.hpp"
#include "../../src/view/window/sounddatawidget.hpp"
#include "../../src/view/window/classdatawidget.hpp"
#include "../../src/view/window/dialdatawidget.hpp"
#include "../../src/view/window/infodatawidget.hpp"

#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/esm/racerecord.hpp"
#include "../../libs/files/esm/cellrecord.hpp"
#include "../../libs/files/esm/questrecord.hpp"
#include "../../libs/files/esm/wthrrecord.hpp"
#include "../../libs/files/esm/sounrecord.hpp"
#include "../../libs/files/esm/classrecord.hpp"
#include "../../libs/files/esm/dialrecord.hpp"
#include "../../libs/files/esm/inforecord.hpp"

using namespace openck;

class TestEditorWidgets : public QObject
{
    Q_OBJECT

private slots:
    void testNpcRecordDataWidgetNull();
    void testNpcRecordDataWidgetValid();
    void testRaceDataWidgetNull();
    void testRaceDataWidgetValid();
    void testCellDataWidgetNull();
    void testCellDataWidgetValid();
    void testQuestDataWidgetNull();
    void testQuestDataWidgetValid();
    void testWthrDataWidgetNull();
    void testWthrDataWidgetValid();
    void testSoundDataWidgetNull();
    void testSoundDataWidgetValid();
    void testClassDataWidgetNull();
    void testClassDataWidgetValid();
    void testDialDataWidgetNull();
    void testDialDataWidgetValid();
    void testInfoDataWidgetNull();
    void testInfoDataWidgetValid();
};

template <typename WidgetT>
static bool widgetHasLayout(QWidget* w)
{
    return w != nullptr && w->layout() != nullptr;
}

template <typename WidgetT, typename RecordT>
static std::unique_ptr<WidgetT> makeWidget(RecordT* rec)
{
    return std::make_unique<WidgetT>(rec, rec ? &rec->components : nullptr);
}

// ---------------------------------------------------------------------------
// NpcRecordDataWidget
// ---------------------------------------------------------------------------
void TestEditorWidgets::testNpcRecordDataWidgetNull()
{
    auto w = makeWidget<NpcRecordDataWidget, NpcRecord>(nullptr);
    QVERIFY(widgetHasLayout<NpcRecordDataWidget>(w.get()));
    auto groupBoxes = w->findChildren<QGroupBox*>();
    QVERIFY(!groupBoxes.isEmpty());
}

void TestEditorWidgets::testNpcRecordDataWidgetValid()
{
    NpcRecord rec;
    rec.race = 0xDEADBEEF;
    rec.class_ = 0xCAFEBABE;
    rec.health = 100;
    rec.magicka = 50;
    rec.stamina = 75;
    auto w = makeWidget<NpcRecordDataWidget, NpcRecord>(&rec);
    QVERIFY(widgetHasLayout<NpcRecordDataWidget>(w.get()));
    auto edits = w->findChildren<QLineEdit*>();
    QVERIFY(edits.size() >= 2);
    QVERIFY(edits[0]->text().contains(QStringLiteral("deadbeef"), Qt::CaseInsensitive));
}

// ---------------------------------------------------------------------------
// RaceDataWidget
// ---------------------------------------------------------------------------
void TestEditorWidgets::testRaceDataWidgetNull()
{
    auto w = makeWidget<RaceDataWidget, RaceRecord>(nullptr);
    QVERIFY(widgetHasLayout<RaceDataWidget>(w.get()));
    auto labels = w->findChildren<QLabel*>();
    QVERIFY(!labels.isEmpty());
}

void TestEditorWidgets::testRaceDataWidgetValid()
{
    RaceRecord rec;
    rec.raceFlags = 0x77;
    rec.npcVariables = {0x1, 0x2, 0x3};
    auto w = makeWidget<RaceDataWidget, RaceRecord>(&rec);
    QVERIFY(widgetHasLayout<RaceDataWidget>(w.get()));
    auto groupBoxes = w->findChildren<QGroupBox*>();
    QVERIFY(groupBoxes.size() >= 2);
}

// ---------------------------------------------------------------------------
// CellDataWidget
// ---------------------------------------------------------------------------
void TestEditorWidgets::testCellDataWidgetNull()
{
    auto w = makeWidget<CellDataWidget, CellRecord>(nullptr);
    QVERIFY(widgetHasLayout<CellDataWidget>(w.get()));
    auto labels = w->findChildren<QLabel*>();
    QVERIFY(!labels.isEmpty());
}

void TestEditorWidgets::testCellDataWidgetValid()
{
    CellRecord rec;
    rec.cellX = 5;
    rec.cellY = -3;
    rec.owner = 0xAB;
    rec.lockLevel = 25;
    auto w = makeWidget<CellDataWidget, CellRecord>(&rec);
    QVERIFY(widgetHasLayout<CellDataWidget>(w.get()));
    auto groupBoxes = w->findChildren<QGroupBox*>();
    QVERIFY(!groupBoxes.isEmpty());
    auto spins = w->findChildren<QSpinBox*>();
    QVERIFY(spins.size() >= 2);
    QCOMPARE(spins[0]->value(), 5);
}

// ---------------------------------------------------------------------------
// QuestDataWidget
// ---------------------------------------------------------------------------
void TestEditorWidgets::testQuestDataWidgetNull()
{
    auto w = makeWidget<QuestDataWidget, QuestRecord>(nullptr);
    QVERIFY(widgetHasLayout<QuestDataWidget>(w.get()));
    auto labels = w->findChildren<QLabel*>();
    QVERIFY(!labels.isEmpty());
}

void TestEditorWidgets::testQuestDataWidgetValid()
{
    QuestRecord rec;
    rec.questDesc = QStringLiteral("Test quest");
    rec.questType = 2;
    rec.stageIds = {10, 20};
    rec.stageDescriptions = {QStringLiteral("A"), QStringLiteral("B")};
    auto w = makeWidget<QuestDataWidget, QuestRecord>(&rec);
    QVERIFY(widgetHasLayout<QuestDataWidget>(w.get()));
    auto groupBoxes = w->findChildren<QGroupBox*>();
    QVERIFY(groupBoxes.size() >= 2);
}

// ---------------------------------------------------------------------------
// WthrDataWidget
// ---------------------------------------------------------------------------
void TestEditorWidgets::testWthrDataWidgetNull()
{
    auto w = makeWidget<WthrDataWidget, WthrRecord>(nullptr);
    QVERIFY(widgetHasLayout<WthrDataWidget>(w.get()));
    auto groupBoxes = w->findChildren<QGroupBox*>();
    QVERIFY(!groupBoxes.isEmpty());
}

void TestEditorWidgets::testWthrDataWidgetValid()
{
    WthrRecord rec;
    rec.sunTexture = QStringLiteral("weather/sun.dds");
    rec.flags = 0x88;
    auto w = makeWidget<WthrDataWidget, WthrRecord>(&rec);
    QVERIFY(widgetHasLayout<WthrDataWidget>(w.get()));
    auto edits = w->findChildren<QLineEdit*>();
    QVERIFY(!edits.isEmpty());
    QCOMPARE(edits[0]->text(), QStringLiteral("weather/sun.dds"));
}

// ---------------------------------------------------------------------------
// SoundDataWidget
// ---------------------------------------------------------------------------
void TestEditorWidgets::testSoundDataWidgetNull()
{
    auto w = makeWidget<SoundDataWidget, SounRecord>(nullptr);
    QVERIFY(widgetHasLayout<SoundDataWidget>(w.get()));
    auto groupBoxes = w->findChildren<QGroupBox*>();
    QVERIFY(!groupBoxes.isEmpty());
}

void TestEditorWidgets::testSoundDataWidgetValid()
{
    SounRecord rec;
    rec.soundFile = QStringLiteral("sound/foo.wav");
    rec.flags = 0x42;
    auto w = makeWidget<SoundDataWidget, SounRecord>(&rec);
    QVERIFY(widgetHasLayout<SoundDataWidget>(w.get()));
    auto edits = w->findChildren<QLineEdit*>();
    QVERIFY(!edits.isEmpty());
    QCOMPARE(edits[0]->text(), QStringLiteral("sound/foo.wav"));
}

// ---------------------------------------------------------------------------
// ClassDataWidget
// ---------------------------------------------------------------------------
void TestEditorWidgets::testClassDataWidgetNull()
{
    auto w = makeWidget<ClassDataWidget, ClassRecord>(nullptr);
    QVERIFY(widgetHasLayout<ClassDataWidget>(w.get()));
    auto groupBoxes = w->findChildren<QGroupBox*>();
    QVERIFY(!groupBoxes.isEmpty());
}

void TestEditorWidgets::testClassDataWidgetValid()
{
    ClassRecord rec;
    rec.className = QStringLiteral("Warrior");
    rec.description = QStringLiteral("Fighter class");
    rec.serviceFlags = 0x10;
    rec.iconPath = QStringLiteral("icons/warrior.dds");
    auto w = makeWidget<ClassDataWidget, ClassRecord>(&rec);
    QVERIFY(widgetHasLayout<ClassDataWidget>(w.get()));
    auto edits = w->findChildren<QLineEdit*>();
    QVERIFY(edits.size() >= 4);
    QCOMPARE(edits[0]->text(), QStringLiteral("Warrior"));
}

// ---------------------------------------------------------------------------
// DialDataWidget
// ---------------------------------------------------------------------------
void TestEditorWidgets::testDialDataWidgetNull()
{
    auto w = makeWidget<DialDataWidget, DialRecord>(nullptr);
    QVERIFY(widgetHasLayout<DialDataWidget>(w.get()));
    auto labels = w->findChildren<QLabel*>();
    QVERIFY(!labels.isEmpty());
}

void TestEditorWidgets::testDialDataWidgetValid()
{
    DialRecord rec;
    rec.topicName = QStringLiteral("Hello");
    rec.responseIds = {0x1, 0x2};
    auto w = makeWidget<DialDataWidget, DialRecord>(&rec);
    QVERIFY(widgetHasLayout<DialDataWidget>(w.get()));
    auto groupBoxes = w->findChildren<QGroupBox*>();
    QVERIFY(groupBoxes.size() >= 2);
}

// ---------------------------------------------------------------------------
// InfoDataWidget
// ---------------------------------------------------------------------------
void TestEditorWidgets::testInfoDataWidgetNull()
{
    auto w = makeWidget<InfoDataWidget, InfoRecord>(nullptr);
    QVERIFY(widgetHasLayout<InfoDataWidget>(w.get()));
    auto labels = w->findChildren<QLabel*>();
    QVERIFY(!labels.isEmpty());
}

void TestEditorWidgets::testInfoDataWidgetValid()
{
    InfoRecord rec;
    rec.responseText = QStringLiteral("Hi there");
    rec.targetId = 0xFEEDFACE;
    rec.conditionIds = {0x1, 0x2, 0x3};
    auto w = makeWidget<InfoDataWidget, InfoRecord>(&rec);
    QVERIFY(widgetHasLayout<InfoDataWidget>(w.get()));
    auto groupBoxes = w->findChildren<QGroupBox*>();
    QVERIFY(groupBoxes.size() >= 2);
}

#include "test_editor_widgets.moc"
QTEST_MAIN(TestEditorWidgets)
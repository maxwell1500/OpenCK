#include <QTest>
#include <QComboBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QMetaObject>

#include "../../src/view/window/materialeditor.hpp"
#include "../../src/model/tools/materialruletemplate.hpp"
#include "../../src/model/world/data.hpp"
#include "../../libs/files/esm/materialrecord.hpp"
#include "../../libs/files/log/logger.hpp"

class TestMaterialEditor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testTemplateComboPopulated();
    void testApplyTemplateFillsSlotTable();
    void testSaveWritesTextureSlots();
};

void TestMaterialEditor::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_materialeditor_log.txt"));
}

void TestMaterialEditor::testTemplateComboPopulated()
{
    FilePaths paths;
    Data data(QStringList(), paths);
    MaterialRecord rec;
    rec.editorId = QStringLiteral("TestMaterial");

    MaterialEditor editor(&data, &rec);

    // No RuleTemplates directory on this data root -> the combo falls back
    // to the built-in template names.
    QComboBox* combo = editor.findChild<QComboBox*>();
    QVERIFY(combo);
    QVERIFY(combo->count() >= MaterialRuleTemplate::builtinNames().size());
    editor.close();
}

void TestMaterialEditor::testApplyTemplateFillsSlotTable()
{
    FilePaths paths;
    Data data(QStringList(), paths);
    MaterialRecord rec;
    rec.editorId = QStringLiteral("TestMaterial");

    MaterialEditor editor(&data, &rec);

    QTabWidget* tabs = editor.findChild<QTabWidget*>();
    QVERIFY(tabs);
    tabs->setCurrentIndex(1);   // Rule Templates tab

    QComboBox* combo = editor.findChild<QComboBox*>();
    QVERIFY(combo);
    combo->setCurrentIndex(0);

    QMetaObject::invokeMethod(&editor, "applyTemplate", Qt::DirectConnection);

    QTableWidget* table = editor.findChild<QTableWidget*>();
    QVERIFY(table);
    QCOMPARE(table->rowCount(), MaterialRuleTemplate::builtinLayerSlots().size());

    // The first column holds the slot names.
    for (int r = 0; r < table->rowCount(); ++r) {
        QVERIFY(table->item(r, 0));
        QVERIFY(!table->item(r, 0)->text().isEmpty());
    }
    editor.close();
}

void TestMaterialEditor::testSaveWritesTextureSlots()
{
    FilePaths paths;
    Data data(QStringList(), paths);
    MaterialRecord rec;
    rec.editorId = QStringLiteral("SaveMaterial");
    rec.formId = 0x50001;

    MaterialEditor editor(&data, &rec);

    QTabWidget* tabs = editor.findChild<QTabWidget*>();
    QVERIFY(tabs);
    tabs->setCurrentIndex(1);

    QComboBox* combo = editor.findChild<QComboBox*>();
    QVERIFY(combo);
    combo->setCurrentIndex(0);

    QMetaObject::invokeMethod(&editor, "applyTemplate", Qt::DirectConnection);
    QMetaObject::invokeMethod(&editor, "saveChanges", Qt::DirectConnection);

    // saveChanges() accepts (closes) the dialog after saving.
    QCOMPARE(rec.textureSlots.size(), MaterialRuleTemplate::builtinLayerSlots().size());
    QVERIFY(rec.textureSlots.contains(QStringLiteral("Diffuse")));
    QVERIFY(rec.textureSlots.contains(QStringLiteral("Normal")));
    // Slot paths are empty until the user assigns textures.
    QCOMPARE(rec.textureSlots.value(QStringLiteral("Diffuse")).isEmpty(), true);
    editor.close();
}

QTEST_MAIN(TestMaterialEditor)
#include "test_materialeditor.moc"

#include <QTest>
#include <QTemporaryFile>
#include <QJsonArray>
#include <QJsonObject>

#include "../../src/model/tools/materialruletemplate.hpp"
#include "../../libs/files/log/logger.hpp"

class TestMaterialRuleTemplate : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testFromJson();
    void testLoadFile();
    void testBuiltinNames();
};

void TestMaterialRuleTemplate::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_materialrule_log.txt"));
}

void TestMaterialRuleTemplate::testFromJson()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("name"), QStringLiteral("2LayerStandard"));
    obj.insert(QStringLiteral("shaderModel"), QStringLiteral("BSLightingShaderProperty"));
    obj.insert(QStringLiteral("layerCount"), 2);

    QJsonArray ops;
    QJsonObject add;
    add.insert(QStringLiteral("op"), QStringLiteral("Add"));
    add.insert(QStringLiteral("target"), QStringLiteral("Albedo"));
    ops.append(add);
    QJsonObject makeConst;
    makeConst.insert(QStringLiteral("op"), QStringLiteral("MakeConst"));
    makeConst.insert(QStringLiteral("target"), QStringLiteral("Layer1"));
    ops.append(makeConst);
    obj.insert(QStringLiteral("operations"), ops);

    const MaterialRuleTemplate tpl = MaterialRuleTemplate::fromJson(obj);
    QCOMPARE(tpl.name, QStringLiteral("2LayerStandard"));
    QCOMPARE(tpl.shaderModel, QStringLiteral("BSLightingShaderProperty"));
    QCOMPARE(tpl.layerCount, 2);
    QCOMPARE(tpl.operations.size(), 2);
    QCOMPARE(tpl.operations[0].op, QStringLiteral("Add"));
    QCOMPARE(tpl.operations[1].target, QStringLiteral("Layer1"));
}

void TestMaterialRuleTemplate::testLoadFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(R"({
        "templates": [
            { "name": "Terrain", "shaderModel": "BSShaderTerrain", "layerCount": 1 },
            { "name": "", "layerCount": 3 },
            { "name": "Skin", "layerCount": 1, "operations": [ { "op": "Add", "target": "Normal" } ] }
        ]
    })");
    file.close();

    QVector<MaterialRuleTemplate> out;
    QVERIFY(MaterialRuleTemplate::loadFile(file.fileName(), out));
    QCOMPARE(out.size(), 2);   // nameless entry skipped
    QCOMPARE(out[0].name, QStringLiteral("Terrain"));
    QCOMPARE(out[1].name, QStringLiteral("Skin"));
    QCOMPARE(out[1].operations.size(), 1);

    QVector<MaterialRuleTemplate> none;
    QVERIFY(!MaterialRuleTemplate::loadFile(QStringLiteral("Z:/missing.json"), none));
}

void TestMaterialRuleTemplate::testBuiltinNames()
{
    const QStringList names = MaterialRuleTemplate::builtinNames();
    QCOMPARE(names.size(), 10);
    QVERIFY(names.contains(QStringLiteral("1LayerStandard")));
    QVERIFY(names.contains(QStringLiteral("4LayerStandard")));
    QVERIFY(names.contains(QStringLiteral("Terrain")));
    QVERIFY(names.contains(QStringLiteral("Water")));
}

QTEST_MAIN(TestMaterialRuleTemplate)
#include "test_materialruletemplate.moc"

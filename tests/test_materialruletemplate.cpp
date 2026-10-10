#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>

#include "../../src/model/tools/materialruletemplate.hpp"
#include "../../libs/files/log/logger.hpp"

class TestMaterialRuleTemplate : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testFromJsonRealShape();
    void testLoadFile();
    void testLoadFileArrayTopLevel();
    void testLoadDirectory();
    void testPatternMatching();
    void testTextureSlots();
    void testApplyToSlots();
    void testApplyToSlotMapMove();
    void testRequiredSlots();
    void testBuiltinNames();
    void testBuiltinTemplate();
};

void TestMaterialRuleTemplate::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_materialrule_log.txt"));
}

void TestMaterialRuleTemplate::testFromJsonRealShape()
{
    // The exact on-disk shape of a real Starfield RuleTemplates/ShaderModels
    // file (abridged from 1LayerStandard.json).
    QJsonObject obj;
    obj.insert(QStringLiteral("Category"), QStringLiteral("ShaderModels"));
    QJsonObject meta;
    meta.insert(QStringLiteral("RootMaterial"), QStringLiteral("1LayerStandard"));
    meta.insert(QStringLiteral("DisplayName"), QStringLiteral("Standard1Layer"));
    meta.insert(QStringLiteral("ComplexityCost"), 10);
    obj.insert(QStringLiteral("MetaData"), meta);
    obj.insert(QStringLiteral("Name"), QStringLiteral("1LayerStandard"));

    QJsonArray rules;
    QJsonObject rm; rm.insert(QStringLiteral("From"), QStringLiteral("*")); rm.insert(QStringLiteral("Op"), QStringLiteral("Remove"));
    rules.append(rm);
    QJsonObject add; add.insert(QStringLiteral("From"), QStringLiteral("Layer1")); add.insert(QStringLiteral("Op"), QStringLiteral("Add"));
    rules.append(add);
    QJsonObject rem; rem.insert(QStringLiteral("From"), QStringLiteral("Layer1/Material/TextureSet/Emissive")); rem.insert(QStringLiteral("Op"), QStringLiteral("Remove"));
    rules.append(rem);
    QJsonObject mv; mv.insert(QStringLiteral("From"), QStringLiteral("Layer1/Material/TextureSet/Diffuse")); mv.insert(QStringLiteral("To"), QStringLiteral("Layer1/Material/TextureSet/Albedo")); mv.insert(QStringLiteral("Op"), QStringLiteral("Move"));
    rules.append(mv);

    QJsonObject ruleClass;
    ruleClass.insert(QStringLiteral("Class"), QStringLiteral("BSMaterial::LayeredMaterialID"));
    ruleClass.insert(QStringLiteral("Rules"), rules);
    QJsonArray ruleClasses;
    ruleClasses.append(ruleClass);
    obj.insert(QStringLiteral("TemplateRules"), ruleClasses);
    obj.insert(QStringLiteral("Version"), QStringLiteral("1"));

    const MaterialRuleTemplate tpl = MaterialRuleTemplate::fromJson(obj);
    QCOMPARE(tpl.category, QStringLiteral("ShaderModels"));
    QCOMPARE(tpl.name, QStringLiteral("1LayerStandard"));
    QCOMPARE(tpl.displayName, QStringLiteral("Standard1Layer"));
    QCOMPARE(tpl.rootMaterial, QStringLiteral("1LayerStandard"));
    QCOMPARE(tpl.complexityCost, 10);
    QCOMPARE(tpl.version, QStringLiteral("1"));
    QCOMPARE(tpl.ruleClasses.size(), 1);
    QCOMPARE(tpl.ruleClasses[0].className, QStringLiteral("BSMaterial::LayeredMaterialID"));
    QCOMPARE(tpl.ruleClasses[0].rules.size(), 4);
    QCOMPARE(tpl.ruleClasses[0].rules[0].from, QStringLiteral("*"));
    QCOMPARE(tpl.ruleClasses[0].rules[0].op, QStringLiteral("Remove"));
    QCOMPARE(tpl.ruleClasses[0].rules[1].from, QStringLiteral("Layer1"));
    QCOMPARE(tpl.ruleClasses[0].rules[3].op, QStringLiteral("Move"));
}

void TestMaterialRuleTemplate::testLoadFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(R"({
        "Category": "ShaderModels",
        "MetaData": { "RootMaterial": "Skin", "DisplayName": "Skin", "ComplexityCost": 20 },
        "Name": "Skin",
        "TemplateRules": [
            { "Class": "BSMaterial::LayeredMaterialID",
              "Rules": [
                { "From": "*", "Op": "Remove" },
                { "From": "Layer1", "Op": "Add" }
              ] }
        ],
        "Version": "1"
    })");
    file.close();

    MaterialRuleTemplate tpl;
    QVERIFY(MaterialRuleTemplate::loadFile(file.fileName(), tpl));
    QCOMPARE(tpl.name, QStringLiteral("Skin"));
    QCOMPARE(tpl.ruleClasses.size(), 1);
    QCOMPARE(tpl.ruleClasses[0].rules.size(), 2);

    MaterialRuleTemplate missing;
    QVERIFY(!MaterialRuleTemplate::loadFile(QStringLiteral("Z:/missing.json"), missing));
}

void TestMaterialRuleTemplate::testLoadFileArrayTopLevel()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(R"([
        { "Name": "First", "TemplateRules": [] },
        { "Name": "Second", "TemplateRules": [] }
    ])");
    file.close();

    MaterialRuleTemplate tpl;
    QVERIFY(MaterialRuleTemplate::loadFile(file.fileName(), tpl));
    QCOMPARE(tpl.name, QStringLiteral("First"));
}

void TestMaterialRuleTemplate::testLoadDirectory()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto writeJson = [&](const QString& name, const QString& body) {
        QFile f(dir.filePath(name));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(body.toUtf8());
        f.close();
    };
    writeJson(QStringLiteral("B.json"), R"({ "Name": "B", "TemplateRules": [] })");
    writeJson(QStringLiteral("A.json"), R"({ "Name": "A", "TemplateRules": [] })");
    writeJson(QStringLiteral("notjson.txt"), R"({ "Name": "C", "TemplateRules": [] })");

    QVector<MaterialRuleTemplate> out;
    const int n = MaterialRuleTemplate::loadDirectory(dir.path(), out);
    QCOMPARE(n, 2);
    QCOMPARE(out.size(), 2);
    QCOMPARE(out[0].name, QStringLiteral("A"));
    QCOMPARE(out[1].name, QStringLiteral("B"));

    QVector<MaterialRuleTemplate> empty;
    QCOMPARE(MaterialRuleTemplate::loadDirectory(QStringLiteral("Z:/missing-dir"), empty), 0);
}

void TestMaterialRuleTemplate::testPatternMatching()
{
    // Bare "*" matches any path at any depth.
    QVERIFY(MaterialRuleTemplate::patternMatches(QStringLiteral("*"), QStringLiteral("Layer1/Material/TextureSet/Diffuse")));
    QVERIFY(MaterialRuleTemplate::patternMatches(QStringLiteral("*"), QStringLiteral("Layer1")));

    // A literal segment matches itself.
    QVERIFY(MaterialRuleTemplate::patternMatches(QStringLiteral("Layer1"), QStringLiteral("Layer1")));
    QVERIFY(!MaterialRuleTemplate::patternMatches(QStringLiteral("Layer1"), QStringLiteral("Layer2")));

    // An inner "*" segment matches within that segment (not across "/").
    QVERIFY(MaterialRuleTemplate::patternMatches(QStringLiteral("Layer*/Material"), QStringLiteral("Layer3/Material")));
    QVERIFY(!MaterialRuleTemplate::patternMatches(QStringLiteral("Layer*/Material"), QStringLiteral("Layer3/Other")));

    // A trailing "/*" matches any remaining depth.
    QVERIFY(MaterialRuleTemplate::patternMatches(QStringLiteral("Layer1/*"), QStringLiteral("Layer1/Material/TextureSet/Diffuse")));
    QVERIFY(MaterialRuleTemplate::patternMatches(QStringLiteral("Layer1/*"), QStringLiteral("Layer1/Material")));
    QVERIFY(!MaterialRuleTemplate::patternMatches(QStringLiteral("Layer1/*"), QStringLiteral("Layer2/Material")));

    // Full path with a slot wildcard.
    QVERIFY(MaterialRuleTemplate::patternMatches(
        QStringLiteral("Layer1/Material/TextureSet/*"),
        QStringLiteral("Layer1/Material/TextureSet/Diffuse")));

    // patternToRegex is anchored: no accidental prefix/suffix match.
    QVERIFY(!MaterialRuleTemplate::patternToRegex(QStringLiteral("Layer1")).match(QStringLiteral("XLay1")).hasMatch());
    QVERIFY(!MaterialRuleTemplate::patternToRegex(QStringLiteral("Layer1")).match(QStringLiteral("Layer12")).hasMatch());
}

void TestMaterialRuleTemplate::testTextureSlots()
{
    MaterialRuleTemplate tpl;
    tpl.ruleClasses.append({QStringLiteral("null"), {
        {QStringLiteral("Layer1/Material/TextureSet/Diffuse"), QStringLiteral("Layer1/Material/TextureSet/Albedo"), QStringLiteral("Move")},
        {QStringLiteral("Layer1/Material/TextureSet/Normal"), QString(), QStringLiteral("Remove")},
        {QStringLiteral("Layer1/Material/TextureSet/Diffuse"), QString(), QStringLiteral("MakeConst")}
    }});

    const QStringList slotList = tpl.textureSlots();
    // First-appearance order, deduplicated, slot = segment after "TextureSet".
    const QStringList expectedSlots = {QStringLiteral("Diffuse"), QStringLiteral("Normal")};
    QCOMPARE(slotList, expectedSlots);
}

void TestMaterialRuleTemplate::testApplyToSlots()
{
    // 1LayerStandard flow: Remove * -> Add Layer1 -> Remove Emissive -> MakeConst VertexColorChannel
    MaterialRuleTemplate tpl;
    tpl.name = QStringLiteral("1LayerStandard");
    tpl.ruleClasses.append({QStringLiteral("BSMaterial::LayeredMaterialID"), {
        {QStringLiteral("*"), QString(), QStringLiteral("Remove")},
        {QStringLiteral("Layer1"), QString(), QStringLiteral("Add")},
        {QStringLiteral("Layer1/Material/TextureSet/Emissive"), QString(), QStringLiteral("Remove")},
        {QStringLiteral("Layer1/Material/TextureSet/VertexColorChannel"), QString(), QStringLiteral("MakeConst")}
    }});

    const MaterialRuleTemplate::ApplyResult res = tpl.applyToSlots({QStringLiteral("Diffuse"), QStringLiteral("Scratch")});
    // After Remove * the set is cleared; Add Layer1 adds all builtin slots;
    // Emissive and VertexColorChannel are then dropped.
    QVERIFY(!res.slotNames.contains(QStringLiteral("Scratch")));
    QVERIFY(res.slotNames.contains(QStringLiteral("Diffuse")));
    QVERIFY(!res.slotNames.contains(QStringLiteral("Emissive")));
    QVERIFY(!res.slotNames.contains(QStringLiteral("VertexColorChannel")));
    // The remaining builtin layer slots are present.
    const QStringList builtins = MaterialRuleTemplate::builtinLayerSlots();
    for (const QString& s : builtins) {
        if (s == QStringLiteral("Emissive") || s == QStringLiteral("VertexColorChannel")) continue;
        QVERIFY2(res.slotNames.contains(s), qPrintable(s));
    }
    QVERIFY(!res.operations.isEmpty());
}

void TestMaterialRuleTemplate::testApplyToSlotMapMove()
{
    MaterialRuleTemplate tpl;
    tpl.ruleClasses.append({QStringLiteral("BSMaterial::LayeredMaterialID"), {
        {QStringLiteral("Layer1/Material/TextureSet/Diffuse"), QStringLiteral("Layer1/Material/TextureSet/Albedo"), QStringLiteral("Move")}
    }});

    QMap<QString, QString> in;
    in[QStringLiteral("Diffuse")] = QStringLiteral("textures/diff.dds");
    in[QStringLiteral("Normal")] = QStringLiteral("textures/nrm.dds");

    const MaterialRuleTemplate::ApplyMapResult res = tpl.applyToSlotMap(in);
    // Move renames the slot key, preserving its texture path.
    QVERIFY(res.slotPaths.contains(QStringLiteral("Albedo")));
    QCOMPARE(res.slotPaths.value(QStringLiteral("Albedo")), QStringLiteral("textures/diff.dds"));
    QVERIFY(!res.slotPaths.contains(QStringLiteral("Diffuse")));
    QCOMPARE(res.slotPaths.value(QStringLiteral("Normal")), QStringLiteral("textures/nrm.dds"));
}

void TestMaterialRuleTemplate::testRequiredSlots()
{
    MaterialRuleTemplate tpl = MaterialRuleTemplate::builtinTemplate(QStringLiteral("1LayerStandard"));
    const QStringList req = tpl.requiredSlots();
    QVERIFY(!req.isEmpty());
    QVERIFY(req.contains(QStringLiteral("Diffuse")));
    // The builtin fallback is "Remove * + Add Layer1" with no per-slot
    // removals, so the required slots are exactly the builtin layer set.
    const QStringList builtinSlotNames = MaterialRuleTemplate::builtinLayerSlots();
    const QSet<QString> reqSet(req.begin(), req.end());
    const QSet<QString> builtinSet(builtinSlotNames.begin(), builtinSlotNames.end());
    QCOMPARE(reqSet, builtinSet);
}

void TestMaterialRuleTemplate::testBuiltinNames()
{
    const QStringList names = MaterialRuleTemplate::builtinNames();
    QVERIFY(names.size() >= 10);
    QVERIFY(names.contains(QStringLiteral("1LayerStandard")));
    QVERIFY(names.contains(QStringLiteral("4LayerStandard")));
    QVERIFY(names.contains(QStringLiteral("Terrain")));
    QVERIFY(names.contains(QStringLiteral("Water")));
    QVERIFY(names.contains(QStringLiteral("Skin")));
    QVERIFY(names.contains(QStringLiteral("Hair")));
}

void TestMaterialRuleTemplate::testBuiltinTemplate()
{
    const MaterialRuleTemplate tpl = MaterialRuleTemplate::builtinTemplate(QStringLiteral("Skin"));
    QCOMPARE(tpl.name, QStringLiteral("Skin"));
    QVERIFY(!tpl.requiredSlots().isEmpty());
}

QTEST_MAIN(TestMaterialRuleTemplate)
#include "test_materialruletemplate.moc"

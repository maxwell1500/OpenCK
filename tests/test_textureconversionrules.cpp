#include <QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>
#include <QImage>

#include "../../src/model/tools/textureconversionrules.hpp"
#include "../../src/model/tools/assetconverter.hpp"

class TestTextureConversionRules : public QObject
{
    Q_OBJECT

private slots:
    void testFormatToStringRoundTrip();
    void testParseArray();
    void testParseObjectWithRules();
    void testParseSettingsObject();
    void testFindRuleGlob();
    void testFindRuleNormalMap();
    void testBuiltin();
    void testInvalidJson();
    void testRuleAwareConversion();
};

void TestTextureConversionRules::testFormatToStringRoundTrip()
{
    QCOMPARE(TextureConversionRule::formatToString(TextureConversionRule::Format::BC7),
             QStringLiteral("BC7"));
    QCOMPARE(TextureConversionRule::formatToString(TextureConversionRule::Format::BC5),
             QStringLiteral("BC5"));
    QCOMPARE(TextureConversionRule::formatToString(TextureConversionRule::Format::BC4),
             QStringLiteral("BC4"));
    QCOMPARE(TextureConversionRule::formatToString(TextureConversionRule::Format::R8),
             QStringLiteral("R8"));
    QCOMPARE(TextureConversionRule::formatToString(TextureConversionRule::Format::R8G8B8A8),
             QStringLiteral("R8G8B8A8"));

    QCOMPARE(TextureConversionRule::stringToFormat(QStringLiteral("BC7_UNORM")),
             TextureConversionRule::Format::BC7);
    QCOMPARE(TextureConversionRule::stringToFormat(QStringLiteral("dxgi_format_bc5_unorm")),
             TextureConversionRule::Format::BC5);
    QCOMPARE(TextureConversionRule::stringToFormat(QStringLiteral("bogus")),
             TextureConversionRule::Format::Unknown);
}

void TestTextureConversionRules::testParseArray()
{
    const QString json = R"([
        { "path": "textures/actors/character/*", "format": "BC7", "mipmaps": true, "srgb": true },
        { "path": "*_n.dds", "format": "BC5", "mipmaps": true, "srgb": false },
        { "path": "textures/water/*", "format": "BC7", "physicallyBasedMipmaps": true }
    ])";

    QVector<TextureConversionRule> rules;
    QVERIFY(TextureConversionRules::parse(json, rules));
    QCOMPARE(rules.size(), 3);

    QCOMPARE(rules[0].pathPattern, QStringLiteral("textures/actors/character/*"));
    QCOMPARE(rules[0].format, TextureConversionRule::Format::BC7);
    QVERIFY(rules[0].generateMipmaps);
    QVERIFY(rules[0].srgb);

    QCOMPARE(rules[1].pathPattern, QStringLiteral("*_n.dds"));
    QCOMPARE(rules[1].format, TextureConversionRule::Format::BC5);
    QVERIFY(!rules[1].srgb);

    QVERIFY(rules[2].physicallyBasedMipmaps);
}

void TestTextureConversionRules::testParseObjectWithRules()
{
    const QString json = R"({
        "rules": [
            { "path": "textures/terrain/*", "format": "BC7", "distanceField": true }
        ]
    })";

    QVector<TextureConversionRule> rules;
    QVERIFY(TextureConversionRules::parse(json, rules));
    QCOMPARE(rules.size(), 1);
    QVERIFY(rules[0].distanceField);
    QCOMPARE(rules[0].format, TextureConversionRule::Format::BC7);
}

void TestTextureConversionRules::testParseSettingsObject()
{
    // xtexconv-style per-key settings object.
    const QString json = R"({
        "settings": {
            "textures/actors/character/*": { "format": "BC7", "mipmaps": true },
            "textures/water/*": { "format": "BC7", "srgb": true }
        }
    })";

    QVector<TextureConversionRule> rules;
    QVERIFY(TextureConversionRules::parse(json, rules));
    QCOMPARE(rules.size(), 2);
    QCOMPARE(rules[0].pathPattern, QStringLiteral("textures/actors/character/*"));
    QCOMPARE(rules[1].pathPattern, QStringLiteral("textures/water/*"));
}

void TestTextureConversionRules::testFindRuleGlob()
{
    QVector<TextureConversionRule> rules;
    TextureConversionRule r;
    r.pathPattern = QStringLiteral("textures/actors/character/*");
    r.format = TextureConversionRule::Format::BC7;
    rules.append(r);
    r = TextureConversionRule();
    r.pathPattern = QStringLiteral("textures/water/*");
    r.format = TextureConversionRule::Format::BC5;
    rules.append(r);

    const TextureConversionRule* hit = TextureConversionRules::findRule(
        rules, QStringLiteral("Textures/Actors/Character/Foo.dds"));
    QVERIFY(hit != nullptr);
    QCOMPARE(hit->format, TextureConversionRule::Format::BC7);

    const TextureConversionRule* miss = TextureConversionRules::findRule(
        rules, QStringLiteral("textures/weapons/sword.dds"));
    QVERIFY(miss == nullptr);
}

void TestTextureConversionRules::testFindRuleNormalMap()
{
    QVector<TextureConversionRule> rules;
    TextureConversionRule r;
    r.pathPattern = QStringLiteral("*_n.dds");
    r.format = TextureConversionRule::Format::BC5;
    rules.append(r);

    const TextureConversionRule* hit = TextureConversionRules::findRule(
        rules, QStringLiteral("textures/armor/plate_armor_n.dds"));
    QVERIFY(hit != nullptr);
    QCOMPARE(hit->format, TextureConversionRule::Format::BC5);

    QVERIFY(TextureConversionRules::findRule(
        rules, QStringLiteral("textures/armor/plate_armor.dds")) == nullptr);
}

void TestTextureConversionRules::testBuiltin()
{
    const QVector<TextureConversionRule> rules = TextureConversionRules::builtin();
    QCOMPARE(rules.size(), 4);
    QVERIFY(!rules.isEmpty());

    const TextureConversionRule* normal = TextureConversionRules::findRule(
        rules, QStringLiteral("textures/armor/foo_n.dds"));
    QVERIFY(normal != nullptr);
    QCOMPARE(normal->format, TextureConversionRule::Format::BC5);
}

void TestTextureConversionRules::testInvalidJson()
{
    QVector<TextureConversionRule> rules;
    QVERIFY(!TextureConversionRules::parse(QStringLiteral("not json"), rules));
    QCOMPARE(rules.size(), 0);

    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    tmpFile.write("not json");
    const QString path = tmpFile.fileName();
    tmpFile.close();
    QVERIFY(!TextureConversionRules::loadFile(path, rules));
}

void TestTextureConversionRules::testRuleAwareConversion()
{
    // Write a normal-map rule file and a source PNG that matches it.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString srcPath = dir.filePath(QStringLiteral("foo_n.png"));
    QImage src(8, 8, QImage::Format_ARGB32);
    src.fill(QColor(120, 140, 160, 255));
    QVERIFY(src.save(srcPath));

    const QString rulesPath = dir.filePath(QStringLiteral("Textures_Settings.json"));
    QFile rulesFile(rulesPath);
    QVERIFY(rulesFile.open(QIODevice::WriteOnly));
    rulesFile.write(R"([
        { "path": "*_n.dds", "format": "BC5", "mipmaps": true },
        { "path": "*", "format": "BC7", "mipmaps": true }
    ])");
    rulesFile.close();

    const QString outDir = dir.filePath(QStringLiteral("out"));
    const AssetConverter::ConversionResult result =
        AssetConverter::convertTexturesByRules({ srcPath }, outDir, rulesPath);

    QVERIFY(result.success);
    QCOMPARE(result.filesConverted, 1);
    QVERIFY(QFile::exists(outDir + QStringLiteral("/foo_n.dds")));

    // Default rules path (empty) should also convert using builtin rules.
    const QString outDir2 = dir.filePath(QStringLiteral("out2"));
    const AssetConverter::ConversionResult result2 =
        AssetConverter::convertTexturesByRules({ srcPath }, outDir2);
    QVERIFY(result2.success);
    QCOMPARE(result2.filesConverted, 1);
}

QTEST_MAIN(TestTextureConversionRules)
#include "test_textureconversionrules.moc"

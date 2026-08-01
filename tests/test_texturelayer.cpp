#include <QTest>

#include "../../src/view/window/landscapeeditor.hpp"
#include "../../libs/files/log/logger.hpp"

class TestTextureLayer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDefaultNoSlopeInfluence();
    void testSlopeFade();
    void testSlopeInverted();
    void testDefaults();
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

QTEST_MAIN(TestTextureLayer)
#include "test_texturelayer.moc"

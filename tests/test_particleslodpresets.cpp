#include <QTest>
#include <QTemporaryFile>

#include "../../src/model/tools/particleslodpresets.hpp"
#include "../../libs/files/log/logger.hpp"

class TestParticleLodPresets : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testParseArray();
    void testParseObject();
    void testParseInvalid();
    void testLoadFile();
    void testMissingCategory();
};

void TestParticleLodPresets::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_lodpresets_log.txt"));
}

void TestParticleLodPresets::testParseArray()
{
    const QByteArray json = R"([
        { "category": "Fire", "near": 100, "middle": 50, "far": 10 },
        { "category": "Smoke", "near": 200, "middle": 80, "far": 0 }
    ])";
    const auto presets = ParticleLodPresets::parseJson(json);
    QCOMPARE(presets.size(), 2);
    QCOMPARE(presets[0].name, QStringLiteral("Fire"));
    QCOMPARE(presets[0].nearBudget, 100);
    QCOMPARE(presets[0].middleBudget, 50);
    QCOMPARE(presets[0].farBudget, 10);
    QCOMPARE(presets[1].name, QStringLiteral("Smoke"));
    QCOMPARE(presets[1].middleBudget, 80);
}

void TestParticleLodPresets::testParseObject()
{
    const QByteArray json = R"({
        "presets": [
            { "category": "Water", "near": 300, "middle": 150, "far": 20 }
        ]
    })";
    const auto presets = ParticleLodPresets::parseJson(json);
    QCOMPARE(presets.size(), 1);
    QCOMPARE(presets[0].name, QStringLiteral("Water"));
    QCOMPARE(presets[0].farBudget, 20);
}

void TestParticleLodPresets::testParseInvalid()
{
    QVERIFY(ParticleLodPresets::parseJson("not json").isEmpty());
}

void TestParticleLodPresets::testLoadFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(R"({"presets": [ { "category": "Dust", "near": 10 } ]})");
    file.close();

    QVector<ParticleLodPresets::Category> out;
    QVERIFY(ParticleLodPresets::loadFile(file.fileName(), out));
    QCOMPARE(out.size(), 1);
    QCOMPARE(out[0].name, QStringLiteral("Dust"));

    QVector<ParticleLodPresets::Category> none;
    QVERIFY(!ParticleLodPresets::loadFile(QStringLiteral("Z:/missing.json"), none));
}

void TestParticleLodPresets::testMissingCategory()
{
    // Entries without a category name are skipped.
    const QByteArray json = R"([
        { "near": 10 },
        { "category": "Valid", "near": 5 }
    ])";
    const auto presets = ParticleLodPresets::parseJson(json);
    QCOMPARE(presets.size(), 1);
    QCOMPARE(presets[0].name, QStringLiteral("Valid"));
}

QTEST_MAIN(TestParticleLodPresets)
#include "test_particleslodpresets.moc"

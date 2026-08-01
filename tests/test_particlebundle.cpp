#include <QTest>
#include <QTemporaryFile>

#include "../../src/model/tools/particlebundle.hpp"
#include "../../libs/files/log/logger.hpp"

class TestParticleBundle : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testParseNodes();
    void testParseSingleNode();
    void testParseEmittersKey();
    void testParseInvalid();
    void testLoadFile();
};

void TestParticleBundle::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_particlebundle_log.txt"));
}

void TestParticleBundle::testParseNodes()
{
    const QByteArray json = R"([
        {
            "name": "FireBundle",
            "nodes": [
                { "name": "Flame", "age": 2.0, "velocity": 10, "gravity": -1.5, "ribbon": true },
                { "name": "Sparks", "age": 1.0, "uvScroll": true }
            ]
        }
    ])";
    const ParticleBundle bundle = ParticleBundle::parse(json);
    QCOMPARE(bundle.name, QStringLiteral("FireBundle"));
    QCOMPARE(bundle.nodes.size(), 2);
    QCOMPARE(bundle.nodes[0].name, QStringLiteral("Flame"));
    QCOMPARE(bundle.nodes[0].bundle, QStringLiteral("FireBundle"));
    QVERIFY(qFuzzyCompare(bundle.nodes[0].age, 2.0f));
    QVERIFY(qFuzzyCompare(bundle.nodes[0].velocity, 10.0f));
    QVERIFY(bundle.nodes[0].ribbon);
    QVERIFY(bundle.nodes[1].uvScroll);
    QVERIFY(!bundle.nodes[1].ribbon);
}

void TestParticleBundle::testParseSingleNode()
{
    const QByteArray json = R"({
        "name": "Sparkle",
        "age": 3.0,
        "alphaByCurve": 0.8,
        "texture": "fx/sparkle.dds"
    })";
    const ParticleBundle bundle = ParticleBundle::parse(json);
    QCOMPARE(bundle.nodes.size(), 1);
    QCOMPARE(bundle.nodes[0].name, QStringLiteral("Sparkle"));
    QVERIFY(qFuzzyCompare(bundle.nodes[0].alphaByCurve, 0.8f));
    QCOMPARE(bundle.nodes[0].texture, QStringLiteral("fx/sparkle.dds"));
}

void TestParticleBundle::testParseEmittersKey()
{
    const QByteArray json = R"({
        "name": "RainBundle",
        "emitters": [ { "name": "Rain", "drag": 0.3 } ]
    })";
    const ParticleBundle bundle = ParticleBundle::parse(json);
    QCOMPARE(bundle.nodes.size(), 1);
    QCOMPARE(bundle.nodes[0].name, QStringLiteral("Rain"));
    QVERIFY(qFuzzyCompare(bundle.nodes[0].drag, 0.3f));
}

void TestParticleBundle::testParseInvalid()
{
    const ParticleBundle bundle = ParticleBundle::parse("garbage");
    QVERIFY(bundle.nodes.isEmpty());
}

void TestParticleBundle::testLoadFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(R"({ "name": "Dust", "nodes": [ { "name": "Puff", "rotationSpeed": 5.0 } ] })");
    file.close();

    ParticleBundle bundle;
    QVERIFY(ParticleBundle::loadFile(file.fileName(), bundle));
    QCOMPARE(bundle.nodes.size(), 1);
    QCOMPARE(bundle.nodes[0].name, QStringLiteral("Puff"));

    ParticleBundle missing;
    QVERIFY(!ParticleBundle::loadFile(QStringLiteral("Z:/missing.pofx"), missing));
}

QTEST_MAIN(TestParticleBundle)
#include "test_particlebundle.moc"

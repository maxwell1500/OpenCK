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
    void testParseAttractorsTurbulenceFlipBook();
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

void TestParticleBundle::testParseAttractorsTurbulenceFlipBook()
{
    const QByteArray json = R"({
        "name": "MagicalBundle",
        "nodes": [
            {
                "name": "Glow",
                "attractors": [
                    { "name": "Center", "x": 1.0, "y": 2.0, "z": 3.0, "strength": 2.5, "radius": 4.0 },
                    { "name": "Side", "x": -1.0, "strength": 0.5 }
                ],
                "turbulence": { "strength": 3.0, "frequency": 2.0 },
                "flipBook": { "columns": 4, "rows": 2, "frameRate": 24.0, "loop": true }
            }
        ]
    })";
    const ParticleBundle bundle = ParticleBundle::parse(json);
    QCOMPARE(bundle.nodes.size(), 1);
    const auto& node = bundle.nodes[0];

    QCOMPARE(node.attractors.size(), 2);
    QCOMPARE(node.attractors[0].name, QStringLiteral("Center"));
    QVERIFY(qFuzzyCompare(node.attractors[0].x, 1.0f));
    QVERIFY(qFuzzyCompare(node.attractors[0].y, 2.0f));
    QVERIFY(qFuzzyCompare(node.attractors[0].z, 3.0f));
    QVERIFY(qFuzzyCompare(node.attractors[0].strength, 2.5f));
    QVERIFY(qFuzzyCompare(node.attractors[0].radius, 4.0f));
    QVERIFY(qFuzzyCompare(node.attractors[1].x, -1.0f));
    QVERIFY(qFuzzyCompare(node.attractors[1].strength, 0.5f));

    QVERIFY(qFuzzyCompare(node.turbulence.strength, 3.0f));
    QVERIFY(qFuzzyCompare(node.turbulence.frequency, 2.0f));

    QCOMPARE(node.flipBook.columns, 4);
    QCOMPARE(node.flipBook.rows, 2);
    QVERIFY(qFuzzyCompare(node.flipBook.frameRate, 24.0f));
    QVERIFY(node.flipBook.loop);

    // Defaults when the keys are absent.
    const QByteArray plain = R"({ "name": "Plain", "nodes": [ { "name": "P" } ] })";
    const ParticleBundle plainBundle = ParticleBundle::parse(plain);
    QVERIFY(plainBundle.nodes[0].attractors.isEmpty());
    QVERIFY(qFuzzyCompare(plainBundle.nodes[0].turbulence.strength, 0.0f));
    QCOMPARE(plainBundle.nodes[0].flipBook.columns, 1);
    QCOMPARE(plainBundle.nodes[0].flipBook.rows, 1);
    QVERIFY(!plainBundle.nodes[0].flipBook.loop);
}

QTEST_MAIN(TestParticleBundle)
#include "test_particlebundle.moc"

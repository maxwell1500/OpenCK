#include <QTest>
#include <QTemporaryFile>

#include "../../src/model/tools/particleprojectilebindings.hpp"
#include "../../libs/files/log/logger.hpp"

class TestParticleProjectileBindings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testParseArray();
    void testParseObject();
    void testParseInvalid();
    void testLoadFile();
    void testKnownVariables();
};

void TestParticleProjectileBindings::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_ppb_log.txt"));
}

void TestParticleProjectileBindings::testParseArray()
{
    const QByteArray json = R"([
        { "particleVariable": "BeamLength", "projectileAttribute": "Distance" },
        { "particleVariable": "HasHit", "projectileAttribute": "Impact" },
        "not an object"
    ])";
    const auto bindings = ParticleProjectileBindings::parseJson(json);
    QCOMPARE(bindings.size(), 2);
    QCOMPARE(bindings[0].particleVariable, QStringLiteral("BeamLength"));
    QCOMPARE(bindings[0].projectileAttribute, QStringLiteral("Distance"));
    QCOMPARE(bindings[1].particleVariable, QStringLiteral("HasHit"));
}

void TestParticleProjectileBindings::testParseObject()
{
    const QByteArray json = R"({
        "bindings": [
            { "particleVariable": "BeamLifeTime", "projectileAttribute": "Lifetime" }
        ]
    })";
    const auto bindings = ParticleProjectileBindings::parseJson(json);
    QCOMPARE(bindings.size(), 1);
    QCOMPARE(bindings[0].particleVariable, QStringLiteral("BeamLifeTime"));
    QCOMPARE(bindings[0].projectileAttribute, QStringLiteral("Lifetime"));
}

void TestParticleProjectileBindings::testParseInvalid()
{
    QVERIFY(ParticleProjectileBindings::parseJson("this is not json").isEmpty());
}

void TestParticleProjectileBindings::testLoadFile()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(R"({"bindings": [ { "particleVariable": "Velocity", "projectileAttribute": "Speed" } ]})");
    file.close();

    QVector<ParticleProjectileBindings::Binding> out;
    QVERIFY(ParticleProjectileBindings::loadFile(file.fileName(), out));
    QCOMPARE(out.size(), 1);
    QCOMPARE(out[0].particleVariable, QStringLiteral("Velocity"));

    QVector<ParticleProjectileBindings::Binding> none;
    QVERIFY(!ParticleProjectileBindings::loadFile(QStringLiteral("Z:/missing.pofx"), none));
}

void TestParticleProjectileBindings::testKnownVariables()
{
    const QStringList vars = ParticleProjectileBindings::knownProjectileVariables();
    QVERIFY(vars.contains(QStringLiteral("BeamLength")));
    QVERIFY(vars.contains(QStringLiteral("BeamLifeTime")));
    QVERIFY(vars.contains(QStringLiteral("HasHit")));
    QVERIFY(vars.contains(QStringLiteral("Velocity")));
}

QTEST_MAIN(TestParticleProjectileBindings)
#include "test_particleprojectilebindings.moc"

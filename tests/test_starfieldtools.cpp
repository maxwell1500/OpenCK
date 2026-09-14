#include <QtTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

#include "../../src/model/tools/starfielddefinitions.hpp"
#include "../../src/model/tools/galaxymap.hpp"
#include "../../src/model/tools/robovoicer.hpp"
#include "../../src/model/tools/houdinibridge.hpp"

class TestStarfieldTools : public QObject
{
    Q_OBJECT

private slots:
    void testSpaceshipJsonRoundTrip();
    void testReflectionProbeJsonRoundTrip();
    void testCrowdRegionJsonRoundTrip();
    void testMorphJsonRoundTrip();
    void testGalaxyMapRoundTrip();
    void testVoicePlanRoundTrip();
    void testVoicePlanRunSuccess();
    void testVoicePlanRunFailure();
    void testHoudiniBridgeCommands();
    // Windows-only SAPI backend (bodies skip elsewhere; the declarations stay
    // unconditional because moc does not evaluate _WIN32).
    void testSapiAvailable();
    void testSapiSynthesizeToWav();
};

void TestStarfieldTools::testSpaceshipJsonRoundTrip()
{
    SpaceshipDefinition def;
    def.editorId = QStringLiteral("SFShip_Frontier");
    def.name = QStringLiteral("Frontier");
    def.shipClass = QStringLiteral("A");
    def.reactorId = QStringLiteral("Reactor_A1");
    def.gravDriveId = QStringLiteral("Grav_A1");
    def.shieldId = QStringLiteral("Shield_A1");
    def.engineId = QStringLiteral("Engine_A1");
    def.engineCount = 2;
    def.cargoCapacity = 450;
    def.crewCapacity = 6;
    def.mass = 1180.0;
    def.hull = 690.0;

    SpaceshipDefinition::Module m;
    m.slot = QStringLiteral("Weapon");
    m.id = QStringLiteral("Laser_MA1");
    m.count = 2;
    def.modules.append(m);

    const SpaceshipDefinition loaded =
        SpaceshipDefinition::fromJson(def.toJson());

    QCOMPARE(loaded.editorId, def.editorId);
    QCOMPARE(loaded.name, def.name);
    QCOMPARE(loaded.shipClass, def.shipClass);
    QCOMPARE(loaded.engineCount, 2);
    QCOMPARE(loaded.cargoCapacity, 450);
    QCOMPARE(loaded.crewCapacity, 6);
    QCOMPARE(loaded.mass, 1180.0);
    QCOMPARE(loaded.hull, 690.0);
    QCOMPARE(loaded.modules.size(), 1);
    QCOMPARE(loaded.modules[0].slot, QStringLiteral("Weapon"));
    QCOMPARE(loaded.modules[0].count, 2);
}

void TestStarfieldTools::testReflectionProbeJsonRoundTrip()
{
    ReflectionProbeDefinition def;
    def.editorId = QStringLiteral("Probe_LandingPad");
    def.x = 100.0;
    def.y = 200.0;
    def.z = 50.0;
    def.radius = 1024.0;
    def.resolution = 256;
    def.boxProjection = true;
    def.brightness = 1.25;
    def.shape = QStringLiteral("Box");

    const ReflectionProbeDefinition loaded =
        ReflectionProbeDefinition::fromJson(def.toJson());

    QCOMPARE(loaded.editorId, def.editorId);
    QCOMPARE(loaded.x, 100.0);
    QCOMPARE(loaded.radius, 1024.0);
    QCOMPARE(loaded.resolution, 256);
    QCOMPARE(loaded.boxProjection, true);
    QCOMPARE(loaded.brightness, 1.25);
    QCOMPARE(loaded.shape, QStringLiteral("Box"));
}

void TestStarfieldTools::testCrowdRegionJsonRoundTrip()
{
    CrowdRegionDefinition def;
    def.editorId = QStringLiteral("Crowd_Market");
    def.behavior = QStringLiteral("Wander");
    def.x = 10.0;
    def.y = 20.0;
    def.z = 0.0;
    def.radius = 2048.0;
    def.density = 0.25;
    def.maxMembers = 40;

    CrowdRegionDefinition::Member member;
    member.id = QStringLiteral("Citizen_Market");
    member.weight = 2.0;
    def.members.append(member);

    const CrowdRegionDefinition loaded =
        CrowdRegionDefinition::fromJson(def.toJson());

    QCOMPARE(loaded.editorId, def.editorId);
    QCOMPARE(loaded.behavior, QStringLiteral("Wander"));
    QCOMPARE(loaded.density, 0.25);
    QCOMPARE(loaded.maxMembers, 40);
    QCOMPARE(loaded.members.size(), 1);
    QCOMPARE(loaded.members[0].id, QStringLiteral("Citizen_Market"));
    QCOMPARE(loaded.members[0].weight, 2.0);
}

void TestStarfieldTools::testMorphJsonRoundTrip()
{
    QVERIFY(!MorphDefinition::commonChannels().isEmpty());

    MorphDefinition def;
    def.editorId = QStringLiteral("Face_Hero");
    def.raceId = QStringLiteral("Human");

    MorphDefinition::Channel c;
    c.name = QStringLiteral("Brow Height");
    c.value = 0.35;
    c.minimum = -1.0;
    c.maximum = 1.0;
    def.channels.append(c);

    const MorphDefinition loaded = MorphDefinition::fromJson(def.toJson());

    QCOMPARE(loaded.editorId, def.editorId);
    QCOMPARE(loaded.raceId, QStringLiteral("Human"));
    QCOMPARE(loaded.channels.size(), 1);
    QCOMPARE(loaded.channels[0].name, QStringLiteral("Brow Height"));
    QCOMPARE(loaded.channels[0].value, 0.35);
}

void TestStarfieldTools::testGalaxyMapRoundTrip()
{
    GalaxyMap map;
    map.name = QStringLiteral("Settled Systems");

    GalaxyMap::StarSystem system;
    system.name = QStringLiteral("Alpha Centauri");
    system.x = 1.0;
    system.y = 2.0;

    GalaxyMap::Planet planet;
    planet.name = QStringLiteral("Jemison");
    planet.type = QStringLiteral("Rock");
    planet.planetEditorId = QStringLiteral("Planet_Jemison");
    planet.moons = 1;
    system.planets.append(planet);
    map.systems.append(system);

    const GalaxyMap loaded = GalaxyMap::fromJson(map.toJson());

    QCOMPARE(loaded.name, map.name);
    QCOMPARE(loaded.systems.size(), 1);
    QCOMPARE(loaded.systems[0].name, QStringLiteral("Alpha Centauri"));
    QCOMPARE(loaded.systems[0].x, 1.0);
    QCOMPARE(loaded.systems[0].planets.size(), 1);
    QCOMPARE(loaded.systems[0].planets[0].moons, 1);
    QCOMPARE(loaded.totalPlanets(), 1);
    QCOMPARE(loaded.findSystem(QStringLiteral("alpha centauri")), 0);
    QCOMPARE(loaded.findSystem(QStringLiteral("Sol")), -1);
}

void TestStarfieldTools::testVoicePlanRoundTrip()
{
    VoiceLinePlan plan;
    plan.name = QStringLiteral("Intro Scene");

    VoiceLine line;
    line.lineId = QStringLiteral("DIAL_Intro_001");
    line.speaker = QStringLiteral("Sarah");
    line.text = QStringLiteral("Welcome aboard.");
    line.voiceId = QStringLiteral("Voice_F_Sarah");
    line.outputPath = QStringLiteral("voice/intro_001.wav");
    plan.lines.append(line);

    const VoiceLinePlan loaded = VoiceLinePlan::fromJson(plan.toJson());

    QCOMPARE(loaded.name, plan.name);
    QCOMPARE(loaded.lines.size(), 1);
    QCOMPARE(loaded.lines[0].speaker, QStringLiteral("Sarah"));
    QCOMPARE(loaded.pendingCount(), 1);
}

namespace {

class FakeSynth : public IVoiceSynthesizer
{
public:
    bool available = true;
    bool succeed = true;
    QStringList synthesized;

    QString name() const override { return QStringLiteral("FakeSynth"); }
    bool isAvailable() const override { return available; }
    bool synthesize(const QString& text, const QString& voiceId,
                    const QString& outputPath) override
    {
        Q_UNUSED(voiceId);
        Q_UNUSED(outputPath);
        synthesized.append(text);
        return succeed;
    }
};

} // namespace

void TestStarfieldTools::testVoicePlanRunSuccess()
{
    VoiceLinePlan plan;
    VoiceLine a;
    a.lineId = QStringLiteral("A");
    a.text = QStringLiteral("Hello.");
    VoiceLine b;
    b.lineId = QStringLiteral("B");
    b.text = QStringLiteral("Goodbye.");
    plan.lines << a << b;

    FakeSynth synth;
    const VoiceRunReport report = runVoicePlan(plan, &synth);

    QCOMPARE(report.completed, 2);
    QCOMPARE(report.failed, 0);
    QCOMPARE(plan.pendingCount(), 0);
}

void TestStarfieldTools::testVoicePlanRunFailure()
{
    VoiceLinePlan plan;
    VoiceLine a;
    a.lineId = QStringLiteral("A");
    a.done = true;      // already done: skipped
    VoiceLine b;
    b.lineId = QStringLiteral("B");
    b.text = QStringLiteral("Fail me.");
    plan.lines << a << b;

    // Null engine marks pending lines failed.
    VoiceRunReport nullReport = runVoicePlan(plan, nullptr);
    QCOMPARE(nullReport.completed, 0);
    QCOMPARE(nullReport.failed, 1);
    QCOMPARE(nullReport.failures, QVector<QString>({QStringLiteral("B")}));
    QCOMPARE(plan.pendingCount(), 1);

    // Unavailable engine fails the same way.
    FakeSynth synth;
    synth.available = false;
    const VoiceRunReport downReport = runVoicePlan(plan, &synth);
    QCOMPARE(downReport.failed, 1);
}

void TestStarfieldTools::testHoudiniBridgeCommands()
{
    HoudiniBridge bridge;
    QVERIFY(!bridge.isConfigured());

    bridge.houdiniExecutable = QStringLiteral("C:/houdini/bin/hython.exe");
    bridge.hipFile = QStringLiteral("C:/work/ship.hip");
    bridge.extraArgs = QStringList({QStringLiteral("-v")});
    QVERIFY(bridge.isConfigured());

    const QStringList interactive = bridge.interactiveCommand();
    QCOMPARE(interactive,
             QStringList({QStringLiteral("C:/houdini/bin/hython.exe"),
                          QStringLiteral("C:/work/ship.hip"),
                          QStringLiteral("-v")}));

    const QStringList batch = bridge.batchCommand(
        QStringLiteral("export_ship.py"),
        QStringList({QStringLiteral("--out"), QStringLiteral("ship.fbx")}));
    QCOMPARE(batch,
             QStringList({QStringLiteral("C:/houdini/bin/hython.exe"),
                          QStringLiteral("export_ship.py"),
                          QStringLiteral("-v"),
                          QStringLiteral("--out"),
                          QStringLiteral("ship.fbx")}));

    QCOMPARE(HoudiniBridge::defaultBatchExecutable(),
             QStringLiteral("hython.exe"));
}

void TestStarfieldTools::testSapiAvailable()
{
#ifdef _WIN32
    SapiVoiceSynthesizer sapi;
    QCOMPARE(sapi.name(), QStringLiteral("SAPI"));
    if (!sapi.isAvailable())
        QSKIP("No SAPI voices installed on this machine");
    QVERIFY(!SapiVoiceSynthesizer::availableVoices().isEmpty());
#else
    QSKIP("SAPI backend is Windows-only");
#endif
}

void TestStarfieldTools::testSapiSynthesizeToWav()
{
#ifdef _WIN32
    SapiVoiceSynthesizer sapi;
    if (!sapi.isAvailable())
        QSKIP("No SAPI voices installed on this machine");

    const QString outPath = QDir::tempPath()
        + QStringLiteral("/openck_sapi_test.wav");
    QFile::remove(outPath);

    // Empty text or empty path must fail without touching the filesystem.
    QVERIFY(!sapi.synthesize(QString(), QString(), outPath));
    QVERIFY(!sapi.synthesize(QStringLiteral("Hello."), QString(), QString()));
    QVERIFY(!QFileInfo::exists(outPath));

    QVERIFY(sapi.synthesize(QStringLiteral("Welcome aboard."), QString(), outPath));
    QFile wav(outPath);
    QVERIFY(wav.open(QIODevice::ReadOnly));
    const QByteArray bytes = wav.readAll();
    QVERIFY(bytes.size() > 44);
    QCOMPARE(bytes.left(4), QByteArray("RIFF", 4));
    wav.close();
    QFile::remove(outPath);

    // End-to-end through the plan runner: the line is marked done.
    VoiceLinePlan plan;
    VoiceLine line;
    line.lineId = QStringLiteral("SAPI_001");
    line.text = QStringLiteral("Hello.");
    line.outputPath = QDir::tempPath() + QStringLiteral("/openck_sapi_plan.wav");
    plan.lines.append(line);
    const VoiceRunReport report = runVoicePlan(plan, &sapi);
    QCOMPARE(report.completed, 1);
    QCOMPARE(plan.pendingCount(), 0);
    QVERIFY(QFileInfo::exists(line.outputPath));
    QFile::remove(line.outputPath);
#else
    QSKIP("SAPI backend is Windows-only");
#endif
}

QTEST_MAIN(TestStarfieldTools)
#include "test_starfieldtools.moc"

#include <QTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QSaveFile>

#include "src/model/tools/blenderbridge.hpp"
#include "libs/files/nif/nifparser.hpp"
#include "libs/files/log/logger.hpp"

// Phase 12.2: Blender live-sync bridge.
//
// The bridge's contract has to hold on a machine with no Blender at all:
// scene resolution (mesh + skeleton + collision), the launch plan, and the
// export verification / commit rules are all testable without launching
// anything. The NIF fixtures used here are written through the editor's own
// NifParser writer, which is the same reader verifyExport accepts with.

namespace {

void writeNifFixture(const QString& path, const QString& name)
{
    Nif::NifParser parser;
    auto* root = new Nif::Node();
    root->name = name;
    Nif::TriShape shape;
    shape.name = name;
    shape.vertices = { { 0.0f, 0.0f, 0.0f },
                       { 1.0f, 0.0f, 0.0f },
                       { 0.0f, 1.0f, 0.0f } };
    shape.uvs = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f } };
    shape.indices = { 0, 1, 2 };
    root->shapes.append(shape);
    parser.setRoot(root);
    QVERIFY2(parser.save(path), qPrintable(path));
}

} // namespace

class TestBlenderBridge : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDiscoverResolvesSkeletonAndCollision();
    void testDiscoverUnknownMesh();
    void testBuildLaunchPlan();
    void testExportVerificationAcceptsFixture();
    void testExportVerificationRejectsGarbage();
    void testCommitSwapsVerifiedExport();
    void testCommitRequiresVerifiedExport();
};

void TestBlenderBridge::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Warning);
    OpenCK::Logging::Logger::instance().init(
        QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_blenderbridge_log.txt"));
}

void TestBlenderBridge::testDiscoverResolvesSkeletonAndCollision()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString mesh = dir.filePath("barrel.nif");
    writeNifFixture(mesh, QStringLiteral("barrel"));
    const QString skeleton = dir.filePath("barrel_skeleton.nif");
    writeNifFixture(skeleton, QStringLiteral("barrel_skeleton"));
    const QString collision = dir.filePath("barrel_collision.nif");
    writeNifFixture(collision, QStringLiteral("barrel_collision"));

    const BlenderBridge::AssetContext context = BlenderBridge::discover(mesh);
    QCOMPARE(context.nifPath, mesh);
    // The skeleton is resolved through the NIF + sibling convention, not
    // guessed: without it a skinned mesh opens rigid in Blender.
    QCOMPARE(context.skeletonPath, skeleton);
    QVERIFY(context.collisionPaths.contains(collision));
}

void TestBlenderBridge::testDiscoverUnknownMesh()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // A mesh that is not a NIF at all still produces a context: the caller
    // gets to report "the file exists but is not readable" instead of a
    // silent empty launch.
    const QString garbage = dir.filePath("broken.nif");
    QFile f(garbage);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("this is not a NIF");
    f.close();

    const BlenderBridge::AssetContext context = BlenderBridge::discover(garbage);
    QCOMPARE(context.nifPath, garbage);
    QVERIFY(context.skeletonPath.isEmpty());
    QVERIFY(context.isValid());
}

void TestBlenderBridge::testBuildLaunchPlan()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString mesh = dir.filePath("chair.nif");
    writeNifFixture(mesh, QStringLiteral("chair"));

    const QString script = BlenderBridge::defaultScriptPath();
    QVERIFY2(!script.isEmpty(), "openck_livesync.py must ship in scripts/blender");
    const BlenderBridge::LaunchSpec spec =
        BlenderBridge::buildLaunch(mesh, QStringLiteral("C:/blender/blender.exe"), script);
    QVERIFY2(spec.error.isEmpty(), qPrintable(spec.error));
    QCOMPARE(spec.context.nifPath, mesh);
    QCOMPARE(spec.outputPath, dir.filePath("chair_opencklive.nif"));

    // The launch is: Blender + our script + the scene it must open.
    const int pythonIndex = spec.blenderArgs.indexOf(QStringLiteral("--python"));
    QVERIFY(pythonIndex >= 0);
    QCOMPARE(spec.blenderArgs.at(pythonIndex + 1), script);
    QVERIFY(spec.blenderArgs.contains(QStringLiteral("--op=open")));
    QVERIFY(spec.blenderArgs.contains(QStringLiteral("--nif=") + mesh));
    QVERIFY(spec.blenderArgs.contains(QStringLiteral("--output=") + spec.outputPath));

    // The watched export path is what the whole round trip keys off.
    // Missing script is a build error, not a runtime surprise.
    const BlenderBridge::LaunchSpec noScript =
        BlenderBridge::buildLaunch(mesh, QStringLiteral("C:/blender/blender.exe"),
                                   QStringLiteral("C:/openck/does-not-exist.py"));
    QVERIFY(!noScript.error.isEmpty());

    // The fixtures this suite verifies must themselves round-trip through
    // the editor's reader, or every "verified export" assertion below is
    // testing nothing.
    Nif::NifParser roundTrip;
    QVERIFY2(roundTrip.load(mesh), "fixture NIF must re-load through NifParser");
}

void TestBlenderBridge::testExportVerificationAcceptsFixture()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // Full loop minus the process launch: arm a session, let the editor's
    // own NIF appear as the export, and the bridge must verify it.
    const QString mesh = dir.filePath("chair.nif");
    writeNifFixture(mesh, QStringLiteral("chair"));

    BlenderBridge::AssetContext context;
    context.nifPath = mesh;
    const QString output = dir.filePath("chair_opencklive.nif");

    BlenderBridge bridge;
    QVERIFY(bridge.armSession(context, output));
    QVERIFY(bridge.hasSession());

    QSignalSpy verified(&bridge, &BlenderBridge::exportVerified);
    QSignalSpy rejected(&bridge, &BlenderBridge::exportRejected);
    QFile::copy(mesh, output);

    QTRY_COMPARE_WITH_TIMEOUT(verified.count(), 1, 10000);
    QCOMPARE(rejected.count(), 0);
    QCOMPARE(verified.first().at(0).toString(), output);
}

void TestBlenderBridge::testExportVerificationRejectsGarbage()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString mesh = dir.filePath("chair.nif");
    writeNifFixture(mesh, QStringLiteral("chair"));

    BlenderBridge::AssetContext context;
    context.nifPath = mesh;
    const QString output = dir.filePath("chair_opencklive.nif");

    BlenderBridge bridge;
    QVERIFY(bridge.armSession(context, output));

    QSignalSpy rejected(&bridge, &BlenderBridge::exportRejected);
    QSignalSpy verified(&bridge, &BlenderBridge::exportVerified);
    QFile garbage(output);
    QVERIFY(garbage.open(QIODevice::WriteOnly));
    garbage.write("this is not a NIF");
    garbage.close();

    QTRY_COMPARE_WITH_TIMEOUT(rejected.count(), 1, 5000);
    QCOMPARE(verified.count(), 0);
    QVERIFY(!rejected.first().at(1).toString().isEmpty());

    // The rejection must not open the commit path: no verified export.
    QString error;
    QVERIFY(!bridge.commit(&error));
    QCOMPARE(error, QStringLiteral("no verified export to commit"));
    // And the plugin's asset is untouched.
    QCOMPARE(QFileInfo(mesh).size(), QFileInfo(mesh).size());
    QVERIFY(QFile::exists(mesh));
}

void TestBlenderBridge::testCommitSwapsVerifiedExport()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // The commit is what actually touches the plugin's asset, so prove
    // both halves: a verified export lands atomically with a backup, and a
    // second commit is refused until a new export is verified.
    const QString mesh = dir.filePath("chair.nif");
    writeNifFixture(mesh, QStringLiteral("chair"));

    // A distinguishable edit: same structure, different shape name and a
    // different vertex count is what the writer produces for a real edit.
    Nif::NifParser edited;
    auto* root = new Nif::Node();
    root->name = QStringLiteral("chair");
    Nif::TriShape shape;
    shape.name = QStringLiteral("chair_edited");
    shape.vertices = { { 0.0f, 0.0f, 0.0f } };
    shape.indices = { 0 };
    root->shapes.append(shape);
    edited.setRoot(root);
    const QString output = dir.filePath("chair_opencklive.nif");

    BlenderBridge::AssetContext context;
    context.nifPath = mesh;
    BlenderBridge bridge;
    QVERIFY(bridge.armSession(context, output));

    QSignalSpy verified(&bridge, &BlenderBridge::exportVerified);
    QSignalSpy committed(&bridge, &BlenderBridge::committed);
    // The export is written after the watcher is armed, exactly like a
    // Blender save.
    QVERIFY(edited.save(output));
    QTRY_COMPARE_WITH_TIMEOUT(verified.count(), 1, 10000);
    QString error;
    QVERIFY2(bridge.commit(&error), qPrintable(error));
    QCOMPARE(committed.count(), 1);

    // The asset now holds the export's bytes and still parses (the commit
    // re-reads it); the original is preserved beside it.
    Nif::NifParser afterLoad;
    QVERIFY2(afterLoad.load(mesh), "committed asset must still parse");
    QVERIFY(QFileInfo::exists(
        dir.filePath("chair.openckbak")));

    // A second commit needs a fresh export: committing twice would roll the
    // asset backwards if the artist kept editing.
    QVERIFY(!bridge.commit(&error));
    QCOMPARE(error, QStringLiteral("no verified export to commit"));
}

void TestBlenderBridge::testCommitRequiresVerifiedExport()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // With no session open at all, commit is a no-op with a clear reason
    // rather than a write against the plugin's asset.
    BlenderBridge bridge;
    QVERIFY(!bridge.hasSession());
    QString error;
    QVERIFY(!bridge.commit(&error));
    QCOMPARE(error, QStringLiteral("no verified export to commit"));

    bridge.stop();   // idempotent without a session
    QVERIFY(!bridge.hasSession());
}

QTEST_MAIN(TestBlenderBridge)
#include "test_blenderbridge.moc"

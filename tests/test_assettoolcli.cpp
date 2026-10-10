#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QSettings>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../../src/model/tools/assettoolcli.hpp"
#include "../../src/model/tools/materialruletemplate.hpp"
#include "../../libs/files/log/logger.hpp"

#ifndef Q_OS_WIN
static QString cmdExe() { return QString(); }
#else
static QString cmdExe() { return QStringLiteral("C:/Windows/System32/cmd.exe"); }
#endif

class TestAssetToolCli : public QObject
{
    Q_OBJECT

private:
    void makeBridge()
    {
        delete mBridge;
        delete mSettings;
        mSettings = new QSettings(mDir.filePath(QStringLiteral("settings.ini")), QSettings::IniFormat);
        mBridge = new AssetToolCliBridge(nullptr, mSettings);
    }

    QTemporaryDir mDir;
    QSettings* mSettings = nullptr;
    AssetToolCliBridge* mBridge = nullptr;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testToolIds();
    void testManualOverride();
    void testManualOverrideNonexistent();
    void testEnvVarDetection();
    void testRunToolEcho();
    void testRunToolTimeout();
    void testRunToolNotFound();
    void testConvertTexturesFallback();
    void testPackageMaterials();
    void testSignals();
};

void TestAssetToolCli::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_assettoolcli_log.txt"));
    QVERIFY(mDir.isValid());
}

void TestAssetToolCli::cleanupTestCase()
{
    delete mBridge;
    delete mSettings;
    mBridge = nullptr;
    mSettings = nullptr;
}

void TestAssetToolCli::testToolIds()
{
    makeBridge();
    const QStringList ids = mBridge->toolIds();
    const QStringList expected = {
        QStringLiteral("AssetTool"), QStringLiteral("TextureTool"), QStringLiteral("NifSkse"),
        QStringLiteral("PapyrusCompiler"), QStringLiteral("PapyrusAssembler"),
        QStringLiteral("LipGenerator"), QStringLiteral("FaceFx"),
        QStringLiteral("IconGenerator"), QStringLiteral("Archive2"),
        QStringLiteral("Sqlite3"), QStringLiteral("XEdit"), QStringLiteral("AssetWatcher")
    };
    QCOMPARE(ids.size(), expected.size());
    for (const QString& id : expected)
        QVERIFY2(ids.contains(id), qPrintable(id));
}

void TestAssetToolCli::testManualOverride()
{
    const QString exe = cmdExe();
    if (exe.isEmpty() || !QFile::exists(exe))
        QSKIP("cmd.exe not available");

    makeBridge();
    mBridge->setToolPath(QStringLiteral("AssetTool"), exe);
    QCOMPARE(mBridge->toolPath(QStringLiteral("AssetTool")), exe);
    QCOMPARE(mBridge->detectTool(QStringLiteral("AssetTool")), exe);
    QVERIFY(mBridge->isToolAvailable(QStringLiteral("AssetTool")));

    // The override persists across bridge instances sharing the settings.
    makeBridge();
    QCOMPARE(mBridge->detectTool(QStringLiteral("AssetTool")), exe);
}

void TestAssetToolCli::testManualOverrideNonexistent()
{
    makeBridge();
    mBridge->setToolPath(QStringLiteral("NifSkse"), QStringLiteral("Z:/missing/nifopt.exe"));
    // A manual override pointing at a nonexistent file is ignored.
    QCOMPARE(mBridge->detectTool(QStringLiteral("NifSkse")).isEmpty(), true);
}

void TestAssetToolCli::testEnvVarDetection()
{
    const QString exe = cmdExe();
    if (exe.isEmpty() || !QFile::exists(exe))
        QSKIP("cmd.exe not available");

    qputenv("OPENCK_ASSETTOOL", exe.toLocal8Bit());
    makeBridge();
    QCOMPARE(mBridge->detectTool(QStringLiteral("AssetTool")), exe);
    qunsetenv("OPENCK_ASSETTOOL");
}

void TestAssetToolCli::testRunToolEcho()
{
    const QString exe = cmdExe();
    if (exe.isEmpty() || !QFile::exists(exe))
        QSKIP("cmd.exe not available");

    makeBridge();
    mBridge->setToolPath(QStringLiteral("AssetTool"), exe);
    const AssetToolCliBridge::RunResult r = mBridge->runTool(
        QStringLiteral("AssetTool"), { QStringLiteral("/c"), QStringLiteral("echo"), QStringLiteral("hello") });
    QVERIFY(r.ok);
    QCOMPARE(r.exitCode, 0);
    QVERIFY(r.stdOut.contains(QStringLiteral("hello")));
}

void TestAssetToolCli::testRunToolTimeout()
{
    const QString exe = cmdExe();
    if (exe.isEmpty() || !QFile::exists(exe))
        QSKIP("cmd.exe not available");

    makeBridge();
    mBridge->setToolPath(QStringLiteral("AssetTool"), exe);
    // ping -n 5 takes ~4 s; the 1 s timeout must kill it.
    const AssetToolCliBridge::RunResult r = mBridge->runTool(
        QStringLiteral("AssetTool"),
        { QStringLiteral("/c"), QStringLiteral("ping"), QStringLiteral("-n"),
          QStringLiteral("5"), QStringLiteral("127.0.0.1") },
        1000);
    QVERIFY(!r.ok);
    QVERIFY(r.stdErr.contains(QStringLiteral("Timed out")));
}

void TestAssetToolCli::testRunToolNotFound()
{
    makeBridge();
    mBridge->setToolPath(QStringLiteral("NifSkse"), QStringLiteral("Z:/missing/nifopt.exe"));
    const AssetToolCliBridge::RunResult r = mBridge->runTool(QStringLiteral("NifSkse"), QStringList());
    QVERIFY(!r.ok);
    QVERIFY(r.stdErr.contains(QStringLiteral("Tool not found")));
}

void TestAssetToolCli::testConvertTexturesFallback()
{
    // No TextureTool CLI is installed for the default game roots (verified on
    // the real Starfield install), so the in-process AssetConverter handles it.
    makeBridge();
    mBridge->setToolPath(QStringLiteral("TextureTool"), QStringLiteral("Z:/missing/TextureToolCLI.exe"));

    const QString src = mDir.filePath(QStringLiteral("tex.png"));
    QImage img(16, 16, QImage::Format_RGBA8888);
    img.fill(QColor(10, 20, 30, 255));
    QVERIFY(img.save(src, "PNG"));

    const QString outDir = mDir.filePath(QStringLiteral("out"));
    QFileInfo fi(src);
    const AssetToolCliBridge::PipelineSummary s = mBridge->convertTextures({ src }, outDir, QStringLiteral("dds"));
    QCOMPARE(s.total, 1);
    QCOMPARE(s.success, 1);
    QCOMPARE(s.failed, 0);
    QVERIFY(QFile::exists(QDir(outDir).absoluteFilePath(fi.completeBaseName() + QStringLiteral(".dds"))));
    QVERIFY(s.summaryLine().contains(QStringLiteral("1/1 ok")));

    // A missing input is skipped, not failed.
    const AssetToolCliBridge::PipelineSummary s2 = mBridge->convertTextures(
        { QStringLiteral("Z:/missing.png") }, outDir, QStringLiteral("dds"));
    QCOMPARE(s2.skipped, 1);
    QCOMPARE(s2.success, 0);
    QCOMPARE(s2.failed, 0);
}

static bool writeTexture(const QString& path)
{
    QImage img(16, 16, QImage::Format_RGBA8888);
    img.fill(QColor(200, 100, 50, 255));
    return img.save(path, "PNG");
}

void TestAssetToolCli::testPackageMaterials()
{
    makeBridge();

    const QString rulesDir = mDir.filePath(QStringLiteral("rules"));
    QDir().mkpath(rulesDir);
    QFile rf(QDir(rulesDir).absoluteFilePath(QStringLiteral("TestMat.json")));
    QVERIFY(rf.open(QIODevice::WriteOnly));
    rf.write(R"({ "Category": "ShaderModels", "Name": "TestMat",
                  "TemplateRules": [ { "Class": "null",
                    "Rules": [ { "From": "*", "Op": "Remove" },
                               { "From": "Layer1", "Op": "Add" } ] } ],
                  "Version": "1" })");
    rf.close();

    const QString texRoot = mDir.filePath(QStringLiteral("textures"));
    QDir().mkpath(texRoot);
    const QStringList builtins = MaterialRuleTemplate::builtinLayerSlots();
    for (const QString& slot : builtins)
        QVERIFY(writeTexture(QDir(texRoot).absoluteFilePath(QStringLiteral("tex_%1.png").arg(slot))));

    const QString matDir = mDir.filePath(QStringLiteral("mats"));
    QDir().mkpath(matDir);
    const QString outDir = mDir.filePath(QStringLiteral("out"));

    // Complete material: every required slot has a resolvable texture.
    QJsonObject texObj;
    for (const QString& slot : builtins)
        texObj.insert(slot, QStringLiteral("tex_%1.png").arg(slot));
    QJsonObject mat;
    mat.insert(QStringLiteral("name"), QStringLiteral("Mat1"));
    mat.insert(QStringLiteral("template"), QStringLiteral("TestMat"));
    mat.insert(QStringLiteral("textures"), texObj);
    const QString mat1 = QDir(matDir).absoluteFilePath(QStringLiteral("mat1.json"));
    QFile mf(mat1);
    QVERIFY(mf.open(QIODevice::WriteOnly));
    mf.write(QJsonDocument(mat).toJson());
    mf.close();

    // Incomplete material: only Diffuse is assigned.
    QJsonObject mat2;
    mat2.insert(QStringLiteral("name"), QStringLiteral("Mat2"));
    mat2.insert(QStringLiteral("template"), QStringLiteral("TestMat"));
    QJsonObject t2;
    t2.insert(QStringLiteral("Diffuse"), QStringLiteral("tex_Diffuse.png"));
    mat2.insert(QStringLiteral("textures"), t2);
    const QString mat2Path = QDir(matDir).absoluteFilePath(QStringLiteral("mat2.json"));
    QFile mf2(mat2Path);
    QVERIFY(mf2.open(QIODevice::WriteOnly));
    mf2.write(QJsonDocument(mat2).toJson());
    mf2.close();

    // Unknown template.
    QJsonObject mat3;
    mat3.insert(QStringLiteral("name"), QStringLiteral("Mat3"));
    mat3.insert(QStringLiteral("template"), QStringLiteral("DoesNotExist"));
    const QString mat3Path = QDir(matDir).absoluteFilePath(QStringLiteral("mat3.json"));
    QFile mf3(mat3Path);
    QVERIFY(mf3.open(QIODevice::WriteOnly));
    mf3.write(QJsonDocument(mat3).toJson());
    mf3.close();

    const AssetToolCliBridge::PipelineSummary s = mBridge->packageMaterials(
        { mat1, mat2Path, mat3Path }, rulesDir, outDir, texRoot);
    QCOMPARE(s.total, 3);
    QCOMPARE(s.success, 1);
    QCOMPARE(s.failed, 2);

    // The complete material produced a "ok" manifest.
    const QString manifest1 = QDir(outDir).absoluteFilePath(QStringLiteral("Mat1.compiled.json"));
    QVERIFY(QFile::exists(manifest1));
    QFile man1File(manifest1);
    QVERIFY(man1File.open(QIODevice::ReadOnly));
    QJsonObject man1 = QJsonDocument::fromJson(man1File.readAll()).object();
    QCOMPARE(man1.value(QStringLiteral("name")).toString(), QStringLiteral("Mat1"));
    QCOMPARE(man1.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
    QCOMPARE(man1.value(QStringLiteral("missingSlots")).toArray().size(), 0);
    QCOMPARE(man1.value(QStringLiteral("unresolvedTextures")).toArray().size(), 0);

    // The incomplete material wrote an "incomplete" manifest listing its unresolved slots.
    const QString manifest2 = QDir(outDir).absoluteFilePath(QStringLiteral("Mat2.compiled.json"));
    QVERIFY(QFile::exists(manifest2));
    QFile man2File(manifest2);
    QVERIFY(man2File.open(QIODevice::ReadOnly));
    QJsonObject man2 = QJsonDocument::fromJson(man2File.readAll()).object();
    QCOMPARE(man2.value(QStringLiteral("status")).toString(), QStringLiteral("incomplete"));
    QVERIFY(man2.value(QStringLiteral("unresolvedTextures")).toArray().size() > 0);

    // Unknown template: no manifest, error recorded.
    QVERIFY(!QFile::exists(QDir(outDir).absoluteFilePath(QStringLiteral("Mat3.compiled.json"))));
    QVERIFY(s.errors.join(QLatin1Char(' ')).contains(QStringLiteral("template not found: DoesNotExist")));
}

void TestAssetToolCli::testSignals()
{
    makeBridge();
    QSignalSpy started(mBridge, &AssetToolCliBridge::pipelineStarted);
    QSignalSpy processed(mBridge, &AssetToolCliBridge::fileProcessed);
    // PipelineSummary is not a registered Qt meta-type, so capture it via a
    // direct lambda connection instead of QSignalSpy.
    int finishedCount = 0;
    AssetToolCliBridge::PipelineSummary lastSummary;
    QObject::connect(mBridge, &AssetToolCliBridge::pipelineFinished,
                     [&](const AssetToolCliBridge::PipelineSummary& s) {
        ++finishedCount;
        lastSummary = s;
    });

    const QString src = mDir.filePath(QStringLiteral("sig.png"));
    QImage img(8, 8, QImage::Format_RGBA8888);
    img.fill(QColor(1, 2, 3, 255));
    QVERIFY(img.save(src, "PNG"));

    mBridge->convertTextures({ src, QStringLiteral("Z:/missing.png") }, mDir.path(), QStringLiteral("png"));

    QCOMPARE(started.count(), 1);
    QCOMPARE(finishedCount, 1);
    QCOMPARE(processed.count(), 2);
    QCOMPARE(lastSummary.total, 2);
    QCOMPARE(lastSummary.success, 1);
    QCOMPARE(lastSummary.skipped, 1);
}

QTEST_MAIN(TestAssetToolCli)
#include "test_assettoolcli.moc"

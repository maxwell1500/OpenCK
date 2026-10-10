#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>

#include "../../src/model/tools/materialcompiler.hpp"
#include "../../src/model/tools/materialruletemplate.hpp"
#include "../../libs/files/log/logger.hpp"

class TestMaterialCompiler : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testCompileOk();
    void testCompileUnresolvedTexture();
    void testSummary();
};

void TestMaterialCompiler::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_materialcompiler_log.txt"));
}

static bool makeTexture(const QString& path)
{
    QImage img(16, 16, QImage::Format_RGBA8888);
    img.fill(QColor(200, 100, 50, 255));
    return img.save(path, "PNG");
}

void TestMaterialCompiler::testCompileOk()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();

    // Provide a real texture on disk for every builtin slot.
    QMap<QString, QString> slotMap;
    const QStringList builtins = MaterialRuleTemplate::builtinLayerSlots();
    for (const QString& slot : builtins) {
        const QString path = QDir(root).absoluteFilePath(QStringLiteral("tex_%1.png").arg(slot));
        QVERIFY(makeTexture(path));
        slotMap.insert(slot, QFileInfo(path).fileName());
    }

    const MaterialRuleTemplate tpl = MaterialRuleTemplate::builtinTemplate(QStringLiteral("1LayerStandard"));
    const MaterialCompileReport report = MaterialCompiler::compile(tpl, slotMap, root);

    QVERIFY(report.ok);
    QCOMPARE(report.templateName, QStringLiteral("1LayerStandard"));
    QCOMPARE(report.missingSlots.size(), 0);
    QCOMPARE(report.unresolvedTextures.size(), 0);
    QCOMPARE(report.resolvedSlots.size(), builtins.size());
    for (const QString& slot : builtins)
        QVERIFY2(report.resolvedSlots.contains(slot), qPrintable(slot));
    QVERIFY(report.summary().startsWith(QStringLiteral("Compiled with template '1LayerStandard'")));
}

void TestMaterialCompiler::testCompileUnresolvedTexture()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();

    QMap<QString, QString> slotMap;
    // Only Diffuse is assigned, and its file does not exist; the template's
    // Add Layer1 fills the other slots with empty paths.
    slotMap.insert(QStringLiteral("Diffuse"), QStringLiteral("does-not-exist.png"));

    const MaterialRuleTemplate tpl = MaterialRuleTemplate::builtinTemplate(QStringLiteral("1LayerStandard"));
    const MaterialCompileReport report = MaterialCompiler::compile(tpl, slotMap, root);

    QVERIFY(!report.ok);
    QVERIFY(report.unresolvedTextures.contains(QStringLiteral("Diffuse")));
    // Every other builtin slot has no texture and is unresolved as well.
    const QStringList builtins = MaterialRuleTemplate::builtinLayerSlots();
    QCOMPARE(report.unresolvedTextures.size(), builtins.size());
    // The Add rule inserted all slots, so nothing is "missing" — just unresolved.
    QCOMPARE(report.missingSlots.size(), 0);
    QCOMPARE(report.resolvedSlots.size(), 0);
    // At least one warning for the missing file.
    QVERIFY(!report.warnings.isEmpty());
}

void TestMaterialCompiler::testSummary()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QMap<QString, QString> slotMap;
    slotMap.insert(QStringLiteral("Diffuse"), QStringLiteral("missing.png"));
    const MaterialRuleTemplate tpl = MaterialRuleTemplate::builtinTemplate(QStringLiteral("1LayerStandard"));
    const MaterialCompileReport report = MaterialCompiler::compile(tpl, slotMap, dir.path());

    QVERIFY(!report.ok);
    QVERIFY(report.summary().startsWith(QStringLiteral("Compile failed")));
    QVERIFY(report.summary().contains(QStringLiteral("unresolved textures")));
}

QTEST_MAIN(TestMaterialCompiler)
#include "test_materialcompiler.moc"

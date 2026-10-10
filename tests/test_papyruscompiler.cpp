#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include "../../src/view/window/papyruscompiler.hpp"
#include "../../libs/files/log/logger.hpp"

class TestPapyrusCompiler : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testStructuredWithColumn();
    void testStructuredNoColumn();
    void testFullPath();
    void testWarningSeverity();
    void testNonDiagnosticLine();
    void testSyntheticToolchain();
    void testStarfieldToolchainDetection();
};

void TestPapyrusCompiler::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_papyruscompiler_log.txt"));
}

void TestPapyrusCompiler::testStructuredWithColumn()
{
    CompilerError error;
    const bool ok = PapyrusCompiler::parseDiagnostic(
        QStringLiteral("Data/Scripts/Source/MyScript.psc(12,5): error: Variable not defined"),
        error);
    QVERIFY(ok);
    QVERIFY(error.file.endsWith(QStringLiteral("MyScript.psc")));
    QCOMPARE(error.line, 12);
    QCOMPARE(error.column, 5);
    QCOMPARE(error.severity, CompilerError::Severity::Error);
    QCOMPARE(error.message, QStringLiteral("Variable not defined"));
}

void TestPapyrusCompiler::testStructuredNoColumn()
{
    CompilerError error;
    const bool ok = PapyrusCompiler::parseDiagnostic(
        QStringLiteral("foo.psc(3): Warning: Unused variable"),
        error);
    QVERIFY(ok);
    QCOMPARE(error.line, 3);
    QCOMPARE(error.column, 0);
    QCOMPARE(error.severity, CompilerError::Severity::Warning);
    QCOMPARE(error.message, QStringLiteral("Unused variable"));
}

void TestPapyrusCompiler::testFullPath()
{
    CompilerError error;
    const bool ok = PapyrusCompiler::parseDiagnostic(
        QStringLiteral("C:/games/Data/Scripts/Source/QuestScript.psc(99,1): fatal: Out of memory"),
        error);
    QVERIFY(ok);
    QVERIFY(error.file.endsWith(QStringLiteral("QuestScript.psc")));
    QCOMPARE(error.line, 99);
    QCOMPARE(error.severity, CompilerError::Severity::Fatal);
    QCOMPARE(error.message, QStringLiteral("Out of memory"));
}

void TestPapyrusCompiler::testWarningSeverity()
{
    CompilerError error;
    QVERIFY(PapyrusCompiler::parseDiagnostic(
        QStringLiteral("w.psc(1): warning: Deprecated call"), error));
    QCOMPARE(error.severity, CompilerError::Severity::Warning);
}

void TestPapyrusCompiler::testNonDiagnosticLine()
{
    CompilerError error;
    QVERIFY(!PapyrusCompiler::parseDiagnostic(
        QStringLiteral("Compiling 3 scripts..."), error));
    QVERIFY(!PapyrusCompiler::parseDiagnostic(
        QStringLiteral("No errors."), error));
}
void TestPapyrusCompiler::testSyntheticToolchain()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString compilerExe = dir.filePath(QStringLiteral("PapyrusCompiler.exe"));
    {
        QFile f(compilerExe);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(12 * 1024, '\0'));
        f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    }

    const QString dll = dir.filePath(QStringLiteral("PCompiler.dll"));
    {
        QFile f2(dll);
        QVERIFY(f2.open(QIODevice::WriteOnly));
        f2.write("dll");
    }

    const QString xsd = dir.filePath(QStringLiteral("PapyrusProject.xsd"));
    {
        QFile f3(xsd);
        QVERIFY(f3.open(QIODevice::WriteOnly));
        f3.write("<schema/>");
    }

    PapyrusCompiler compiler;
    QVERIFY(compiler.setCompilerPath(compilerExe));
    QVERIFY(compiler.isStarfieldToolchain());
    QCOMPARE(compiler.toolchainDirectory(), dir.path());
    QCOMPARE(compiler.compilerDllPath(), dll);
    QCOMPARE(compiler.projectSchemaPath(), xsd);
}

void TestPapyrusCompiler::testStarfieldToolchainDetection()
{
    const QString compilerPath = PapyrusCompiler::detectCompilerPath();
    if (compilerPath.isEmpty()) {
        QSKIP("No installed Papyrus compiler found on machine");
    }

    PapyrusCompiler compiler;
    QVERIFY(compiler.setCompilerPath(compilerPath));
    QVERIFY(!compiler.toolchainDirectory().isEmpty());
}

QTEST_MAIN(TestPapyrusCompiler)
#include "test_papyruscompiler.moc"

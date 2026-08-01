#include <QTest>
#include <QTemporaryDir>
#include <QFile>

#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/npcrecord.hpp"

#include <QProcess>
#include <QCoreApplication>

// End-to-end test for the headless CLI: builds a tiny plugin file with the
// ESM writer, then runs `openck --cli export` against it and checks the
// output file appears.
class TestCli : public QObject
{
    Q_OBJECT

private slots:
    void testHelp();
    void testExport();
};

static QString openckExePath()
{
    return QCoreApplication::applicationDirPath() + "/openck.exe";
}

void TestCli::testHelp()
{
    QProcess process;
    process.start(openckExePath(), { "--cli", "help" });
    QVERIFY(process.waitForFinished(15000));
    QCOMPARE(process.exitCode(), 0);
    const QString out = QString::fromUtf8(process.readAllStandardOutput());
    QVERIFY(out.contains("--cli export"));
    QVERIFY(out.contains("--cli info"));
}

void TestCli::testExport()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString pluginPath = dir.filePath("test.esp");

    // Build a minimal plugin with one NPC record.
    {
        QFile file(pluginPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("CLI Test");
        writer.save(file);

        NpcRecord npc;
        npc.editorId = "CLITestNPC";
        npc.formId = 0x1234;
        npc.fullName = "CLI Test Character";
        npc.level = 5;

        RecHeader recHeader;
        recHeader.id = 0x1234;
        writer.startRecord('NPC_', recHeader);
        npc.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    const QString outPath = dir.filePath("out.json");
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(openckExePath(), {
        "--cli", "export", pluginPath,
        "--format", "json",
        "--out", outPath
    });
    QVERIFY(process.waitForFinished(30000));
    const QString cliOut = QString::fromUtf8(process.readAllStandardOutput());
    qWarning() << "CLI output:" << cliOut;

    QFile outFile(outPath);
    QVERIFY2(outFile.exists(), "export output file was not created");
    QVERIFY(outFile.open(QIODevice::ReadOnly));
    const QByteArray data = outFile.readAll();
    outFile.close();

    QVERIFY(!data.isEmpty());
    QVERIFY(data.contains("CLITestNPC"));
}

QTEST_GUILESS_MAIN(TestCli)
#include "test_cli.moc"

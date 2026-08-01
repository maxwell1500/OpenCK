#include <QTest>
#include <QTemporaryDir>

#include "../../src/model/tools/audiopipelinetools.hpp"

class TestAudioPipelineTools : public QObject
{
    Q_OBJECT

private slots:
    void testToolNames();
    void testFindTool();
    void testLipGeneratorArguments();
    void testFacefxArguments();
    void testRoboVoicerArguments();
    void testWwiseCodecId();
};

void TestAudioPipelineTools::testToolNames()
{
    QCOMPARE(AudioPipelineTools::toolName(AudioPipelineTools::Tool::LipGenerator),
             QStringLiteral("LipGenerator"));
    QCOMPARE(AudioPipelineTools::toolName(AudioPipelineTools::Tool::FaceFx),
             QStringLiteral("FaceFX"));
    QCOMPARE(AudioPipelineTools::toolName(AudioPipelineTools::Tool::Wwise),
             QStringLiteral("Wwise"));
    QCOMPARE(AudioPipelineTools::toolName(AudioPipelineTools::Tool::RoboVoicer),
             QStringLiteral("RoboVoicer"));
}

void TestAudioPipelineTools::testFindTool()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString base = dir.path();

    QDir(base).mkpath(QStringLiteral("LipGenerator"));
    QFile lip(base + QStringLiteral("/LipGenerator/LipGenerator.exe"));
    lip.open(QIODevice::WriteOnly);
    lip.write("MZ");
    lip.close();

    QDir(base).mkpath(QStringLiteral("FaceFX"));
    QFile ffx(base + QStringLiteral("/FaceFX/ffxc.exe"));
    ffx.open(QIODevice::WriteOnly);
    ffx.write("MZ");
    ffx.close();

    QCOMPARE(AudioPipelineTools::findTool(AudioPipelineTools::Tool::LipGenerator, base),
             base + QStringLiteral("/LipGenerator/LipGenerator.exe"));
    QCOMPARE(AudioPipelineTools::findTool(AudioPipelineTools::Tool::FaceFx, base),
             base + QStringLiteral("/FaceFX/ffxc.exe"));

    // Missing tools return empty.
    QVERIFY(AudioPipelineTools::findTool(AudioPipelineTools::Tool::Wwise, base).isEmpty());
    QVERIFY(AudioPipelineTools::findTool(AudioPipelineTools::Tool::LipGenerator,
                                         base + QStringLiteral("/nope")).isEmpty());
}

void TestAudioPipelineTools::testLipGeneratorArguments()
{
    const QStringList args = AudioPipelineTools::lipGeneratorArguments(
        QStringLiteral("C:/Tools/LipGenerator.exe"),
        QStringLiteral("C:/voice.wav"),
        QStringLiteral("C:/voice.lip"),
        QStringLiteral("C:/FonixData.cdf"),
        22050);

    QCOMPARE(args.size(), 9);
    QCOMPARE(args[0], QStringLiteral("C:/Tools/LipGenerator.exe"));
    QCOMPARE(args[1], QStringLiteral("-wav"));
    QCOMPARE(args[2], QStringLiteral("C:/voice.wav"));
    QCOMPARE(args[3], QStringLiteral("-out"));
    QCOMPARE(args[4], QStringLiteral("C:/voice.lip"));
    QCOMPARE(args[5], QStringLiteral("-data"));
    QCOMPARE(args[6], QStringLiteral("C:/FonixData.cdf"));
    QCOMPARE(args[7], QStringLiteral("-rate"));
    QCOMPARE(args[8], QStringLiteral("22050"));
}

void TestAudioPipelineTools::testFacefxArguments()
{
    const QStringList args = AudioPipelineTools::facefxArguments(
        QStringLiteral("C:/FaceFX/ffxc.exe"),
        QStringLiteral("C:/facefx/actor.ffproj"),
        QStringLiteral("C:/facefx/out"));

    QCOMPARE(args.size(), 5);
    QCOMPARE(args[0], QStringLiteral("C:/FaceFX/ffxc.exe"));
    QCOMPARE(args[1], QStringLiteral("-project"));
    QCOMPARE(args[2], QStringLiteral("C:/facefx/actor.ffproj"));
    QCOMPARE(args[3], QStringLiteral("-out"));
    QCOMPARE(args[4], QStringLiteral("C:/facefx/out"));
}

void TestAudioPipelineTools::testRoboVoicerArguments()
{
    const QStringList args = AudioPipelineTools::roboVoicerArguments(
        QStringLiteral("C:/RoboVoicer.exe"),
        QStringLiteral("Hello there"),
        QStringLiteral("C:/out.wav"));

    QCOMPARE(args.size(), 5);
    QCOMPARE(args[2], QStringLiteral("Hello there"));
    QCOMPARE(args[4], QStringLiteral("C:/out.wav"));
}

void TestAudioPipelineTools::testWwiseCodecId()
{
    QCOMPARE(AudioPipelineTools::wwiseExternalCodecId(), 4);
}

QTEST_MAIN(TestAudioPipelineTools)
#include "test_audiopipelinetools.moc"

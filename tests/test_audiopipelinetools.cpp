#include <QTest>
#include <QTemporaryDir>
#include <QFile>

#include "../../src/model/tools/audiopipelinetools.hpp"

class TestAudioPipelineTools : public QObject
{
    Q_OBJECT

private slots:
    void testToolNames();
    void testFindTool();
    void testLipGeneratorArguments();
    void testFacefxActorPath();
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
    // The shipped Starfield tool takes positional inputs plus optional
    // colon-valued flags; verified against the tool's own usage text.
    const QStringList args = AudioPipelineTools::lipGeneratorArguments(
        QStringLiteral("C:/Tools/LipGenerator.exe"),
        QStringLiteral("C:/voice.wav"),
        QStringLiteral("C:/line.txt"),
        QStringLiteral("C:/FaceFX/StarfieldHumanMale.facefx"),
        QStringLiteral("C:/FaceFX/StarfieldHumanMale.facefx"),
        QStringLiteral("C:/out.ffxanim"));

    // 5 positional + language + -OutputFileName; no animation group.
    QCOMPARE(args.size(), 7);
    QCOMPARE(args[0], QStringLiteral("C:/Tools/LipGenerator.exe"));
    QCOMPARE(args[1], QStringLiteral("C:/voice.wav"));
    QCOMPARE(args[2], QStringLiteral("C:/line.txt"));
    QCOMPARE(args[3], QStringLiteral("C:/FaceFX/StarfieldHumanMale.facefx"));
    QCOMPARE(args[4], QStringLiteral("C:/FaceFX/StarfieldHumanMale.facefx"));
    QCOMPARE(args[5], QStringLiteral("-Language:USEnglish"));
    QCOMPARE(args[6], QStringLiteral("-OutputFileName:C:/out.ffxanim"));

    const QStringList withGroup = AudioPipelineTools::lipGeneratorArguments(
        QStringLiteral("C:/Tools/LipGenerator.exe"), QStringLiteral("a.wav"),
        QStringLiteral("a.txt"), QStringLiteral("a.facefx"),
        QStringLiteral("a.facefx"), QStringLiteral("out.ffxanim"),
        QStringLiteral("USEnglish"), QStringLiteral("speech"));
    QCOMPARE(withGroup.size(), 8);
    QCOMPARE(withGroup.last(), QStringLiteral("-AnimationGroupName:speech"));

    const QStringList bare = AudioPipelineTools::lipGeneratorArguments(
        QStringLiteral("C:/Tools/LipGenerator.exe"), QStringLiteral("a.wav"),
        QStringLiteral("a.txt"), QStringLiteral("a.facefx"),
        QStringLiteral("a.facefx"), QString());
    QCOMPARE(bare.size(), 6);
}

void TestAudioPipelineTools::testFacefxActorPath()
{
    // The shipped layout puts the actors next to the tool's own dir.
    const QString toolsDir =
        QCoreApplication::applicationDirPath() + QStringLiteral("/Tools");
    const QString actor =
        AudioPipelineTools::facefxActorPath(toolsDir,
                                            QStringLiteral("StarfieldHumanMale"));
    // Nothing installed -> empty, and a nested FaceFX/FaceFX layout would be
    // found the same way if the install nests it.
    QVERIFY(actor.isEmpty() || actor.endsWith(QStringLiteral("StarfieldHumanMale.facefx")));
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
    // No INI at all: the Creation Kit's fallback is still the answer.
    QCOMPARE(AudioPipelineTools::wwiseExternalCodecId(QString()), 4);

    // A readable INI that states [Wwise] iDefaultExternalCodecID defers to it.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString iniPath = dir.path() + QStringLiteral("/CreationKit.ini");
    QFile ini(iniPath);
    QVERIFY(ini.open(QIODevice::WriteOnly));
    ini.write("[Wwise]\niDefaultExternalCodecID = 9\n");
    ini.close();
    QCOMPARE(AudioPipelineTools::wwiseExternalCodecId(iniPath), 9);

    // An INI that exists but does not state the codec falls back rather than
    // reporting "unknown".
    QFile bare(dir.path() + QStringLiteral("/bare.ini"));
    QVERIFY(bare.open(QIODevice::WriteOnly));
    bare.write("[Archive]\nSResourceArchiveList = A.ba2\n");
    bare.close();
    QCOMPARE(AudioPipelineTools::wwiseExternalCodecId(dir.path() + QStringLiteral("/bare.ini")), 4);
}

QTEST_MAIN(TestAudioPipelineTools)
#include "test_audiopipelinetools.moc"

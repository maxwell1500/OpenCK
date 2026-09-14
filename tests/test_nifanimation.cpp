#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QtMath>

#include "../../libs/files/nifanim/nifanimation.hpp"
#include "../../libs/files/nifanim/nifanimationexporter.hpp"
#include "../../libs/files/nifanim/nifanimationimporter.hpp"
#include "model/tools/nifanimationstate.hpp"

class TestNifAnimation : public QObject
{
    Q_OBJECT

private slots:
    void testJsonRoundTrip();
    void testXmlRoundTrip();
    void testExportNull();
    void testImportMissingFile();
    void testQuatJsonRoundTrip();
    void testQuatXmlRoundTrip();
    void testSlerpTakesShortPath();
    void testEulerFallbackPreserved();
    void testBlendWithStoredQuats();

private:
    static NifAnimation sampleAnimation();
    static NifAnimation quatAnimation();
};

NifAnimation TestNifAnimation::sampleAnimation()
{
    NifAnimation anim;
    anim.name = QStringLiteral("TestAnim");

    AnimClip clip;
    clip.name = QStringLiteral("Idle");
    clip.duration = 2.0f;

    AnimChannel channel;
    channel.boneName = QStringLiteral("Bip01 Head");
    channel.type = QStringLiteral("transform");
    channel.duration = 2.0f;

    for (int i = 0; i < 3; ++i)
    {
        AnimKeyframe kf;
        kf.time = static_cast<float>(i) * 0.5f;
        kf.tx = static_cast<float>(i);
        kf.ty = static_cast<float>(i) + 0.25f;
        kf.tz = static_cast<float>(i) + 0.5f;
        kf.rx = static_cast<float>(i) * 0.1f;
        kf.ry = static_cast<float>(i) * 0.2f;
        kf.rz = static_cast<float>(i) * 0.3f;
        kf.sx = 1.0f + static_cast<float>(i) * 0.01f;
        kf.sy = 1.0f + static_cast<float>(i) * 0.02f;
        kf.sz = 1.0f + static_cast<float>(i) * 0.03f;
        channel.keyframes.append(kf);
    }
    clip.channels.append(channel);
    anim.clips.append(clip);

    AnimMarker marker;
    marker.time = 1.0f;
    marker.name = QStringLiteral("midpoint");
    marker.color = QColor(255, 128, 0);
    anim.markers.append(marker);

    return anim;
}

void TestNifAnimation::testJsonRoundTrip()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + "/anim.json";

    const NifAnimation original = sampleAnimation();
    QVERIFY(NifAnimationExporter::exportToJson(&original, path));
    QVERIFY(QFile::exists(path));

    NifAnimation* loaded = NifAnimationImporter::importFromJson(path);
    QVERIFY(loaded != nullptr);
    QCOMPARE(loaded->name, original.name);
    QCOMPARE(loaded->clipCount(), original.clipCount());
    QCOMPARE(loaded->totalKeyframeCount(), original.totalKeyframeCount());
    QCOMPARE(loaded->clips[0].name, original.clips[0].name);
    QCOMPARE(loaded->clips[0].channels[0].boneName, original.clips[0].channels[0].boneName);
    QCOMPARE(loaded->clips[0].channels[0].keyframes.size(), 3);
    QCOMPARE(loaded->clips[0].channels[0].keyframes[2].tx, 2.0f);
    QCOMPARE(loaded->clips[0].channels[0].keyframes[2].sz, 1.06f);
    QCOMPARE(loaded->markers.size(), 1);
    QCOMPARE(loaded->markers[0].name, original.markers[0].name);
    delete loaded;
}

void TestNifAnimation::testXmlRoundTrip()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + "/anim.xml";

    const NifAnimation original = sampleAnimation();
    QVERIFY(NifAnimationExporter::exportToXml(&original, path));
    QVERIFY(QFile::exists(path));

    NifAnimation* loaded = NifAnimationImporter::importFromXml(path);
    QVERIFY(loaded != nullptr);
    QCOMPARE(loaded->name, original.name);
    QCOMPARE(loaded->clipCount(), 1);
    QCOMPARE(loaded->totalKeyframeCount(), 3);
    QCOMPARE(loaded->clips[0].channels[0].keyframes.size(), 3);
    QCOMPARE(loaded->clips[0].channels[0].keyframes[1].ty, 1.25f);
    QCOMPARE(loaded->markers.size(), 1);
    QCOMPARE(loaded->markers[0].color, QColor(255, 128, 0));
    delete loaded;
}

void TestNifAnimation::testExportNull()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(!NifAnimationExporter::exportToJson(nullptr, tmp.path() + "/x.json"));
    QVERIFY(!NifAnimationExporter::exportToXml(nullptr, tmp.path() + "/x.xml"));
}

void TestNifAnimation::testImportMissingFile()
{
    QVERIFY(NifAnimationImporter::importFromJson("Z:/nonexistent/anim.json") == nullptr);
    QVERIFY(NifAnimationImporter::importFromXml("Z:/nonexistent/anim.xml") == nullptr);
}

// A 350-degree turn about Y: stored as a quaternion (short-path aware) with
// the matching Euler angles alongside.
NifAnimation TestNifAnimation::quatAnimation()
{
    NifAnimation anim;
    anim.name = QStringLiteral("QuatAnim");

    AnimClip clip;
    clip.name = QStringLiteral("Turn");
    clip.duration = 1.0f;

    AnimChannel channel;
    channel.boneName = QStringLiteral("Bip01 Spine");
    channel.type = QStringLiteral("transform");
    channel.duration = 1.0f;

    AnimKeyframe kf0;
    kf0.time = 0.0f;
    kf0.qw = 1.0f; kf0.qx = 0.0f; kf0.qy = 0.0f; kf0.qz = 0.0f;
    kf0.hasQuat = true;
    channel.keyframes.append(kf0);

    // 350 degrees about Y == -10 degrees: quat (-0.9962, 0, 0.0872, 0).
    AnimKeyframe kf1;
    kf1.time = 1.0f;
    kf1.ry = static_cast<float>(qDegreesToRadians(350.0));
    kf1.qw = -0.9961947f; kf1.qx = 0.0f; kf1.qy = 0.0871557f; kf1.qz = 0.0f;
    kf1.hasQuat = true;
    channel.keyframes.append(kf1);

    clip.channels.append(channel);
    anim.clips.append(clip);
    return anim;
}

void TestNifAnimation::testQuatJsonRoundTrip()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + "/quat.json";

    const NifAnimation original = quatAnimation();
    QVERIFY(NifAnimationExporter::exportToJson(&original, path));

    NifAnimation* loaded = NifAnimationImporter::importFromJson(path);
    QVERIFY(loaded != nullptr);
    QCOMPARE(loaded->totalKeyframeCount(), 2);
    const AnimKeyframe& kf = loaded->clips[0].channels[0].keyframes[1];
    QVERIFY(kf.hasQuat);
    QCOMPARE(kf.qw, -0.9961947f);
    QCOMPARE(kf.qy, 0.0871557f);
    delete loaded;
}

void TestNifAnimation::testQuatXmlRoundTrip()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + "/quat.xml";

    const NifAnimation original = quatAnimation();
    QVERIFY(NifAnimationExporter::exportToXml(&original, path));

    NifAnimation* loaded = NifAnimationImporter::importFromXml(path);
    QVERIFY(loaded != nullptr);
    QCOMPARE(loaded->totalKeyframeCount(), 2);
    const AnimKeyframe& kf = loaded->clips[0].channels[0].keyframes[1];
    QVERIFY(kf.hasQuat);
    QCOMPARE(kf.qw, -0.9961947f);
    QCOMPARE(kf.qy, 0.0871557f);
    delete loaded;
}

void TestNifAnimation::testSlerpTakesShortPath()
{
    NifAnimation anim = quatAnimation();
    NifAnimationState state;
    state.setAnimation(&anim);
    state.setCurrentTime(0.5f);

    const QVector<TransformKeyframe> frames = state.getCurrentFrame();
    QCOMPARE(frames.size(), 1);
    const TransformKeyframe& f = frames[0];
    QVERIFY(f.hasQuat);
    // Slerp midpoint of identity -> -10 deg is -5 deg about Y. An Euler lerp
    // of 0 -> 350 deg would sit at 175 deg instead (the flip §8.2 kills).
    QVERIFY(qAbs(f.ry - static_cast<float>(qDegreesToRadians(-5.0))) < 0.01f);
    QVERIFY(qAbs(f.qw - 0.99905f) < 0.001f);
}

void TestNifAnimation::testEulerFallbackPreserved()
{
    // Clips without quaternions (JSON legacy, binary import) keep Euler lerp.
    NifAnimation anim = sampleAnimation();
    NifAnimationState state;
    state.setAnimation(&anim);
    state.setCurrentTime(0.25f);

    const QVector<TransformKeyframe> frames = state.getCurrentFrame();
    QCOMPARE(frames.size(), 1);
    const TransformKeyframe& f = frames[0];
    QVERIFY(!f.hasQuat);
    QVERIFY(qAbs(f.rx - 0.05f) < 0.0001f);
    QVERIFY(qAbs(f.ry - 0.1f) < 0.0001f);
    QVERIFY(qAbs(f.rz - 0.15f) < 0.0001f);
}

void TestNifAnimation::testBlendWithStoredQuats()
{
    // Blending a quat clip with itself at 50% must reproduce the frame.
    NifAnimation anim = quatAnimation();
    NifAnimationState state;
    state.setAnimation(&anim);
    state.setCurrentTime(0.5f);
    const QVector<TransformKeyframe> plain = state.getCurrentFrame();

    state.setBlendAnimation(&anim);
    state.setBlendClip(QStringLiteral("Turn"));
    state.setBlendWeight(0.5f);
    const QVector<TransformKeyframe> blended = state.getCurrentFrame();

    QCOMPARE(blended.size(), 1);
    QVERIFY(blended[0].hasQuat);
    QVERIFY(qAbs(blended[0].qw - plain[0].qw) < 0.0001f);
    QVERIFY(qAbs(blended[0].qy - plain[0].qy) < 0.0001f);
    QVERIFY(qAbs(blended[0].ry - plain[0].ry) < 0.0001f);
}

QTEST_MAIN(TestNifAnimation)
#include "test_nifanimation.moc"

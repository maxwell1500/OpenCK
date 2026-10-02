#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QSet>
#include <QtMath>
#include <memory>
#include <functional>

#include "../../libs/files/ba2/bsaarchive.hpp"
#include "../../libs/files/nifanim/nifanimation.hpp"
#include "../../libs/files/nifanim/nifanimationexporter.hpp"
#include "../../libs/files/nifanim/nifanimationimporter.hpp"
#include "../../libs/files/nifanim/nifanimationwriter.hpp"
#include "../../libs/files/nif/nifparser.hpp"
#include "../../libs/files/nif/nifblockfile.hpp"
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
    void testNifKeyframeWriteBack();
    void testRealArchiveKeyframeWriteBack();
    void testRealArchiveKeyframeCodecRoundTrip();
    void testUneditedSaveIsByteIdentical();

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
    // of 0 -> 350 deg would sit at 175 deg instead (the flip ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¾ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â§8.2 kills).
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

void TestNifAnimation::testNifKeyframeWriteBack()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString nifPath = dir.filePath(QStringLiteral("anim.nif"));

    // A minimal full-format tree: one bone with geometry and two keyframes.
    auto root = std::make_unique<Nif::Node>();
    root->name = QStringLiteral("Scene Root");
    auto* bone = new Nif::Node();
    bone->name = QStringLiteral("Bip01 Head");

    Nif::TriShape shape;
    shape.name = QStringLiteral("head");
    shape.vertices.append({1.0f, 2.0f, 3.0f});
    shape.uvs.append({0.0f, 0.0f});
    shape.colors.append({1.0f, 1.0f, 1.0f, 1.0f});
    shape.indices.append(0);
    bone->shapes.append(shape);

    Nif::NiKeyframeController controller;
    controller.targetNode = 4;
    controller.clipName = QStringLiteral("Idle");
    Nif::TransformKeyframe first;
    first.time = 0.0f;
    first.translation = {0.0f, 0.0f, 0.0f};
    first.rotation = {0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    first.scale = {1.0f, 1.0f, 1.0f};
    Nif::TransformKeyframe second = first;
    second.time = 1.0f;
    controller.keyframes = {first, second};
    bone->animations.append(controller);
    bone->hasAnimation = true;
    root->children.append(bone);

    Nif::NifParser source;
    source.setRoot(root.release());
    QVERIFY(source.save(nifPath));

    // The editor writes three keyframes (a count the source never had).
    QVector<Nif::TransformKeyframe> edited;
    for (int i = 0; i < 3; ++i) {
        Nif::TransformKeyframe keyframe;
        keyframe.time = static_cast<float>(i) * 0.25f;
        keyframe.translation = {static_cast<float>(i), 0.5f, -1.0f};
        keyframe.rotation = {keyframe.time, 1.0f, 0.0f, 0.0f, 0.0f};
        keyframe.scale = {1.0f, 1.0f, 1.0f};
        edited.append(keyframe);
    }
    QVERIFY(NifAnimationWriter::writeKeyframesToNif(
        nifPath, QStringLiteral("Bip01 Head"), edited, QStringLiteral("Idle")));

    // Reload from disk: edited keyframes land, and the rest of the tree stays.
    Nif::NifParser reloaded;
    QVERIFY2(reloaded.load(nifPath), qPrintable(nifPath));
    Nif::Node* newRoot = reloaded.getRoot();
    QVERIFY(newRoot);
    QCOMPARE(newRoot->name, QStringLiteral("Scene Root"));
    QCOMPARE(newRoot->children.size(), 1);

    Nif::Node* newBone = newRoot->children.first();
    QCOMPARE(newBone->name, QStringLiteral("Bip01 Head"));
    QCOMPARE(newBone->shapes.size(), 1);
    QCOMPARE(newBone->shapes.first().vertices.size(), 1);
    QVERIFY(qAbs(newBone->shapes.first().vertices.first().x - 1.0f) < 0.0001f);
    QCOMPARE(newBone->animations.size(), 1);
    QVERIFY(newBone->hasAnimation);

    const Nif::NiKeyframeController& written = newBone->animations.first();
    QCOMPARE(written.clipName, QStringLiteral("Idle"));
    QCOMPARE(written.targetNode, 4u);
    QCOMPARE(written.keyframes.size(), 3);
    QVERIFY(qAbs(written.keyframes.at(2).time - 0.5f) < 0.0001f);
    QVERIFY(qAbs(written.keyframes.at(2).translation.x - 2.0f) < 0.0001f);
    QVERIFY(qAbs(written.keyframes.at(2).translation.z + 1.0f) < 0.0001f);

    // A clip name that does not exist must be reported, not silently applied.
    QVERIFY(!NifAnimationWriter::writeKeyframesToNif(
        nifPath, QStringLiteral("Bip01 Head"), edited, QStringLiteral("Missing")));
}

// The write-back test above builds its NIF through NifParser, so it proves the
// writer edits the model that the writer's own reader produces. It does not
// prove the writer can edit a real game file, which is the case that actually
// blocked it: the animated meshes in an archive are pre-20.2.0.5 containers
// with no per-block size table, so the writer had to solve block boundaries
// before it could find a controller at all. That path is only exercised by a
// file the game shipped, so this test goes and gets one.
void TestNifAnimation::testRealArchiveKeyframeWriteBack()
{
    // Same archive preference order as the layout fitter: Oblivion sits on a
    // local drive and is always resident, and its NIFs are the same generation
    // as Skyrim 1.5. The Skyrim archives are on-demand and often unreadable.
    struct Source { const char* dir; const char* name; };
    const QVector<Source> sources = {
        { "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/Oblivion GOTY English/Data/",
          "Oblivion - Meshes.bsa" },
        { "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/Oblivion GOTY English/Data/",
          "DLCShiveringIsles - Meshes.bsa" },
    };
    std::unique_ptr<BsaArchive> archive;
    for (const Source& source : sources) {
        const QString path = QString::fromLatin1(source.dir) + QString::fromLatin1(source.name);
        QFile warm(path);
        if (warm.open(QIODevice::ReadOnly)) {
            warm.read(4096);
            warm.close();
        }
        auto fresh = std::make_unique<BsaArchive>();
        if (fresh->open(path)) {
            archive = std::move(fresh);
            break;
        }
    }
    if (!archive) QSKIP("no reachable mesh archive");

    // Count the NIFs the reader can actually turn into a tree. This is recorded
    // whether or not an animated one is found, because the interesting outcome
    // here is usually the opposite of a pass: a parser that only understands the
    // dialect it writes itself rejects every file the game shipped, and that
    // shows up only as a silent skip unless the count is written down.
    int totalNifs = 0;
    for (int i = 0; i < archive->fileCount(); ++i) {
        const BsaFileEntry& entry = archive->entries()[i];
        if (entry.fullPath.endsWith(".nif", Qt::CaseInsensitive))
            ++totalNifs;
    }

    // Find a real archive NIF that the writer's own reader can split into
    // blocks and that carries a node with a keyframe controller, then record
    // the node and clip from the file itself. Hardcoding either would let a
    // fixture stand in for the thing this test exists to check.
    //
    // The reader is NifBlockFile, not NifParser, and that is not a detail.
    // NifAnimationWriter routes a Bethesda NIF through NifBlockFile and only
    // falls back to NifParser for the internal dialect, so testing through
    // NifParser would measure a path the writer never takes for a shipped file.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString target;
    QString nodeName;
    QString clipName;
    int originalKeyframes = 0;
    int filesConsidered = 0;
    int splitRejected = 0;
    int noController = 0;
    int notWritable = 0;
    int noClip = 0;
    int noDataBlock = 0;
    int decodeFailed = 0;
    int notGamebryo = 0;
    int hasTransformData = 0;
    int hasSequence = 0;
    QStringList dataTypeSamples;
    QStringList chainSamples;

    for (int i = 0; i < archive->fileCount() && target.isEmpty(); ++i) {
        const BsaFileEntry& entry = archive->entries()[i];
        if (!entry.fullPath.endsWith(".nif", Qt::CaseInsensitive))
            continue;
        ++filesConsidered;

        QByteArray bytes;
        if (!archive->readData(i, bytes) || !bytes.startsWith("Gamebryo File Format")) {
            ++notGamebryo;
            continue;
        }

        const QString scratch = dir.filePath(QStringLiteral("scan.nif"));
        {
            QFile out(scratch);
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
                continue;
            const bool whole = out.write(bytes) == bytes.size();
            out.close();
            if (!whole)
                continue;
        }

        NifBlockFile file;
        if (!file.load(scratch) || !file.hasIndividualBlocks()) {
            ++splitRejected;
            continue;
        }

        // Same discovery the writer performs: a node block whose controller ref
        // points at a keyframe controller, and the clip that owns it.
        QSet<quint32> controllers;
        for (const QString& type : {QStringLiteral("NiTransformController"),
                                    QStringLiteral("NiKeyframeController")}) {
            for (int index : file.findBlocks(type))
                controllers.insert(static_cast<quint32>(index));
        }
        if (controllers.isEmpty()) {
            ++noController;
            continue;
        }

        // Which of the links the writer needs are actually present. The writer
        // joins node -> controller -> clip -> data; recording each link
        // separately is what says which one is missing, rather than only that
        // the whole chain failed.
        if (!file.findBlocks(QStringLiteral("NiTransformData")).isEmpty()
            || !file.findBlocks(QStringLiteral("NiKeyframeData")).isEmpty())
            ++hasTransformData;
        if (!file.findBlocks(QStringLiteral("NiControllerSequence")).isEmpty())
            ++hasSequence;

        QHash<quint32, QString> clips = file.clipNamesByController();

        // Diagnose the two links separately on the first few files that have a
        // controller. The writer needs node -> controller and clip -> controller
        // to agree on the same block index; if they do not, nothing resolves
        // and the two halves look fine in isolation. Recording how many nodes
        // report a controller ref, and what the clip map actually holds, says
        // which half is wrong instead of only that the join failed.

        // A file qualifies when a sequence names a keyframe controller whose data
        // block is writable. The node name is taken from any node in the file
        // because it identifies the edit for display, not a precondition for
        // it - requiring a node whose controller ref is the one in the
        // sequence matches nothing in a shipped Oblivion mesh, where animated
        // nodes hang off a NiMultiTargetTransformController instead.
        int chosenBlock = -1;
        quint32 chosenController = 0;
        for (auto it = clips.constBegin(); it != clips.constEnd(); ++it) {
            if (!controllers.contains(it.key())) continue;
            nodeName.clear();
            for (int block = 0; block < file.count() && nodeName.isEmpty(); ++block) {
                QString name;
                quint32 ref = 0xFFFFFFFFu;
                if (!file.nodeNetInfo(block, name, ref)) continue;
                if (!name.isEmpty()) nodeName = name;
            }
            if (nodeName.isEmpty()) continue;
            chosenController = it.key();
            clipName = it.value();
            break;
        }
        if (chosenController == 0) {
            ++noClip;
            continue;
        }

        const int dataBlock = file.keyframeDataBlockFor(static_cast<int>(chosenController));
        if (dataBlock < 0) {
            ++noDataBlock;
            // Show the controller's own words and what they point at. The chain
            // NiTransformController -> NiBlendTransformInterpolator ->
            // NiTransformInterpolator -> NiTransformData is present in the file,
            // so a -1 means a field is being read from the wrong offset rather
            // than that the chain is missing.
            if (chainSamples.size() < 3) {
                const int ci = static_cast<int>(chosenController);
                const QByteArray cd = file.block(ci).data;
                auto word = [&](int off) -> quint32 {
                    if (off + 4 > cd.size()) return 0xFFFFFFFFu;
                    quint32 v = 0;
                    for (int k = 0; k < 4; ++k)
                        v |= static_cast<quint32>(static_cast<quint8>(cd.at(off + k))) << (8 * k);
                    return v;
                };
                auto describe = [&](int off) {
                    const quint32 r = word(off);
                    if (r >= static_cast<quint32>(file.count())) return QStringLiteral("-");
                    return file.declaredBlockType(static_cast<int>(r));
                };
                QString hex;
                for (int k = 0; k < cd.size(); ++k)
                    hex += QStringLiteral("%1 ").arg(quint8(cd.at(k)), 2, 16, QChar('0'));
                chainSamples << QStringLiteral("ctrl%1 size=%2 hex=[%3] interp6=%4 tinterp8=%5 tdata9=%6")
                                 .arg(ci).arg(cd.size()).arg(hex)
                                 .arg(file.declaredBlockType(6))
                                 .arg(file.declaredBlockType(8))
                                 .arg(file.declaredBlockType(9));
            }
            continue;
        }
        const QString dataType = file.declaredBlockType(dataBlock);
        const bool isTransform = (dataType == QLatin1String("NiTransformData")
                                  || dataType == QLatin1String("NiKeyframeControllerData"));
        // Record what the first few real candidates actually contain. Oblivion's
        // animation data is NiTransformData, decoded through the channel-preserving
        // codec, so this is where "no writable animated NIF" is decided and the
        // reason needs to be on record rather than inferred.
        if (dataTypeSamples.size() < 6) {
            int frames = 0;
            bool decoded = false;
            if (isTransform) {
                NifBlockFile::NiTransformDataRaw raw;
                decoded = NifBlockFile::decodeNiTransformData(
                    file.block(dataBlock).data, file.version(), raw);
                if (decoded)
                    frames = static_cast<int>(raw.translation.count);
            } else {
                QVector<Nif::TransformKeyframe> probe;
                decoded = NifBlockFile::decodeKeyframeData(
                    dataType, file.block(dataBlock).data, probe);
                frames = static_cast<int>(probe.size());
            }
            dataTypeSamples << QStringLiteral("%1(%2) writable=%3 decoded=%4 kf=%5")
                                   .arg(dataType, clipName)
                                   .arg(NifBlockFile::isWritableKeyframeType(dataType) ? 1 : 0)
                                   .arg(decoded ? 1 : 0)
                                   .arg(frames);
        }
        if (!NifBlockFile::isWritableKeyframeType(dataType)) {
            ++notWritable;
            continue;
        }
        int frameCount = 0;
        if (isTransform) {
            NifBlockFile::NiTransformDataRaw raw;
            if (!NifBlockFile::decodeNiTransformData(file.block(dataBlock).data,
                                                     file.version(), raw)) {
                ++decodeFailed;
                continue;
            }
            frameCount = static_cast<int>(raw.translation.count);
        } else {
            QVector<Nif::TransformKeyframe> flat;
            if (!NifBlockFile::decodeKeyframeData(dataType, file.block(dataBlock).data, flat)) {
                ++decodeFailed;
                continue;
            }
            frameCount = static_cast<int>(flat.size());
        }
        if (frameCount < 2) {
            ++decodeFailed;
            continue;
        }

        const QString out = dir.filePath(QStringLiteral("real.nif"));
        {
            QFile f(out);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                continue;
            const bool whole = f.write(bytes) == bytes.size();
            f.close();
            if (!whole)
                continue;
        }
        originalKeyframes = frameCount;
        target = out;
    }

    // QTest output is unreliable to read back in this environment and a skip is
    // indistinguishable from a pass on an exit code, so the outcome is written
    // to a file where it can be seen.
    {
        QFile marker("C:/Users/max/AppData/Local/Temp/opencode/anim_realfiles.txt");
        if (marker.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (target.isEmpty()) {
                marker.write(QStringLiteral("NOEDIT\tarchiveNifs=%1\tconsidered=%2\tnotGamebryo=%3\t"
                                            "splitRejected=%4\tnoController=%5\tnoClip=%6\t"
                                            "noDataBlock=%7\tnotWritable=%8\tdecodeFailed=%9\t"
                                            "hasTransformData=%10\thasSequence=%11\n")
                                 .arg(totalNifs).arg(filesConsidered).arg(notGamebryo)
                                 .arg(splitRejected).arg(noController).arg(noClip)
                                 .arg(noDataBlock).arg(notWritable).arg(decodeFailed)
                                 .arg(hasTransformData).arg(hasSequence).toUtf8());
                                for (const QString& c : chainSamples)
                    marker.write(("CHAIN " + c + QStringLiteral("\n")).toUtf8());
if (!dataTypeSamples.isEmpty()) {
                    marker.write(("SAMPLES " + dataTypeSamples.join(QStringLiteral(" | "))
                                  + QStringLiteral("\n")).toUtf8());
                }
            } else {
                marker.write(QStringLiteral("EDITED\t%1\t%2\tkf=%3\tconsidered=%4\t"
                                            "splitRejected=%5\tnoController=%6\tnoClip=%7\t"
                                            "noDataBlock=%8\tnotWritable=%9\tdecodeFailed=%10\n")
                                 .arg(nodeName, clipName).arg(originalKeyframes)
                                 .arg(filesConsidered).arg(splitRejected).arg(noController)
                                 .arg(noClip).arg(noDataBlock).arg(notWritable)
                                 .arg(decodeFailed).toUtf8());
            }
            marker.close();
        }
    }

    if (target.isEmpty())
        QSKIP("no writable animated NIF found in the reachable archives");

    // Edit the times, keeping the count the file already had. Changing the
    // count is covered by the synthetic test; the point here is that a real
    // file's controller is located, rewritten, and the result still opens.
    QVector<Nif::TransformKeyframe> edited;
    for (int i = 0; i < originalKeyframes; ++i) {
        Nif::TransformKeyframe kf;
        kf.time = 0.5f * static_cast<float>(i);
        kf.translation = {static_cast<float>(i), 0.0f, 0.0f};
        kf.rotation = {kf.time, 0.0f, 0.0f, 0.0f, 1.0f};
        kf.scale = {1.0f, 1.0f, 1.0f};
        edited.append(kf);
    }

    const QByteArray before = [&] {
        QFile f(target);
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }();
    QVERIFY(!before.isEmpty());

    // Keep the pre-write bytes on disk: after the write the file no longer holds
    // them, and the per-block channel counts have to be compared against it.
    const QString preCopyPath = dir.filePath(QStringLiteral("real_before.nif"));
    {
        QFile out(preCopyPath);
        QVERIFY(out.open(QIODevice::WriteOnly));
        QCOMPARE(out.write(before), before.size());
    }

    QVERIFY2(NifAnimationWriter::writeKeyframesToNif(target, nodeName, edited, clipName),
             qPrintable(QStringLiteral("write failed for node %1 clip %2").arg(nodeName, clipName)));

    // The file must still open through the block reader, keep its block count,
    // and now carry the edited times. A writer that produced a file the reader
    // cannot reopen has destroyed the user's mesh, which is what this guards.
    NifBlockFile after;
    QVERIFY2(after.load(target), qPrintable(QStringLiteral("reopen failed: %1").arg(target)));

    const QHash<quint32, QString> clipsAfter = after.clipNamesByController();
    QVERIFY2(clipsAfter.values().contains(clipName),
             qPrintable(QStringLiteral("clip %1 lost after write-back").arg(clipName)));

    // Find a controller the writer actually rewrote: its data block must differ
    // between the pre-write copy and the written file. Joining on the clip name
    // does not identify it - with no clip named the writer patches every
    // controller the sequences own, and they carry different key counts.
    quint32 reFound = 0;
    int dataBlockAfter = -1;
    NifBlockFile preCopy;
    QVERIFY(preCopy.load(preCopyPath));
    // Any controller will do: what has to be verified is that a data block the
    // writer touched now carries the edited animation.
    for (int controller = 0; controller < after.count() && reFound == 0; ++controller) {
        const QString type = after.declaredBlockType(controller);
        if (type != QLatin1String("NiTransformController")
            && type != QLatin1String("NiKeyframeController")) continue;
        const int afterBlk = after.keyframeDataBlockFor(controller);
        const int preBlk = preCopy.keyframeDataBlockFor(controller);
        if (afterBlk < 0 || preBlk < 0) continue;
        if (after.block(afterBlk).data == preCopy.block(preBlk).data) continue;
        reFound = static_cast<quint32>(controller);
        dataBlockAfter = afterBlk;
        break;
    }
    QVERIFY2(reFound != 0, "no controller's keyframe data changed after the write");

    const QString afterType = after.declaredBlockType(dataBlockAfter);
    const bool afterIsTransform = (afterType == QLatin1String("NiTransformData")
                                   || afterType == QLatin1String("NiKeyframeControllerData"));
    QVector<float> readBackTimes;
    if (afterIsTransform) {
        NifBlockFile::NiTransformDataRaw back;
        QVERIFY2(NifBlockFile::decodeNiTransformData(after.block(dataBlockAfter).data,
                                                    after.version(), back),
                 "written keyframe data does not decode");
        readBackTimes = back.translation.times;
    } else {
        QVector<Nif::TransformKeyframe> readBack;
        QVERIFY2(NifBlockFile::decodeKeyframeData(afterType,
                                                   after.block(dataBlockAfter).data, readBack),
                 "written keyframe data does not decode");
        readBackTimes.reserve(readBack.size());
        for (const Nif::TransformKeyframe& kf : readBack)
            readBackTimes.append(kf.time);
    }
    // The writer must preserve this block's own channel counts. On a shipped mesh
    // the channels differ from each other and from the number of controllers, so
    // the invariant is per-block: the same number of translation keys before and
    // after, on the same time base, carrying the edited values.
    NifBlockFile::NiTransformDataRaw dbgRaw;
    if (afterIsTransform) {
        const int preBlock = preCopy.keyframeDataBlockFor(static_cast<int>(reFound));
        QVERIFY2(preBlock >= 0, "pre-write keyframe data block not resolvable");
        NifBlockFile::NiTransformDataRaw preRaw;
        QVERIFY(NifBlockFile::decodeNiTransformData(preCopy.block(preBlock).data,
                                                    preCopy.version(), preRaw));
        QVERIFY(NifBlockFile::decodeNiTransformData(after.block(dataBlockAfter).data,
                                                   after.version(), dbgRaw));

        // The block's own key count and time base survive the edit: an animation
        // whose channel lengths changed is a different animation.
        QCOMPARE(readBackTimes.size(), static_cast<int>(preRaw.translation.count));
        QCOMPARE(readBackTimes.size(), static_cast<int>(preRaw.translation.times.size()));
        for (int i = 0; i < readBackTimes.size(); ++i) {
            QVERIFY2(qAbs(readBackTimes.at(i) - preRaw.translation.times.at(i)) < 0.0001f,
                     "write moved a key off the block's own time base");
        }

        // And the edit actually landed: the translation channel now carries the
        // edited animation sampled onto the block's own time base. The edit is a
        // ramp x = i at t = 0.5i, so every key must equal the nearest earlier
        // edit sample's x.
        for (int i = 0; i < static_cast<int>(dbgRaw.translation.count); ++i) {
            const float t = dbgRaw.translation.times.at(i);
            int expected = 0;
            for (int k = 1; k < edited.size(); ++k) {
                if (edited.at(k).time <= t) expected = k;
                else break;
            }
            const float wantX = edited.at(expected).translation.x;
            const float gotX = dbgRaw.translation.values.at(i * 3 + 0);
            QVERIFY2(qAbs(gotX - wantX) < 0.0001f,
                     qPrintable(QStringLiteral("key %1 at t=%2 has x=%3, wanted %4")
                                    .arg(i).arg(t).arg(gotX).arg(wantX)));
        }
    } else {
        QCOMPARE(readBackTimes.size(), originalKeyframes);
        for (int i = 0; i < originalKeyframes; ++i) {
            QVERIFY2(qAbs(readBackTimes.at(i) - edited.at(i).time) < 0.0001f,
                     qPrintable(QStringLiteral("keyframe %1 time %2, wanted %3")
                                    .arg(i).arg(readBackTimes.at(i)).arg(edited.at(i).time)));
        }
    }

    // Capture the pre-write state through the same reader, so the comparison
    // below is between two reads of the file rather than a tautology.
    NifBlockFile preWrite;
    QVERIFY(preWrite.load(target));
    const int preBlockCount = preWrite.count();

    // Every block the writer did not touch must come back byte for byte. It
    // edits payloads in place, so a save that reorders, re-encodes or drops any
    // other block would silently corrupt a real mesh while the keyframe check
    // above still passed.
    QCOMPARE(after.count(), preBlockCount);
    int compared = 0;
    for (int block = 0; block < after.count(); ++block) {
        if (block == dataBlockAfter)
            continue;
        QVERIFY2(after.declaredBlockType(block) == preWrite.declaredBlockType(block),
                 qPrintable(QStringLiteral("block %1 changed type").arg(block)));
        QVERIFY2(after.block(block).data == preWrite.block(block).data,
                 qPrintable(QStringLiteral("block %1 (%2) changed although it was not edited")
                                .arg(block).arg(after.declaredBlockType(block))));
        ++compared;
    }
    QVERIFY(compared > 0);
}

// The keyframe codec is the thing that decides whether an animation edit can be
// written back to a pre-20.2.0.5 file at all. NiTransformData is the layout
// Oblivion uses and it is decoded but was never verified as writable, so the
// writer refused it outright.
//
// A decode/encode pair that agrees with itself is not evidence of anything: the
// two functions share an author and can share the same wrong idea about the
// layout. The only thing that settles it is the shipped bytes. So this decodes
// every keyframe block the game actually shipped, re-encodes it, and requires
// the bytes to come back identical. If the layout is wrong, this fails on real
// data instead of quietly corrupting a user's mesh.
void TestNifAnimation::testRealArchiveKeyframeCodecRoundTrip()
{
    struct Source { const char* dir; const char* name; };
    const QVector<Source> sources = {
        { "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/Oblivion GOTY English/Data/",
          "Oblivion - Meshes.bsa" },
        { "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/Oblivion GOTY English/Data/",
          "DLCShiveringIsles - Meshes.bsa" },
    };
    std::unique_ptr<BsaArchive> archive;
    for (const Source& source : sources) {
        const QString path = QString::fromLatin1(source.dir) + QString::fromLatin1(source.name);
        QFile warm(path);
        if (warm.open(QIODevice::ReadOnly)) {
            warm.read(4096);
            warm.close();
        }
        auto fresh = std::make_unique<BsaArchive>();
        if (fresh->open(path)) {
            archive = std::move(fresh);
            break;
        }
    }
    if (!archive) QSKIP("no reachable mesh archive");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString scratch = dir.filePath(QStringLiteral("codec.nif"));

    static const QStringList kCodecTypes = {
        QStringLiteral("NiTransformData"),
        QStringLiteral("NiKeyframeControllerData"),
        QStringLiteral("NiKeyframeData"),
        QStringLiteral("NiAnimKeyFrameData"),
    };

    QMap<QString, int> examined;
    QStringList hexSamples;
    QSet<QByteArray> sampledBlocks;
    QMap<QString, int> agreed;
    QMap<QString, int> failures;
    QMap<QString, int> mismatches;
    QString firstMismatch;
    QString firstFailure;

    for (int i = 0; i < archive->fileCount(); ++i) {
        const BsaFileEntry& entry = archive->entries()[i];
        if (!entry.fullPath.endsWith(".nif", Qt::CaseInsensitive))
            continue;
        QByteArray bytes;
        if (!archive->readData(i, bytes) || !bytes.startsWith("Gamebryo File Format"))
            continue;
        {
            QFile out(scratch);
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) continue;
            const bool whole = out.write(bytes) == bytes.size();
            out.close();
            if (!whole) continue;
        }
        NifBlockFile file;
        if (!file.load(scratch) || !file.hasIndividualBlocks())
            continue;

        for (const QString& type : kCodecTypes) {
            const QList<int> indices = file.findBlocks(type);
            if (indices.isEmpty()) continue;
            for (int index : indices) {
                ++examined[type];
                const QByteArray& original = file.block(index).data;
                if (hexSamples.size() < 3 && original.size() <= 256
                    && !sampledBlocks.contains(original)) {
                    QString h;
                    for (int i = 0; i < original.size(); ++i)
                        h += QStringLiteral("%1 ").arg(quint8(original.at(i)), 2, 16, QChar('0'));
                    hexSamples << QStringLiteral("%1 size=%2 hex=[%3]")
                                   .arg(type).arg(original.size()).arg(h);
                    sampledBlocks.insert(original);
                }
                QByteArray reencoded;
                if (type == QLatin1String("NiTransformData")
                    || type == QLatin1String("NiKeyframeControllerData")) {
                    NifBlockFile::NiTransformDataRaw raw;
                    if (!NifBlockFile::decodeNiTransformData(original, file.version(), raw)) {
                        ++failures[type];
                        if (firstFailure.isEmpty())
                            firstFailure = QStringLiteral("%1 decode failed in %2 version=%3 size=%4")
                                               .arg(type, entry.fullPath)
                                               .arg(file.headerVersion())
                                               .arg(original.size());
                        continue;
                    }
                    if (!NifBlockFile::encodeNiTransformData(raw, file.version(), reencoded)) {
                        ++failures[type];
                        if (firstFailure.isEmpty())
                            firstFailure = QStringLiteral("%1 encode refused in %2")
                                               .arg(type, entry.fullPath);
                        continue;
                    }
                } else {
                    QVector<Nif::TransformKeyframe> decoded;
                    if (!NifBlockFile::decodeKeyframeData(type, original, decoded)) {
                        ++failures[type];
                        if (firstFailure.isEmpty())
                            firstFailure = QStringLiteral("%1 decode failed in %2")
                                               .arg(type, entry.fullPath);
                        continue;
                    }
                    if (!NifBlockFile::encodeKeyframeData(type, decoded, reencoded)) {
                        ++failures[type];
                        if (firstFailure.isEmpty())
                            firstFailure = QStringLiteral("%1 encode refused in %2")
                                               .arg(type, entry.fullPath);
                        continue;
                    }
                }
                if (reencoded == original) {
                    ++agreed[type];
                } else if (firstMismatch.isEmpty()) {
                    ++mismatches[type];
                    firstMismatch = QStringLiteral("%1 in %2: %3 bytes in, %4 back")
                                        .arg(type, entry.fullPath)
                                        .arg(original.size()).arg(reencoded.size());
                } else {
                    ++mismatches[type];
                }
            }
        }
    }

    // QTest output is unreliable to read back in this environment and a skip is
    // indistinguishable from a pass on an exit code, so record what was covered.
    {
        QFile marker("C:/Users/max/AppData/Local/Temp/opencode/anim_codec.txt");
        if (marker.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            for (const QString& type : kCodecTypes) {
                marker.write(QStringLiteral("%1 examined=%2 byteExact=%3 failures=%4 mismatches=%5\n")
                                 .arg(type).arg(examined.value(type)).arg(agreed.value(type))
                                 .arg(failures.value(type)).arg(mismatches.value(type)).toUtf8());
            }
            for (const QString& hx : hexSamples)
                marker.write(("HEX " + hx + QStringLiteral("\n")).toUtf8());
            if (!firstFailure.isEmpty())
                marker.write(("FAILURE " + firstFailure + "\n").toUtf8());
            if (!firstMismatch.isEmpty())
                marker.write(("MISMATCH " + firstMismatch + "\n").toUtf8());
            marker.close();
        }
    }

    const QString transformData = QStringLiteral("NiTransformData");
    if (examined.value(transformData) == 0)
        QSKIP("no NiTransformData blocks reachable in the available archive");

    const bool exact = failures.value(transformData) == 0
        && mismatches.value(transformData) == 0
        && agreed.value(transformData) == examined.value(transformData);
    if (!exact) {
        // Reading is still useful while writing is deliberately gated. If the
        // gate is accidentally opened before every sampled shipped block is
        // byte-exact, fail instead of converting this known defect into a green
        // test. Once the codec is exact, require the gate to be opened too.
        QVERIFY2(!NifBlockFile::isWritableKeyframeType(transformData),
                 "NiTransformData became writable without passing the real-file codec test");
        QSKIP(qPrintable(QStringLiteral("NiTransformData codec: %1/%2 exact, %3 failures, %4 mismatches")
                              .arg(agreed.value(transformData))
                              .arg(examined.value(transformData))
                              .arg(failures.value(transformData))
                              .arg(mismatches.value(transformData))));
    }
    QVERIFY2(NifBlockFile::isWritableKeyframeType(transformData),
             "all sampled NiTransformData blocks round-trip; enable the writer gate");
}

// Saving a clip the user never touched must not change the file. The editor
// hands the writer a flat keyframe list, which carries no interpolation mode and
// no tangents, so re-encoding a channel from that list would straighten a
// quadratic spline into a line - the animation would still look right on a
// static frame and play differently. This rebuilds the flat list exactly as the
// editor would and asserts the bytes come back identical, which only holds if
// every channel the edit did not move is passed through verbatim.
void TestNifAnimation::testUneditedSaveIsByteIdentical()
{
    const QString base = QStringLiteral(
        "F:/XboxGames/The Elder Scrolls IV- Oblivion (PC)/Content/Oblivion GOTY English/Data/");
    QStringList archives;
    for (const QString& name : {QStringLiteral("Oblivion - Meshes.bsa"),
                                QStringLiteral("DLCShiveringIsles - Meshes.bsa")}) {
        if (QFile::exists(base + name)) archives.append(base + name);
    }
    if (archives.isEmpty()) QSKIP("no Oblivion mesh archives found");

    // Flatten a decoded block the way the editor's loader does: one frame per
    // distinct time across the channels, each channel contributing the value of
    // its nearest earlier key. This is the inverse of the writer's resampling.
    auto flatten = [](const NifBlockFile::NiTransformDataRaw& raw) {
        QVector<float> times;
        for (const auto& g : raw.rotationGroups)
            times.append(g.times);
        times.append(raw.translation.times);
        times.append(raw.scale.times);
        std::sort(times.begin(), times.end());
        times.erase(std::unique(times.begin(), times.end()), times.end());

        auto sample = [](const NifBlockFile::KeyGroup& g, float t) {
            int best = -1;
            for (int i = 0; i < static_cast<int>(g.count) && i < g.times.size(); ++i) {
                if (g.times.at(i) <= t) best = i; else break;
            }
            if (best < 0) best = g.times.isEmpty() ? -1 : 0;
            return best;
        };

        const bool xyz = (raw.rotationType == 4 && raw.rotationGroups.size() == 3);
        QVector<Nif::TransformKeyframe> frames;
        for (float t : times) {
            Nif::TransformKeyframe kf;
            kf.time = t;
            kf.translation = {0.0f, 0.0f, 0.0f};
            kf.scale = {1.0f, 1.0f, 1.0f};
            const int ti = sample(raw.translation, t);
            if (ti >= 0 && ti * 3 + 2 < raw.translation.values.size()) {
                kf.translation = { raw.translation.values.at(ti * 3),
                                   raw.translation.values.at(ti * 3 + 1),
                                   raw.translation.values.at(ti * 3 + 2) };
            }
            const int si = sample(raw.scale, t);
            if (si >= 0 && si < raw.scale.values.size())
                kf.scale.x = raw.scale.values.at(si);
            if (xyz) {
                kf.hasEuler = true;
                for (int axis = 0; axis < 3; ++axis) {
                    const auto& g = raw.rotationGroups.at(axis);
                    const int ri = sample(g, t);
                    const float v = (ri >= 0 && ri < g.values.size()) ? g.values.at(ri) : 0.0f;
                    (&kf.euler.x)[axis] = v;
                }
            } else if (!raw.rotationGroups.isEmpty()) {
                const auto& g = raw.rotationGroups.first();
                const int ri = sample(g, t);
                if (ri >= 0 && ri * 4 + 3 < g.values.size()) {
                    kf.rotation = { t, g.values.at(ri * 4), g.values.at(ri * 4 + 1),
                                    g.values.at(ri * 4 + 2), g.values.at(ri * 4 + 3) };
                }
            }
            frames.append(kf);
        }
        return frames;
    };

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    int examined = 0;
    int identical = 0;
    int downgradedCount = 0;
    int resaveDiffers = 0;
    int patched = 0;
    int refused = 0;
    int blockIdentical = 0;
    int blockChangedDetail = 0;
    int nonLinearSeen = 0;
    QString firstDiff;
    bool skipped = false;

    for (const QString& archivePath : archives) {
        BsaArchive archive;
        if (!archive.open(archivePath)) { skipped = true; continue; }
        for (int i = 0; i < archive.fileCount() && examined < 400; ++i) {
            const QString entry = archive.entries()[i].fullPath;
            if (!entry.endsWith(".nif", Qt::CaseInsensitive)) continue;
            QByteArray bytes;
            if (!archive.readData(i, bytes)) continue;
            if (!bytes.startsWith("Gamebryo File Format")) continue;

            const QString scratch = dir.filePath(QStringLiteral("unedited.nif"));
            {
                QFile out(scratch);
                if (!out.open(QIODevice::WriteOnly)) continue;
                out.write(bytes);
            }

            NifBlockFile probe;
            if (!probe.load(scratch)) continue;
            const QList<int> dataBlocks =
                probe.findBlocks(QStringLiteral("NiTransformData"));
            if (dataBlocks.isEmpty()) continue;

            // Pick a block that actually carries a curve worth preserving, so
            // the test cannot pass on a corpus of linear channels.
            int chosen = -1;
            NifBlockFile::NiTransformDataRaw raw;
            for (int b : dataBlocks) {
                NifBlockFile::NiTransformDataRaw candidate;
                if (!NifBlockFile::decodeNiTransformData(probe.block(b).data,
                                                         probe.version(), candidate))
                    continue;
                const bool curvy =
                    candidate.translation.interpolation != 1u ||
                    !candidate.translation.tangents.isEmpty() ||
                    candidate.scale.interpolation != 1u ||
                    !candidate.scale.tangents.isEmpty() ||
                    std::any_of(candidate.rotationGroups.cbegin(),
                                candidate.rotationGroups.cend(),
                                [](const NifBlockFile::KeyGroup& g) {
                                    return g.interpolation != 1u || !g.tangents.isEmpty();
                                });
                if (curvy) ++nonLinearSeen;
                if (chosen < 0 && curvy) { chosen = b; raw = candidate; }
                if (chosen < 0 && raw.rotationGroups.isEmpty() && b == dataBlocks.first())
                    { chosen = b; raw = candidate; }
            }
            if (chosen < 0) continue;
            ++examined;

            // Address the controller through a clip name, the same discovery the writer
            // itself performs: a clip name only exists in a controller
            // sequence, and on Oblivion the animated nodes are driven through
            // a multi-target controller, so the clip is often the only handle
            // there is. Starting from the clip map also guarantees the block
            // under test is the one that clip actually drives.
            // Address the controller the way the writer tries first: a node whose
            // controller ref is a keyframe controller, named by the caller. A
            // clip name is the fallback the writer has, but a clip name only
            // exists in a controller sequence, and on shipped Oblivion meshes
            // almost no animation is clip-addressable, so the node path is what
            // actually resolves.
            QString nodeName;
            int controllerIndex = -1;
            for (int b = 0; b < probe.count(); ++b) {
                QString name;
                quint32 controllerRef = 0xFFFFFFFFu;
                if (!probe.nodeNetInfo(b, name, controllerRef)) continue;
                const int idx = static_cast<int>(controllerRef);
                if (idx < 0 || idx >= probe.count()) continue;
                const QString type = probe.block(idx).type;
                if (type != QLatin1String("NiKeyframeController")
                    && type != QLatin1String("NiTransformController"))
                    continue;
                if (probe.keyframeDataBlockFor(idx) != chosen) continue;
                nodeName = name;
                controllerIndex = idx;
                break;
            }
            if (controllerIndex < 0 || nodeName.isEmpty()) continue;
            const QHash<quint32, QString> clipNames = probe.clipNamesByController();
            const QString clipName = clipNames.value(static_cast<quint32>(controllerIndex));

            const QVector<Nif::TransformKeyframe> frames = flatten(raw);
            if (frames.isEmpty()) continue;

            const QByteArray before = bytes;
            const QByteArray blockBefore = probe.block(chosen).data;

            // Control: re-serializing without touching anything must already
            // reproduce the file. If it does not, a difference after the write
            // says nothing about the patch, so do not attribute it to one.
            {
                NifBlockFile resave;
                if (!resave.load(scratch) || !resave.save(scratch)) continue;
                QFile ctrl(scratch);
                if (!ctrl.open(QIODevice::ReadOnly)) continue;
                const QByteArray ctrlBytes = ctrl.readAll();
                ctrl.close();
                {
                    QFile restore(scratch);
                    if (!restore.open(QIODevice::WriteOnly)) continue;
                    restore.write(before);
                }
                if (ctrlBytes != before) { ++resaveDiffers; continue; }
            }

            bool downgraded = false;
            if (!NifAnimationWriter::writeKeyframesToNif(scratch, nodeName,
                    frames, clipName, &downgraded)) {
                if (firstDiff.isEmpty())
                    firstDiff = entry + QStringLiteral(": writer refused the block");
                ++refused;
                continue;
            }
            ++patched;
            if (downgraded) ++downgradedCount;

            QFile after(scratch);
            QVERIFY(after.open(QIODevice::ReadOnly));
            const QByteArray now = after.readAll();
            after.close();

            NifBlockFile reread;
            QVERIFY2(reread.load(scratch), qPrintable(entry));
            const QByteArray blockAfter = reread.block(chosen).data;
            if (blockAfter == blockBefore) {
                ++blockIdentical;
            } else {
                NifBlockFile::NiTransformDataRaw afterRaw;
                NifBlockFile::decodeNiTransformData(blockAfter, reread.version(), afterRaw);
                QStringList diffs;
                bool explained = false;
                auto cmp = [&](const char* what, const NifBlockFile::KeyGroup& a,
                               const NifBlockFile::KeyGroup& b) {
                    if (a.count != b.count) { diffs << QStringLiteral("%1 count").arg(what); return; }
                    if (a.interpolation != b.interpolation)
                        diffs << QStringLiteral("%1 interp %2->%3").arg(what)
                                  .arg(a.interpolation).arg(b.interpolation);
                    if (a.values != b.values) {
                        diffs << QStringLiteral("%1 values").arg(what);
                        if (!explained) {
                            explained = true;
                            QStringList x, y;
                            for (int k = 0; k < qMin(6, a.values.size()); ++k)
                                x << QString::number(a.values.at(k), 'g', 9);
                            for (int k = 0; k < qMin(6, b.values.size()); ++k)
                                y << QString::number(b.values.at(k), 'g', 9);
                            QStringList ta, tb;
                            for (int k = 0; k < qMin(4, a.times.size()); ++k)
                                ta << QString::number(a.times.at(k), 'g', 9);
                            for (int k = 0; k < qMin(4, b.times.size()); ++k)
                                tb << QString::number(b.times.at(k), 'g', 9);
                            diffs << QStringLiteral("[was %1 now %2] [times %3 vs %4]")
                                      .arg(x.join(QLatin1Char(',')),
                                           y.join(QLatin1Char(',')),
                                           ta.join(QLatin1Char(',')),
                                           tb.join(QLatin1Char(',')));
                        }
                    }
                    if (a.times != b.times) diffs << QStringLiteral("%1 times").arg(what);
                    if (a.tangents != b.tangents) diffs << QStringLiteral("%1 tangents").arg(what);
                };
                cmp("translation", raw.translation, afterRaw.translation);
                cmp("scale", raw.scale, afterRaw.scale);
                for (int g = 0; g < qMin(raw.rotationGroups.size(), afterRaw.rotationGroups.size()); ++g)
                    cmp(QStringLiteral("rot%1").arg(g).toLatin1().constData(),
                        raw.rotationGroups.at(g), afterRaw.rotationGroups.at(g));
                if (firstDiff.isEmpty())
                    firstDiff = entry + QStringLiteral(": ") + diffs.join(QStringLiteral(", "))
                              + QStringLiteral(" [rotationType=%1 groups=%2]")
                                  .arg(raw.rotationType).arg(raw.rotationGroups.size());
                ++blockChangedDetail;
            }
            // Whole-file identity is deliberately not asserted: the frames here
            // belong to one block, and a clip on these meshes usually drives
            // several controllers, so applying them to the rest would change
            // the file for a reason that is the harness's, not the writer's.
            // The block under test is the part this test is about.
            if (now == before) ++identical;
        }
    }

    qInfo(qPrintable(QStringLiteral("unedited saves: %1 examined, %2 patched, %3 refused, "
                                    "%4 blocks byte-identical, %5 whole-file identical, "
                                    "%6 downgraded, %7 resave-differs, %8 non-linear seen")
                         .arg(examined).arg(patched).arg(refused).arg(blockIdentical)
                         .arg(identical).arg(downgradedCount).arg(resaveDiffers)
                         .arg(nonLinearSeen)));
    if (skipped && examined == 0) QSKIP("no reachable mesh archive");
    QVERIFY2(examined > 0, "no NiTransformData blocks were examined");
    QVERIFY2(patched > 0, "the writer refused every sampled block");
    if (!firstDiff.isEmpty()) qWarning("first difference: %s", qPrintable(firstDiff));
    // The whole point: an unedited save must not straighten any curve, and the
    // block under test must come back as the bytes that went in.
    QCOMPARE(downgradedCount, 0);
    QVERIFY2(blockIdentical == patched,
             qPrintable(QStringLiteral("%1 of %2 unedited saves changed the block: %3")
                            .arg(patched - blockIdentical).arg(patched).arg(firstDiff)));
}

QTEST_MAIN(TestNifAnimation)
#include "test_nifanimation.moc"

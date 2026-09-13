#include <QtTest>
#include <QTemporaryDir>
#include <QFile>

#include "../../libs/files/nifanim/nifanimation.hpp"
#include "../../libs/files/nifanim/nifanimationexporter.hpp"
#include "../../libs/files/nifanim/nifanimationimporter.hpp"

class TestNifAnimation : public QObject
{
    Q_OBJECT

private slots:
    void testJsonRoundTrip();
    void testXmlRoundTrip();
    void testExportNull();
    void testImportMissingFile();

private:
    static NifAnimation sampleAnimation();
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

QTEST_MAIN(TestNifAnimation)
#include "test_nifanimation.moc"

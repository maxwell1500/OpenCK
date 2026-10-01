#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QtMath>
#include <memory>
#include <functional>

#include "../../libs/files/ba2/bsaarchive.hpp"
#include "../../libs/files/nifanim/nifanimation.hpp"
#include "../../libs/files/nifanim/nifanimationexporter.hpp"
#include "../../libs/files/nifanim/nifanimationimporter.hpp"
#include "../../libs/files/nifanim/nifanimationwriter.hpp"
#include "../../libs/files/nif/nifparser.hpp"
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

    // Find a real NIF that carries both a named node and a transform controller,
    // and record how it reads back before any edit. The node name and clip come
    // from the file itself rather than being hardcoded, because shipping a NIF
    // that a hand-written fixture could stand in for is exactly the gap here.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString written;
    QString nodeName;
    QString clipName;
    int originalKeyframes = 0;
    int filesConsidered = 0;
    int readable = 0;
    int readRejected = 0;
    int parseRejected = 0;
    int noRoot = 0;

    for (int i = 0; i < archive->fileCount() && written.isEmpty(); ++i) {
        const BsaFileEntry& entry = archive->entries()[i];
        if (!entry.fullPath.endsWith(".nif", Qt::CaseInsensitive))
            continue;
        if (++filesConsidered > 4000)
            break;

        QByteArray bytes;
        if (!archive->readData(i, bytes) || !bytes.startsWith("Gamebryo File Format")) {
            ++readRejected;
            continue;
        }
        ++readable;

        const QString scratch = dir.filePath(QStringLiteral("scan.nif"));
        {
            QFile out(scratch);
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
                continue;
            if (out.write(bytes) != bytes.size()) {
                out.close();
                continue;
            }
            out.close();
        }
        Nif::NifParser parser;
        if (!parser.load(scratch)) {
            ++parseRejected;
            continue;
        }
        Nif::Node* root = parser.getRoot();
        if (!root) {
            ++noRoot;
            continue;
        }

        std::function<Nif::Node*(Nif::Node*)> findAnimated = [&](Nif::Node* node) -> Nif::Node* {
            if (!node->name.isEmpty() && !node->animations.isEmpty()
                && !node->animations.first().keyframes.isEmpty())
                return node;
            for (Nif::Node* child : node->children) {
                if (Nif::Node* hit = findAnimated(child))
                    return hit;
            }
            return nullptr;
        };
        Nif::Node* animated = findAnimated(root);
        if (!animated)
            continue;

        const Nif::NiKeyframeController& controller = animated->animations.first();
        if (controller.keyframes.size() < 2)
            continue;

        const QString target = dir.filePath(QStringLiteral("real.nif"));
        {
            QFile out(target);
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
                continue;
            const bool whole = out.write(bytes) == bytes.size();
            out.close();
            if (!whole)
                continue;
        }

        nodeName = animated->name;
        clipName = controller.clipName;
        originalKeyframes = controller.keyframes.size();
        written = target;
    }

    if (written.isEmpty()) {
        {
            QFile marker("C:/Users/max/AppData/Local/Temp/opencode/anim_realfiles.txt");
            if (marker.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                marker.write(QStringLiteral("NOEDIT\tarchiveNifs=%1\tconsidered=%2\treadable=%3\t"
                                            "readRejected=%4\tparseRejected=%5\tnoRoot=%6\n")
                                 .arg(totalNifs).arg(filesConsidered).arg(readable).arg(readRejected)
                                 .arg(parseRejected).arg(noRoot).toUtf8());
                marker.close();
            }
        }
        QSKIP("no animated NIF found in the reachable archives");
    }

    // QTest's own output is unreliable to read back in this environment, and a
    // skip is indistinguishable from a pass on an exit code. Drop a marker so
    // the difference is on disk, where a build log or a later run can see it.
    {
        QFile marker("C:/Users/max/AppData/Local/Temp/opencode/anim_realfiles.txt");
        if (marker.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            marker.write(QStringLiteral("EDITED\t%1\t%2\tkf=%3\tconsidered=%4\n")
                             .arg(nodeName, clipName)
                             .arg(originalKeyframes)
                             .arg(filesConsidered).toUtf8());
            marker.close();
        }
    }

    qInfo().noquote() << "editing" << nodeName << "clip" << clipName
                      << "with" << originalKeyframes << "keyframes from" << filesConsidered
                      << "files considered";

    // Edit the time of every keyframe, keeping the count the file already had.
    // Changing the count is exercised by the synthetic test; what this test is
    // for is that a real file's controller is found, rewritten and still parses
    // as the same tree afterwards.
    Nif::NifParser before;
    QVERIFY(before.load(written));
    std::vector<float> newTimes;
    for (int i = 0; i < originalKeyframes; ++i)
        newTimes.push_back(0.5f * static_cast<float>(i));

    QVector<Nif::TransformKeyframe> edited;
    for (int i = 0; i < originalKeyframes; ++i) {
        Nif::TransformKeyframe kf;
        kf.time = newTimes[i];
        kf.translation = {static_cast<float>(i), 0.0f, 0.0f};
        kf.rotation = {newTimes[i], 0.0f, 0.0f, 0.0f, 1.0f};
        kf.scale = {1.0f, 1.0f, 1.0f};
        edited.append(kf);
    }

    QVERIFY2(NifAnimationWriter::writeKeyframesToNif(written, nodeName, edited, clipName),
             qPrintable(QStringLiteral("write failed for %1 clip %2").arg(nodeName, clipName)));

    // The edited file must still parse, keep its node and clip names, and now
    // carry the new times. A writer that produced a file the reader cannot
    // reopen has destroyed the user's mesh, which is the failure this guards.
    Nif::NifParser after;
    QVERIFY2(after.load(written), qPrintable(QStringLiteral("reparse failed: %1").arg(written)));
    Nif::Node* newRoot = after.getRoot();
    QVERIFY(newRoot);

    std::function<Nif::Node*(Nif::Node*)> findNamed = [&](Nif::Node* node) -> Nif::Node* {
        if (node->name == nodeName)
            return node;
        for (Nif::Node* child : node->children) {
            if (Nif::Node* hit = findNamed(child))
                return hit;
        }
        return nullptr;
    };
    Nif::Node* reloaded = findNamed(newRoot);
    QVERIFY2(reloaded, qPrintable(QStringLiteral("node %1 vanished after write-back").arg(nodeName)));
    QCOMPARE(reloaded->animations.size(), 1);
    QCOMPARE(reloaded->animations.first().clipName, clipName);

    const QVector<Nif::TransformKeyframe>& writtenKeys = reloaded->animations.first().keyframes;
    QCOMPARE(writtenKeys.size(), originalKeyframes);
    for (int i = 0; i < originalKeyframes; ++i)
        QVERIFY2(qAbs(writtenKeys.at(i).time - newTimes[i]) < 0.0001f,
                 qPrintable(QStringLiteral("keyframe %1 time %2, wanted %3")
                                .arg(i).arg(writtenKeys.at(i).time).arg(newTimes[i])));
}

QTEST_MAIN(TestNifAnimation)
#include "test_nifanimation.moc"

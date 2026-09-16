#include <QtTest>
#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QDirIterator>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <cmath>

#include "../../libs/files/nif/nifparser.hpp"
#include "../../libs/files/nif/nifskinning.hpp"
#include "../../libs/files/ba2/ba2archive.hpp"
#include "nifrecord.hpp"
#include "logger.hpp"
#include "model/tools/nifanimationstate.hpp"

// Per-vertex skinning (REMAINING.md §8.1): the GUI-free blend core, the
// NiSkinInstance/NiSkinData block codec, and a synthetic dialect file proving
// the loader links skin blocks to shapes end to end.
class TestNifSkinning : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBlendIdentity();
    void testSingleBoneEquivalence();
    void testTwoBoneMidpoint();
    void testWeightNormalization();
    void testUnweightedFallback();
    void testOutOfRangeIgnored();
    void testNormalRotation();
    void testSkinBlockRoundTrip();
    void testBlockConsumption();
    void testSyntheticWalkClean();
    void testPlaybackComposition();
    void testRealNifSurvey();
    void testExternalMeshData();
    void testFaceSkinBlocks();
    void testSyntheticSkinnedFileLoad();
};

namespace {

constexpr float kIdentity16[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};
constexpr float kIdentity9[9] = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1,
};

void translatePalette(float out[16], float dx, float dy, float dz)
{
    for (int i = 0; i < 16; ++i) out[i] = kIdentity16[i];
    out[3] = dx;
    out[7] = dy;
    out[11] = dz;
}

} // namespace

void TestNifSkinning::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_nifskinning_log.txt"));
}

void TestNifSkinning::testBlendIdentity()
{
    const float rest[6] = {1, 2, 3, 4, 5, 6};
    const float nrm[6] = {0, 0, 1, 0, 1, 0};
    Nif::SkinVertexWeight w;
    w.vertex = 0; w.bone = 0; w.weight = 1.0f;
    float outPos[6] = {};
    float outNrm[6] = {};
    Nif::blendSkinnedLocal(rest, nrm, 2, &kIdentity16, &kIdentity9, 1, &w, 1,
                           outPos, outNrm);
    QCOMPARE(outPos[0], 1.0f);
    QCOMPARE(outPos[2], 3.0f);
    // Vertex 1 has no weight: rest pose preserved.
    QCOMPARE(outPos[3], 4.0f);
    QCOMPARE(outPos[5], 6.0f);
    QCOMPARE(outNrm[2], 1.0f);
}

void TestNifSkinning::testSingleBoneEquivalence()
{
    // One weight-1.0 bone must equal the rigid owner transform.
    const float rest[3] = {1, 0, 0};
    const float nrm[3] = {0, 0, 1};
    float palette[16];
    translatePalette(palette, 5.0f, -2.0f, 0.5f);
    Nif::SkinVertexWeight w;
    w.vertex = 0; w.bone = 0; w.weight = 1.0f;
    float outPos[3] = {};
    float outNrm[3] = {};
    Nif::blendSkinnedLocal(rest, nrm, 1, &palette, &kIdentity9, 1, &w, 1,
                           outPos, outNrm);
    QCOMPARE(outPos[0], 6.0f);
    QCOMPARE(outPos[1], -2.0f);
    QCOMPARE(outPos[2], 0.5f);
}

void TestNifSkinning::testTwoBoneMidpoint()
{
    // Opposing translations at 50/50 cancel out.
    const float rest[3] = {1, 1, 1};
    const float nrm[3] = {0, 0, 1};
    float p0[16], p1[16];
    translatePalette(p0, 5.0f, 0.0f, 0.0f);
    translatePalette(p1, -5.0f, 0.0f, 0.0f);
    float palFlat[32];
    for (int i = 0; i < 16; ++i) { palFlat[i] = p0[i]; palFlat[16 + i] = p1[i]; }
    float nrmFlat[18];
    for (int i = 0; i < 9; ++i) { nrmFlat[i] = (i % 4 == 0) ? 1.0f : 0.0f; nrmFlat[9 + i] = nrmFlat[i]; }
    Nif::SkinVertexWeight ws[2];
    ws[0].vertex = 0; ws[0].bone = 0; ws[0].weight = 0.5f;
    ws[1].vertex = 0; ws[1].bone = 1; ws[1].weight = 0.5f;
    float outPos[3] = {};
    float outNrm[3] = {};
    Nif::blendSkinnedLocal(rest, nrm, 1,
                           reinterpret_cast<const float(*)[16]>(palFlat),
                           reinterpret_cast<const float(*)[9]>(nrmFlat),
                           2, ws, 2, outPos, outNrm);
    QVERIFY(qAbs(outPos[0] - 1.0f) < 0.0001f);
    QVERIFY(qAbs(outPos[1] - 1.0f) < 0.0001f);
    QVERIFY(qAbs(outPos[2] - 1.0f) < 0.0001f);
}

void TestNifSkinning::testWeightNormalization()
{
    // A single weight of 2.0 behaves like 1.0.
    const float rest[3] = {1, 0, 0};
    const float nrm[3] = {0, 0, 1};
    float palette[16];
    translatePalette(palette, 5.0f, 0.0f, 0.0f);
    Nif::SkinVertexWeight w;
    w.vertex = 0; w.bone = 0; w.weight = 2.0f;
    float outPos[3] = {};
    float outNrm[3] = {};
    Nif::blendSkinnedLocal(rest, nrm, 1, &palette, &kIdentity9, 1, &w, 1,
                           outPos, outNrm);
    QCOMPARE(outPos[0], 6.0f);
}

void TestNifSkinning::testUnweightedFallback()
{
    const float rest[3] = {7, 8, 9};
    const float nrm[3] = {0, 1, 0};
    float outPos[3] = {};
    float outNrm[3] = {};
    Nif::blendSkinnedLocal(rest, nrm, 1, &kIdentity16, &kIdentity9, 1,
                           nullptr, 0, outPos, outNrm);
    QCOMPARE(outPos[0], 7.0f);
    QCOMPARE(outPos[1], 8.0f);
    QCOMPARE(outPos[2], 9.0f);
    QCOMPARE(outNrm[1], 1.0f);
}

void TestNifSkinning::testOutOfRangeIgnored()
{
    const float rest[3] = {1, 2, 3};
    const float nrm[3] = {0, 0, 1};
    Nif::SkinVertexWeight ws[3];
    ws[0].vertex = 5; ws[0].bone = 0; ws[0].weight = 1.0f;  // bad vertex
    ws[1].vertex = 0; ws[1].bone = 7; ws[1].weight = 1.0f;  // bad bone
    ws[2].vertex = 0; ws[2].bone = 0; ws[2].weight = 0.0f;  // bad weight
    float outPos[3] = {};
    float outNrm[3] = {};
    Nif::blendSkinnedLocal(rest, nrm, 1, &kIdentity16, &kIdentity9, 1,
                           ws, 3, outPos, outNrm);
    QCOMPARE(outPos[0], 1.0f);
    QCOMPARE(outPos[1], 2.0f);
    QCOMPARE(outPos[2], 3.0f);
}

void TestNifSkinning::testNormalRotation()
{
    // 90-degree palette rotation about Z turns +X normals into +Y.
    const float rest[3] = {0, 0, 0};
    const float nrm[3] = {1, 0, 0};
    constexpr float rotZ90[16] = {
        0, -1, 0, 0,
        1, 0, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    constexpr float rotZ90Nrm[9] = {
        0, -1, 0,
        1, 0, 0,
        0, 0, 1,
    };
    Nif::SkinVertexWeight w;
    w.vertex = 0; w.bone = 0; w.weight = 1.0f;
    float outPos[3] = {};
    float outNrm[3] = {};
    Nif::blendSkinnedLocal(rest, nrm, 1, &rotZ90, &rotZ90Nrm, 1, &w, 1,
                           outPos, outNrm);
    QVERIFY(qAbs(outNrm[0] - 0.0f) < 0.0001f);
    QVERIFY(qAbs(outNrm[1] - 1.0f) < 0.0001f);
    QVERIFY(qAbs(outNrm[2] - 0.0f) < 0.0001f);
}

void TestNifSkinning::testSkinBlockRoundTrip()
{
    // Codec: write() output re-parses to identical fields (write() prefixes
    // the parse body with the className/dataRef framing, so parse from the
    // body offset).
    NifSkinInstance inst;
    inst.className = QStringLiteral("NiSkinInstance");
    inst.dataRef = 4;
    inst.refTargetShape = 2;
    inst.refSkeletonRoot = 0;
    inst.bones = {1};
    inst.refSkinData = 5;

    NifSkinData data;
    data.className = QStringLiteral("NiSkinData");
    data.dataRef = 5;
    NifSkinData::BoneWeights bw;
    for (int i = 0; i < 16; ++i) bw.bindPose[i] = kIdentity16[i];
    NifSkinData::BoneWeights::Influence i0;
    i0.vertex = 0; i0.weight = 1.0f;
    NifSkinData::BoneWeights::Influence i1;
    i1.vertex = 1; i1.weight = 0.5f;
    bw.weights = {i0, i1};
    data.bones = {bw};

    auto roundTrip = [](NifObject& obj) -> QByteArray {
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        obj.write(buf, 0);
        return buf.data();
    };
    auto bodyOffset = [](const QByteArray& bytes, const QString& name) -> int {
        const QByteArray latin = name.toLatin1();
        return 4 + latin.size() + 4;
    };

    {
        QByteArray bytes = roundTrip(inst);
        QBuffer in(&bytes);
        in.open(QIODevice::ReadOnly);
        in.seek(bodyOffset(bytes, inst.className));
        NifSkinInstance back;
        back.parse(in, 0, {});
        QCOMPARE(back.refTargetShape, quint32(2));
        QCOMPARE(back.refSkeletonRoot, quint32(0));
        QCOMPARE(back.bones, QList<quint32>({1}));
        QCOMPARE(back.refSkinData, quint32(5));
    }
    {
        QByteArray bytes = roundTrip(data);
        QBuffer in(&bytes);
        in.open(QIODevice::ReadOnly);
        in.seek(bodyOffset(bytes, data.className));
        NifSkinData back;
        back.parse(in, 0, {});
        QCOMPARE(back.bones.size(), 1);
        QCOMPARE(back.bones[0].bindPose[0], 1.0f);
        QCOMPARE(back.bones[0].bindPose[5], 1.0f);
        QCOMPARE(back.bones[0].weights.size(), 2);
        QCOMPARE(back.bones[0].weights[0].vertex, quint32(0));
        QCOMPARE(back.bones[0].weights[0].weight, 1.0f);
        QCOMPARE(back.bones[0].weights[1].vertex, quint32(1));
        QCOMPARE(back.bones[0].weights[1].weight, 0.5f);
    }
}

void TestNifSkinning::testBlockConsumption()
{
    // Pins each block's parse() byte consumption against its write().
    constexpr quint32 kVersion = 0x140200;
    {
        NifNode node;
        node.className = QStringLiteral("NiNode");
        node.dataRef = 0;
        node.name = QStringLiteral("Root");
        node.children = {1, 2};
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        node.write(buf, kVersion);
        const int framing = 4 + 6 + 4;
        const int bodyLen = buf.data().size() - framing;
        QByteArray bytes = buf.data();
        QBuffer in(&bytes);
        in.open(QIODevice::ReadOnly);
        in.seek(framing);
        NifNode back;
        back.parse(in, kVersion, {});
        QCOMPARE(static_cast<int>(in.pos()) - framing, bodyLen);
        QCOMPARE(back.children, QList<quint32>({1, 2}));
    }
    {
        NifTriShapeData data;
        data.className = QStringLiteral("NiTriShapeData");
        data.dataRef = 3;
        NiPoint3 v{1, 2, 3};
        data.vertices = {v, v};
        data.uvs = {{0, 0}, {1, 1}};
        data.normals = {v, v};
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        data.write(buf, kVersion);
        const int framing = 4 + 14 + 4;
        QCOMPARE(buf.data().size(), framing + 88);  // body per parse()
        QByteArray bytes = buf.data();
        QBuffer in(&bytes);
        in.open(QIODevice::ReadOnly);
        in.seek(framing);
        NifTriShapeData back;
        back.parse(in, kVersion, {});
        QCOMPARE(static_cast<int>(in.pos()), framing + 88);
        QCOMPARE(back.vertices.size(), 2);
    }
}

namespace {

// Minimal synthetic dialect-file writer (mirrors the parse() field order of
// each block). Version 0x140200 is below the 0x0A000000 NifObject-padding
// threshold, so block bodies carry no padding word.

void writeLenString(QDataStream& out, const char* s)
{
    const QByteArray bytes(s);
    out << static_cast<quint32>(bytes.size());
    out.writeRawData(bytes.constData(), bytes.size());
}

void beginBlock(QDataStream& out, const char* className, quint32 ref)
{
    writeLenString(out, className);
    out << ref;
}

void writeAvObject(QDataStream& out, const char* name,
                   const QVector<quint32>& children)
{
    writeLenString(out, name);
    out << quint32(0) << quint32(0) << quint32(0) << quint32(0);  // 4 refs
    out << static_cast<quint32>(children.size());
    for (quint32 c : children) out << c;
    out << quint32(0);  // no controllers
}

void writeNode(QDataStream& out, const char* className, quint32 ref,
               const char* name, const QVector<quint32>& children)
{
    beginBlock(out, className, ref);
    writeAvObject(out, name, children);
    out << 0.0f << 0.0f << 0.0f << 0.0f;  // bounding center + radius
}

void writeTestSkinnedFile(const QString& path)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    // The dialect stores 4-byte floats; QDataStream defaults to promoting
    // float<< to double, so pin single precision.
    out.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // parseNifHeader framing: 4 magic bytes, 3 version bytes, u16 + filename.
    out.writeRawData("NIF0", 4);
    out.writeRawData("\x14\x02\x00", 3);
    out << static_cast<quint16>(8);
    out.writeRawData("test.nif", 8);

    writeNode(out, "NiNode", 0, "Root", {1, 2});
    writeNode(out, "NiNode", 1, "Bone01", {});

    beginBlock(out, "NiTriShape", 2);
    writeAvObject(out, "Mesh01", {});
    writeLenString(out, "MeshData");
    out << quint32(3);  // refGeometryData

    beginBlock(out, "NiTriShapeData", 3);
    out << quint32(2);  // 2 vertices
    out << 0.0f << 0.0f << 0.0f << 1.0f << 0.0f << 0.0f;
    out << quint32(2);  // 2 uvs
    out << 0.0f << 0.0f << 1.0f << 1.0f;
    out << quint32(2);  // 2 normals
    out << 0.0f << 0.0f << 1.0f << 0.0f << 0.0f << 1.0f;
    out << quint32(0);  // no tangents
    out << quint32(0);  // no vertex colors
    out << quint32(0);  // no indices

    beginBlock(out, "NiSkinInstance", 4);
    out << quint32(2);  // refTargetShape
    out << quint32(0);  // refSkeletonRoot
    out << quint32(1);  // one bone
    out << quint32(1);  // bone block ref
    out << quint32(5);  // refSkinData

    beginBlock(out, "NiSkinData", 5);
    out << quint32(1);  // one bone
    for (int i = 0; i < 16; ++i) out << kIdentity16[i];
    out << quint32(2);  // two weights
    out << quint32(0) << 1.0f;
    out << quint32(1) << 0.5f;
}

const Nif::Node* findNamedChild(const Nif::Node* node, const QString& name)
{
    if (!node) return nullptr;
    for (const Nif::Node* child : node->children)
        if (child->name == name) return child;
    return nullptr;
}

} // namespace

void TestNifSkinning::testSyntheticWalkClean()
{
    // Replicates the loader's block walk over a synthetic file: every
    // framing must land and every parse() must consume exactly its body.
    QTemporaryFile file(QStringLiteral("XXXXXX.nif"));
    QVERIFY(file.open());
    const QString path = file.fileName();
    file.close();
    writeTestSkinnedFile(path);

    QFile in(path);
    QVERIFY(in.open(QIODevice::ReadOnly));
    // Skip the parseNifHeader framing.
    QVERIFY(in.read(4).size() == 4);
    QVERIFY(in.read(3).size() == 3);
    quint16 nameLen = 0;
    QVERIFY(in.read(reinterpret_cast<char*>(&nameLen), 2) == 2);
    QVERIFY(in.read(nameLen).size() == nameLen);

    const QStringList expected = {QStringLiteral("NiNode"),
                                  QStringLiteral("NiNode"),
                                  QStringLiteral("NiTriShape"),
                                  QStringLiteral("NiTriShapeData"),
                                  QStringLiteral("NiSkinInstance"),
                                  QStringLiteral("NiSkinData")};
    // Body sizes mirrored from writeTestSkinnedFile.
    const QList<qint64> bodySizes = {56, 50, 50, 88, 20, 88};
    for (int b = 0; b < expected.size(); ++b)
    {
        quint32 classNameLen = 0;
        QVERIFY(in.read(reinterpret_cast<char*>(&classNameLen), 4) == 4);
        const QByteArray nameBytes = in.read(classNameLen);
        QCOMPARE(nameBytes.size(), static_cast<int>(classNameLen));
        QCOMPARE(QString::fromLatin1(nameBytes), expected[b]);
        quint32 dataRef = 0;
        QVERIFY(in.read(reinterpret_cast<char*>(&dataRef), 4) == 4);
        QCOMPARE(dataRef, static_cast<quint32>(b));

        NifObject* obj = nullptr;
        const QString className = QString::fromLatin1(nameBytes);
        if (className == QStringLiteral("NiNode"))
            obj = new NifNode();
        else if (className == QStringLiteral("NiTriShape"))
            obj = new NifTriShape();
        else if (className == QStringLiteral("NiTriShapeData"))
            obj = new NifTriShapeData();
        else if (className == QStringLiteral("NiSkinInstance"))
            obj = new NifSkinInstance();
        else if (className == QStringLiteral("NiSkinData"))
            obj = new NifSkinData();
        QVERIFY(obj != nullptr);
        const qint64 before = in.pos();
        obj->parse(in, 0x140200, {});
        const qint64 consumed = in.pos() - before;
        delete obj;
        QCOMPARE(consumed, bodySizes[b]);
    }
    QCOMPARE(in.pos(), in.size());
    QFile::remove(path);
}

void TestNifSkinning::testPlaybackComposition()
{
    // The headless playback pipeline (§8.3): sample a slerped animation
    // frame, build the bone palette the way the viewport does, blend a
    // skinned vertex through the shared core, and check the world position.
    NifAnimation anim;
    anim.name = QStringLiteral("Playback");
    AnimClip clip;
    clip.name = QStringLiteral("Turn");
    clip.duration = 1.0f;
    AnimChannel channel;
    channel.boneName = QStringLiteral("Bone01");
    channel.type = QStringLiteral("transform");
    AnimKeyframe kf0;
    kf0.time = 0.0f;
    kf0.qw = 1.0f;
    kf0.hasQuat = true;
    channel.keyframes.append(kf0);
    AnimKeyframe kf1;
    kf1.time = 1.0f;
    kf1.qw = -0.9961947f; kf1.qy = 0.0871557f;  // 350 deg about Y == -10 deg
    kf1.hasQuat = true;
    channel.keyframes.append(kf1);
    clip.channels.append(channel);
    anim.clips.append(clip);

    NifAnimationState state;
    state.setAnimation(&anim);
    state.setCurrentTime(0.5f);  // slerp midpoint: -5 deg about Y
    const QVector<TransformKeyframe> frames = state.getCurrentFrame();
    QCOMPARE(frames.size(), 1);
    QVERIFY(frames[0].hasQuat);

    // Viewport-style palette: animated bone world (from the frame quat) times
    // the rest-world inverse (identity here), owner at identity.
    QMatrix4x4 boneWorld;
    boneWorld.rotate(QQuaternion(frames[0].qw, frames[0].qx,
                                 frames[0].qy, frames[0].qz));
    const float* colMajor = boneWorld.constData();
    float palette[16];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            palette[i * 4 + j] = colMajor[j * 4 + i];

    const float rest[3] = {1, 0, 0};
    const float nrm[3] = {0, 0, 1};
    Nif::SkinVertexWeight w;
    w.vertex = 0; w.bone = 0; w.weight = 1.0f;
    float outPos[3] = {};
    float outNrm[3] = {};
    Nif::blendSkinnedLocal(rest, nrm, 1, &palette, &kIdentity9, 1, &w, 1,
                           outPos, outNrm);
    // (1,0,0) rotated -5 deg about Y: x=cos5, z=+sin5 (right-handed, Y-up).
    QVERIFY(qAbs(outPos[0] - 0.99619f) < 0.001f);
    QVERIFY(qAbs(outPos[1] - 0.0f) < 0.001f);
    QVERIFY(qAbs(outPos[2] - 0.08716f) < 0.001f);
}

void TestNifSkinning::testRealNifSurvey()
{
    // Real-asset survey canary (§8.3): the Gamebryo 20.2.0.7 reader lands
    // scene hierarchies from shipped NIFs, and hash-style external mesh
    // paths resolve through the mesh BA2s into real vertices (name-style
    // paths match no shipped archive entry and stay vert-less). If a
    // skinned animated mesh ever loads with skin weights, the §8.3 gate
    // must be re-run against it.
    const QString meshesDir =
        qEnvironmentVariable("OPENCK_DATA_DIR",
                             QStringLiteral("C:/XboxGames/Starfield/Content/Data"))
        + QStringLiteral("/meshes");
    QDir dir(meshesDir);
    if (!dir.exists())
        QSKIP("No local meshes directory; set OPENCK_DATA_DIR");
    QStringList files;
    QDirIterator it(meshesDir, QStringList({QStringLiteral("*.nif")}),
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && files.size() < 8)
    {
        it.next();
        files.append(it.filePath());
    }
    if (files.isEmpty())
        QSKIP("No .nif files under the meshes directory");

    int loaded = 0;
    int totalVerts = 0;
    int totalMeshRefs = 0;
    int namedNodes = 0;
    for (const QString& path : files)
    {
        Nif::NifParser parser;
        if (!parser.load(path))
            continue;
        ++loaded;
        totalVerts += parser.totalVertexCount();
        totalMeshRefs += parser.externalMeshRefs().size();
        if (parser.getRoot())
        {
            for (const Nif::Node* c : parser.getRoot()->children)
                if (!c->name.isEmpty())
                    ++namedNodes;
        }
    }
    qDebug() << "survey:" << loaded << "loaded," << namedNodes
             << "named nodes," << totalMeshRefs << "external meshes,"
             << totalVerts << "verts";
    QVERIFY2(loaded == files.size(), "A shipped NIF failed the Gamebryo reader");
    QVERIFY2(totalMeshRefs > 0, "No external mesh references resolved");
    QVERIFY2(namedNodes > 0, "No scene-graph nodes extracted");
    QVERIFY2(totalVerts > 0, "No external mesh vertices resolved");
}

void TestNifSkinning::testExternalMeshData()
{
    // End of the §8.3 external-mesh chain: a shipped BSGeometry's mesh path
    // resolves into Meshes01.ba2 and the extracted .mesh stream decodes to
    // real vertices. The pair below is verified by hand (16 verts, 8 tris,
    // exact 560-byte consumption).
    const QString dataDir =
        qEnvironmentVariable("OPENCK_DATA_DIR",
                             QStringLiteral("C:/XboxGames/Starfield/Content/Data"));
    const QString archivePath = dataDir + QStringLiteral("/Starfield - Meshes01.ba2");
    if (!QFileInfo::exists(archivePath))
        QSKIP("No Meshes01.ba2; set OPENCK_DATA_DIR");

    Ba2Archive ba2;
    QVERIFY2(ba2.open(archivePath), "Meshes01.ba2 did not open");
    const QString wanted =
        QStringLiteral("geometries/00856bcea008815c2f5e/e866b4e0f724b36a7090.mesh");
    int found = -1;
    for (quint32 i = 0; i < ba2.fileCount(); ++i)
    {
        if (ba2.entries().at(i).relativePath == wanted)
        {
            found = static_cast<int>(i);
            break;
        }
    }
    QVERIFY2(found >= 0, "Known .mesh entry not in Meshes01.ba2");

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString meshPath = tmp.filePath(QStringLiteral("probe.mesh"));
    QVERIFY(ba2.extract(static_cast<quint32>(found), meshPath));

    QFile f(meshPath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray bytes = f.readAll();
    QCOMPARE(bytes.size(), 560);

    Nif::BsMeshData mesh;
    QVERIFY2(Nif::parseBsMeshData(bytes, mesh), "BSMeshData decode failed");
    QCOMPARE(mesh.vertices.size(), 16);
    QCOMPARE(mesh.triangles.size(), 24);
    QCOMPARE(mesh.uvs.size(), 16);
    QCOMPARE(mesh.normals.size(), 16);
    QCOMPARE(mesh.weights.size(), 0);
    quint32 top = 0;
    for (quint32 idx : mesh.triangles)
        top = qMax(top, idx);
    QVERIFY(top < static_cast<quint32>(mesh.vertices.size()));
    for (const Nif::Vector3& v : mesh.vertices)
        QVERIFY(std::isfinite(v.x + v.y + v.z));
}

void TestNifSkinning::testFaceSkinBlocks()
{
    // Starfield BSSkin triplets from a shipped face NIF (FaceMeshes.ba2):
    // BSGeometry + SkinAttach + BSSkin::Instance + BSSkin::BoneData.
    // The load links bone names onto shapes whose meshes resolved.
    const QString dataDir =
        qEnvironmentVariable("OPENCK_DATA_DIR",
                             QStringLiteral("C:/XboxGames/Starfield/Content/Data"));
    const QString archivePath = dataDir + QStringLiteral("/Starfield - FaceMeshes.ba2");
    if (!QFileInfo::exists(archivePath))
        QSKIP("No FaceMeshes.ba2; set OPENCK_DATA_DIR");

    Ba2Archive ba2;
    QVERIFY2(ba2.open(archivePath), "FaceMeshes.ba2 did not open");
    const QString wanted = QStringLiteral(
        "meshes/actors/character/facegendata/facegeom/starfield.esm/000124ac.nif");
    int found = -1;
    for (quint32 i = 0; i < ba2.fileCount(); ++i)
    {
        if (ba2.entries().at(i).relativePath == wanted)
        {
            found = static_cast<int>(i);
            break;
        }
    }
    QVERIFY2(found >= 0, "Known face NIF not in FaceMeshes.ba2");

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString nifPath = tmp.filePath(QStringLiteral("face.nif"));
    QVERIFY(ba2.extract(static_cast<quint32>(found), nifPath));

    Nif::NifParser parser;
    QVERIFY2(parser.load(nifPath), "Face NIF did not load");
    QVERIFY(parser.getRoot() != nullptr);

    int shapes = 0;
    int skinned = 0;
    int verts = 0;
    int namedBones = 0;
    QStack<const Nif::Node*> stack;
    stack.push(parser.getRoot());
    while (!stack.isEmpty())
    {
        const Nif::Node* node = stack.pop();
        for (const Nif::TriShape& shape : node->shapes)
        {
            ++shapes;
            verts += shape.vertices.size();
            if (!shape.skinBones.isEmpty())
            {
                ++skinned;
                for (const Nif::SkinBone& b : shape.skinBones)
                    if (!b.boneName.isEmpty())
                        ++namedBones;
            }
        }
        for (const Nif::Node* c : node->children)
            stack.push(c);
    }
    qDebug() << "face:" << shapes << "shapes," << skinned << "skinned,"
             << namedBones << "named bones," << verts << "verts";
    QCOMPARE(shapes, 10);
    QCOMPARE(skinned, 10);
    QCOMPARE(namedBones, 129);
    QCOMPARE(verts, 53444);
}

void TestNifSkinning::testSyntheticSkinnedFileLoad()
{
    QTemporaryFile file(QStringLiteral("XXXXXX.nif"));
    QVERIFY(file.open());
    const QString path = file.fileName();
    file.setAutoRemove(false);
    file.close();
    writeTestSkinnedFile(path);

    Nif::NifParser parser;
    QVERIFY(parser.load(path));
    QCOMPARE(parser.getVersionString(), QStringLiteral("20.2.0"));
    const Nif::Node* root = parser.getRoot();
    QVERIFY(root != nullptr);
    QStringList childNames;
    for (const Nif::Node* c : root->children) childNames.append(c->name);
    QCOMPARE(childNames, QStringList({QStringLiteral("Bone01"), QStringLiteral("Mesh01")}));

    const Nif::Node* mesh = findNamedChild(root, QStringLiteral("Mesh01"));
    QVERIFY(mesh != nullptr);
    QCOMPARE(mesh->shapes.size(), 1);
    const Nif::TriShape& shape = mesh->shapes[0];
    QCOMPARE(shape.vertices.size(), 2);
    QVERIFY(shape.isSkinned());
    QCOMPARE(shape.skinBones.size(), 1);
    QCOMPARE(shape.skinBones[0].boneName, QStringLiteral("Bone01"));
    QVERIFY(shape.skinBones[0].boneNode != nullptr);
    QCOMPARE(shape.skinWeights.size(), 2);
    QCOMPARE(shape.skinWeights[0].vertex, quint32(0));
    QCOMPARE(shape.skinWeights[0].bone, quint32(0));
    QCOMPARE(shape.skinWeights[0].weight, 1.0f);
    QCOMPARE(shape.skinWeights[1].vertex, quint32(1));
    QCOMPARE(shape.skinWeights[1].weight, 0.5f);

    const Nif::Node* bone = findNamedChild(root, QStringLiteral("Bone01"));
    QVERIFY(bone != nullptr);
    QCOMPARE(shape.skinBones[0].boneNode, bone);
    QFile::remove(path);
}

QTEST_MAIN(TestNifSkinning)
#include "test_nifskinning.moc"

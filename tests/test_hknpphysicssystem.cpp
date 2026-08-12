#include <QTest>
#include <QFileInfo>
#include <QDir>
#include <QFile>

#include "model/tools/hknpphysicssystem.hpp"
#include "ba2archive.hpp"
#include "logger.hpp"

// Decodes a real Starfield bhkPhysicsSystem block (hknp collision data) from a
// mesh NIF inside the game's Meshes BA2. Requires the user's Starfield install;
// not registered with CTest by default.
class TestHknpPhysicsSystem : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDecodeRealCollision();
    void testRoundTripEncodeDecode();
};

void TestHknpPhysicsSystem::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_hknp_log.txt"));
}

void TestHknpPhysicsSystem::testDecodeRealCollision()
{
    const QString archive = QStringLiteral(
        "C:/XboxGames/Starfield/Content/Data/Starfield - Meshes01.ba2");
    QVERIFY2(QFileInfo::exists(archive), "Starfield Meshes01.ba2 not found");

    Ba2Archive ba2;
    QVERIFY(ba2.open(archive));

    // Find a NIF that carries a bhkPhysicsSystem / TAG0 block.
    int nifIndex = -1;
    QByteArray nifBytes;
    for (quint32 i = 0; i < ba2.fileCount() && nifIndex < 0; ++i)
    {
        const auto& e = ba2.entries().at(i);
        if (!e.relativePath.endsWith(".nif", Qt::CaseInsensitive)) continue;
        const QString tmp = QDir::tempPath() + QStringLiteral("/openck_hknp_probe.nif");
        if (!ba2.extract(i, tmp)) continue;
        QFile f(tmp);
        if (f.open(QIODevice::ReadOnly)) nifBytes = f.readAll();
        f.close();
        QFile::remove(tmp);
        if (nifBytes.contains("bhkPhysicsSystem") && nifBytes.contains("TAG0"))
            nifIndex = static_cast<int>(i);
    }
    QVERIFY2(nifIndex >= 0, "no collision NIF found");
    qDebug() << "decoding collision from NIF index" << nifIndex << "size" << nifBytes.size();

    // The bhkPhysicsSystem block is: u32 data_length + TAG0 chunk stream.
    // Locate the TAG0 fourcc and back up over the chunk header + length.
    const int tag0 = nifBytes.indexOf("TAG0");
    QVERIFY(tag0 >= 8);
    const int blockStart = tag0 - 8;
    const QByteArray block = nifBytes.mid(blockStart);

    const HknpPhysicsSystem sys = HknpPhysicsSystem::read(block);
    QVERIFY(sys.ok);
    QVERIFY(sys.dataLength > 0);
    QVERIFY(!sys.sdkvVersion.isEmpty());
    qDebug() << "sdkv:" << sys.sdkvVersion.trimmed()
             << "chunks:" << sys.chunks.size()
             << "items:" << sys.items.size()
             << "patches:" << sys.patches.size();
    QVERIFY(sys.chunks.size() >= 4);

    bool sawData = false, sawType = false, sawIndx = false;
    for (const auto& c : sys.chunks)
    {
        if (c.fourcc == "DATA") { sawData = true; QVERIFY(!c.body.isEmpty()); }
        else if (c.fourcc == "TYPE") sawType = true;
        else if (c.fourcc == "INDX") sawIndx = true;
    }
    QVERIFY(sawData && sawType && sawIndx);
    QVERIFY(!sys.items.isEmpty());

    qDebug() << "polytope (best-effort): verts=" << sys.vertices.size()
             << "planes=" << sys.planes.size()
             << "faces=" << sys.faces.size()
             << "indices=" << sys.indices.size();
    // The chunk container (TAG0 -> SDKV/DATA/TYPE/INDX -> ITEM/PTCH) is what is
    // decoded; the per-shape payload (hknpConvexShape etc.) is shape-type
    // specific and only populated when the hkRelArray item indices form a
    // valid convex-hull pattern.
    if (sys.vertices.size() >= 3 && !sys.planes.isEmpty())
    {
        QCOMPARE(sys.planes.at(0).size(), 4);
        qDebug() << "first vertex:" << sys.vertices.at(0).at(0)
                 << sys.vertices.at(0).at(1) << sys.vertices.at(0).at(2);
    }
}

void TestHknpPhysicsSystem::testRoundTripEncodeDecode()
{
    // Synthesize a minimal tetrahedron-shaped convex descriptor entirely in
    // memory (no Bethesda data) and check encode() -> read() preserves it.
    HknpPhysicsSystem::ConvexShapeData shape;
    shape.vertices = { { 0.0f, 0.0f, 0.0f },
                       { 1.0f, 0.0f, 0.0f },
                       { 0.0f, 1.0f, 0.0f },
                       { 0.0f, 0.0f, 1.0f } };
    shape.planes = { { 1.0f, 0.0f, 0.0f, 0.0f },
                     { 0.0f, 1.0f, 0.0f, 0.0f },
                     { 0.0f, 0.0f, 1.0f, 0.0f },
                     { 1.0f, 1.0f, 1.0f, -1.0f } };
    // Packed (firstIndex | numIndices<<16 | minHalfAngle<<24): four triangle
    // faces of three indices starting at 0, 3, 6, 9.
    for (quint32 k = 0; k < 4; ++k)
        shape.faces.append(k * 3 | (3u << 16));
    shape.indices = { 0, 1, 2, 0, 2, 3, 0, 3, 1, 1, 2, 3 };
    for (quint32 k = 0; k < 6; ++k)
        shape.faceLinks.append(k & 0xFFFFu);
    for (quint32 k = 0; k < 4; ++k)
        shape.vertexEdges.append(k & 0xFFFFu);
    shape.itemTypeIdx = { 8, 9, 10, 11, 12, 13 };

    QVector<HknpPhysicsSystem::Patch> patches;
    patches.append({ -1, { 0x0C, 0x100 } });
    patches.append(HknpPhysicsSystem::Patch{ 2, QVector<quint32>{ 0 } });

    const QString version = QStringLiteral("1.7.12.5");
    const QByteArray typeBody = QByteArray("\x01\x00\x00\x00TYPE-PASSTHROUGH", 20);

    const QByteArray block =
        HknpPhysicsSystem::encode(version, typeBody, shape, patches);
    const HknpPhysicsSystem sys = HknpPhysicsSystem::read(block);
    QVERIFY(sys.ok);
    QCOMPARE(sys.dataLength, static_cast<quint32>(block.size() - 4));
    QCOMPARE(sys.sdkvVersion, version);
    QCOMPARE(sys.typeBody, typeBody);
    QCOMPARE(sys.chunks.size(), 4);
    QCOMPARE(sys.chunks.at(0).fourcc, QByteArray("SDKV"));
    QCOMPARE(sys.chunks.at(1).fourcc, QByteArray("DATA"));
    QCOMPARE(sys.chunks.at(2).fourcc, QByteArray("TYPE"));
    QCOMPARE(sys.chunks.at(3).fourcc, QByteArray("INDX"));

    QVERIFY(sys.vertices == shape.vertices);
    QVERIFY(sys.planes == shape.planes);
    QVERIFY(sys.faces == shape.faces);
    QVERIFY(sys.indices == shape.indices);
    QVERIFY(sys.faceLinks == shape.faceLinks);
    QVERIFY(sys.vertexEdges == shape.vertexEdges);

    QCOMPARE(sys.items.size(), 6);
    for (int k = 0; k < 6; ++k)
    {
        QCOMPARE(sys.items.at(k).typeIdx, shape.itemTypeIdx.value(k));
        switch (k)
        {
        case 0: QCOMPARE(sys.items.at(k).count, quint32(shape.vertices.size())); break;
        case 1: QCOMPARE(sys.items.at(k).count, quint32(shape.planes.size())); break;
        case 2: QCOMPARE(sys.items.at(k).count, quint32(shape.faces.size())); break;
        case 3: QCOMPARE(sys.items.at(k).count, quint32(shape.indices.size())); break;
        case 4: QCOMPARE(sys.items.at(k).count, quint32(shape.faceLinks.size())); break;
        case 5: QCOMPARE(sys.items.at(k).count, quint32(shape.vertexEdges.size())); break;
        }
    }

    QCOMPARE(sys.patches.size(), patches.size());
    for (int k = 0; k < patches.size(); ++k)
    {
        QCOMPARE(sys.patches.at(k).typeIdx, patches.at(k).typeIdx);
        QCOMPARE(sys.patches.at(k).offsets, patches.at(k).offsets);
    }

    qDebug() << "round-trip ok: block" << block.size()
             << "bytes, verts" << sys.vertices.size()
             << "faces" << sys.faces.size();
}

QTEST_MAIN(TestHknpPhysicsSystem)
#include "test_hknpphysicssystem.moc"

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

QTEST_MAIN(TestHknpPhysicsSystem)
#include "test_hknpphysicssystem.moc"

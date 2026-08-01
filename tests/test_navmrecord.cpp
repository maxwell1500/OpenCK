#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QVector3D>

#include "../../libs/files/esm/navmrecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/log/logger.hpp"

class TestNavmRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRoundTrip();
};

void TestNavmRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_navmrecord_log.txt"));
}

void TestNavmRecord::testRoundTrip()
{
    NavmRecord rec;
    rec.editorId = QStringLiteral("NavMeshTest");
    rec.formId = 0x12345;
    rec.flags = 1;
    rec.cellFormId = 0xABCD;
    rec.vertices = { QVector3D(0, 0, 0), QVector3D(10, 0, 0), QVector3D(0, 10, 0), QVector3D(0, 0, 10) };

    NavmTriangle t0;
    t0.v0 = 0; t0.v1 = 1; t0.v2 = 2;
    t0.edge0 = -1; t0.edge1 = -1; t0.edge2 = -1;
    t0.flags = 1;
    rec.triangles.append(t0);

    NavmTriangle t1;
    t1.v0 = 0; t1.v1 = 2; t1.v2 = 3;
    t1.edge0 = -1; t1.edge1 = -1; t1.edge2 = -1;
    t1.flags = 1;
    rec.triangles.append(t1);

    rec.externalConnections = { 0x1111, 0x2222 };

    QTemporaryFile tmpFile;
    tmpFile.open();
    QString path = tmpFile.fileName();
    tmpFile.close();

    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("Test");
        writer.save(file);
        RecHeader recHeader;
        recHeader.id = 0x12345;
        writer.startRecord('NAVM', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('NAVM'));
        NavmRecord loaded;
        loaded.load(reader, true);

        QVERIFY(loaded.editorId.startsWith("NavMeshTest"));
        QCOMPARE(loaded.formId, static_cast<quint32>(0x12345));
        QCOMPARE(loaded.cellFormId, static_cast<quint32>(0xABCD));
        QCOMPARE(loaded.vertices.size(), 4);
        if (loaded.vertices.size() == 4)
        {
            QCOMPARE(loaded.vertices[0], QVector3D(0, 0, 0));
            QCOMPARE(loaded.vertices[2], QVector3D(0, 10, 0));
        }
        QCOMPARE(loaded.triangles.size(), 2);
        if (loaded.triangles.size() == 2)
        {
            QCOMPARE(loaded.triangles[0].v0, (qint16)0);
            QCOMPARE(loaded.triangles[0].v1, (qint16)1);
            QCOMPARE(loaded.triangles[0].v2, (qint16)2);
            QCOMPARE(loaded.triangles[0].flags, (quint16)1);
            QCOMPARE(loaded.triangles[1].v2, (qint16)3);
        }
        QCOMPARE(loaded.externalConnections, QVector<quint32>({ 0x1111, 0x2222 }));
    }
}

QTEST_MAIN(TestNavmRecord)
#include "test_navmrecord.moc"

#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../libs/files/esm/locationrecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/log/logger.hpp"

class TestLocationRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRoundTripLinkedRefs();
    void testNoLinkedRefs();
};

void TestLocationRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_locationrecord_log.txt"));
}

void TestLocationRecord::testRoundTripLinkedRefs()
{
    LocationRecord rec;
    rec.editorId = QStringLiteral("TestLocation");
    rec.formId = 0x33445;
    rec.flags = 2;
    rec.locationName = QStringLiteral("Test Place");
    rec.parentId = 0x10000;
    rec.x = 10;
    rec.y = 20;
    rec.z = 30;

    LocationRecord::LinkedRef group;
    group.refTypeId = 0x20001;  // LCRT (LocationRefType) form ID
    group.linkedIds = { 0x30001, 0x30002 };
    rec.linkedRefs.append(group);

    LocationRecord::LinkedRef second;
    second.refTypeId = 0x20002;
    second.linkedIds = { 0x30003 };
    rec.linkedRefs.append(second);

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
        recHeader.id = 0x33445;
        writer.startRecord('LCTN', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('LCTN'));
        LocationRecord loaded;
        loaded.load(reader, true);

        QVERIFY(loaded.editorId.startsWith(QStringLiteral("TestLocation")));
        QCOMPARE(loaded.formId, static_cast<quint32>(0x33445));
        QCOMPARE(loaded.flags, static_cast<quint32>(2));
        QCOMPARE(loaded.parentId, static_cast<quint32>(0x10000));
        QCOMPARE(loaded.x, static_cast<quint32>(10));
        QCOMPARE(loaded.y, static_cast<quint32>(20));
        QCOMPARE(loaded.z, static_cast<quint32>(30));

        QCOMPARE(loaded.linkedRefs.size(), 2);
        QCOMPARE(loaded.linkedRefs[0].refTypeId, static_cast<quint32>(0x20001));
        QCOMPARE(loaded.linkedRefs[0].linkedIds.size(), 2);
        QCOMPARE(loaded.linkedRefs[0].linkedIds[0], static_cast<quint32>(0x30001));
        QCOMPARE(loaded.linkedRefs[0].linkedIds[1], static_cast<quint32>(0x30002));
        QCOMPARE(loaded.linkedRefs[1].refTypeId, static_cast<quint32>(0x20002));
        QCOMPARE(loaded.linkedRefs[1].linkedIds.size(), 1);
        QCOMPARE(loaded.linkedRefs[1].linkedIds[0], static_cast<quint32>(0x30003));
    }
}

void TestLocationRecord::testNoLinkedRefs()
{
    LocationRecord rec;
    rec.editorId = QStringLiteral("Plain");
    rec.formId = 0x77;
    rec.flags = 0;
    rec.parentId = 0;

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
        recHeader.id = 0x77;
        writer.startRecord('LCTN', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        reader.readName();
        LocationRecord loaded;
        loaded.load(reader, true);
        QVERIFY(loaded.editorId.startsWith(QStringLiteral("Plain")));
        QVERIFY(loaded.linkedRefs.isEmpty());
    }
}

QTEST_MAIN(TestLocationRecord)
#include "test_locationrecord.moc"

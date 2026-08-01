#include <QTest>
#include <QFile>
#include <QTemporaryFile>

#include "../../libs/files/esm/cellrecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/log/logger.hpp"

class TestCellRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testXclwRoundTrip();
    void testNoWaterRoundTrip();
    void testBlank();
};

void TestCellRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_cellrecord_log.txt"));
}

void TestCellRecord::testXclwRoundTrip()
{
    CellRecord rec;
    rec.editorId = QStringLiteral("CellWaterTest");
    rec.formId = 0x2000A;
    rec.flags = 1;
    rec.cellX = 5;
    rec.cellY = -3;
    rec.owner = 0x1234;
    rec.lockLevel = 0;
    rec.cellName = QStringLiteral("Water Cell");
    rec.hasWaterHeight = true;
    rec.waterHeight = 512.5f;

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
        recHeader.id = 0x2000A;
        writer.startRecord('CELL', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('CELL'));
        CellRecord loaded;
        loaded.load(reader, true);

        QVERIFY(loaded.editorId.startsWith("CellWaterTest"));
        QCOMPARE(loaded.formId, static_cast<quint32>(0x2000A));
        QCOMPARE(loaded.flags, static_cast<quint8>(1));
        QCOMPARE(loaded.cellX, static_cast<quint32>(5));
        QCOMPARE(loaded.cellY, static_cast<quint32>(0xFFFFFFFD)); // -3 as uint32
        QCOMPARE(loaded.owner, static_cast<quint32>(0x1234));
        QVERIFY(loaded.cellName.startsWith("Water Cell"));
        QVERIFY(loaded.hasWaterHeight);
        QVERIFY(qFuzzyCompare(loaded.waterHeight, 512.5f));
    }
}

void TestCellRecord::testNoWaterRoundTrip()
{
    CellRecord rec;
    rec.editorId = QStringLiteral("DryCell");
    rec.formId = 0x2000B;
    rec.flags = 0;
    rec.cellX = 0;
    rec.cellY = 0;
    rec.cellName = QStringLiteral("No Water");

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
        recHeader.id = 0x2000B;
        writer.startRecord('CELL', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('CELL'));
        CellRecord loaded;
        loaded.load(reader, true);

        QVERIFY(!loaded.hasWaterHeight);
        QCOMPARE(loaded.waterHeight, 0.0f);
    }
}

void TestCellRecord::testBlank()
{
    CellRecord rec;
    rec.editorId = QStringLiteral("Old");
    rec.formId = 0x2000C;
    rec.flags = 3;
    rec.cellName = QStringLiteral("Old");
    rec.hasWaterHeight = true;
    rec.waterHeight = 100.0f;

    rec.blank();

    QVERIFY(rec.editorId.isEmpty());
    QCOMPARE(rec.formId, static_cast<quint32>(0));
    QCOMPARE(rec.flags, static_cast<quint8>(0));
    QVERIFY(!rec.hasWaterHeight);
    QCOMPARE(rec.waterHeight, 0.0f);
}

QTEST_MAIN(TestCellRecord)
#include "test_cellrecord.moc"

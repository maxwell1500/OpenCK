#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <cstring>

#include "../../libs/files/esm/effectshaderrecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/log/logger.hpp"

class TestEfshRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDataRoundTrip();
    void testNoData();
};

void TestEfshRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_efshrecord_log.txt"));
}

void TestEfshRecord::testDataRoundTrip()
{
    EfshRecord rec;
    rec.editorId = QStringLiteral("TestEFSH");
    rec.formId = 0x22334;
    rec.flags = 0;
    rec.data.present = true;
    rec.data.shaderFlags = 0x0F0F0F0F;
    rec.data.fillR = 10; rec.data.fillG = 20; rec.data.fillB = 30; rec.data.fillA = 255;
    rec.data.rimR = 200; rec.data.rimG = 100; rec.data.rimB = 50;  rec.data.rimA = 128;
    rec.data.baseR = 1;  rec.data.baseG = 2;  rec.data.baseB = 3;  rec.data.baseA = 4;
    rec.data.fillScale = 0.5f;
    rec.data.rimScale = 1.5f;
    rec.data.baseScale = 2.0f;
    rec.data.unk1 = 0xDEAD;
    rec.data.unk2 = 0xBEEF;

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
        recHeader.id = 0x22334;
        writer.startRecord('EFSH', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('EFSH'));
        EfshRecord loaded;
        loaded.load(reader, true);

        QVERIFY(loaded.editorId.startsWith(QStringLiteral("TestEFSH")));
        QCOMPARE(loaded.formId, static_cast<quint32>(0x22334));
        QVERIFY(loaded.data.present);
        QCOMPARE(loaded.data.shaderFlags, static_cast<quint32>(0x0F0F0F0F));
        QCOMPARE(loaded.data.fillR, 10);
        QCOMPARE(loaded.data.fillG, 20);
        QCOMPARE(loaded.data.fillB, 30);
        QCOMPARE(loaded.data.fillA, 255);
        QCOMPARE(loaded.data.rimR, 200);
        QCOMPARE(loaded.data.rimG, 100);
        QCOMPARE(loaded.data.rimB, 50);
        QCOMPARE(loaded.data.rimA, 128);
        QCOMPARE(loaded.data.baseR, 1);
        QCOMPARE(loaded.data.baseG, 2);
        QCOMPARE(loaded.data.baseB, 3);
        QCOMPARE(loaded.data.baseA, 4);
        QVERIFY(qFuzzyCompare(loaded.data.fillScale, 0.5f));
        QVERIFY(qFuzzyCompare(loaded.data.rimScale, 1.5f));
        QVERIFY(qFuzzyCompare(loaded.data.baseScale, 2.0f));
        QCOMPARE(loaded.data.unk1, static_cast<quint32>(0xDEAD));
        QCOMPARE(loaded.data.unk2, static_cast<quint32>(0xBEEF));
        QVERIFY(loaded.rawSubRecords.isEmpty());
    }
}

void TestEfshRecord::testNoData()
{
    EfshRecord rec;
    rec.editorId = QStringLiteral("NoDataEFSH");
    rec.formId = 0x99;
    rec.data.present = false;

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
        recHeader.id = 0x99;
        writer.startRecord('EFSH', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        reader.readName();
        EfshRecord loaded;
        loaded.load(reader, true);
        QVERIFY(loaded.editorId.startsWith(QStringLiteral("NoDataEFSH")));
        QVERIFY(!loaded.data.present);
    }
}

QTEST_MAIN(TestEfshRecord)
#include "test_efshrecord.moc"

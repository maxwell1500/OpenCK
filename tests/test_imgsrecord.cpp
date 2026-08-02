#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <cstring>

#include "../../libs/files/esm/imagespacerecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/log/logger.hpp"

class TestImgsRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDataRoundTrip();
    void testShortData();
    void testNoData();
};

void TestImgsRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_imgsrecord_log.txt"));
}

void TestImgsRecord::testDataRoundTrip()
{
    ImgsRecord rec;
    rec.editorId = QStringLiteral("TestIMGS");
    rec.formId = 0x112233;
    rec.flags = 0;
    rec.data.present = true;
    for (int i = 0; i < 48; i++)
        rec.data.values[i] = static_cast<float>(i) * 0.5f;
    for (int c = 0; c < 6; c++)
    {
        rec.data.color[c][0] = static_cast<quint8>(c * 10);
        rec.data.color[c][1] = static_cast<quint8>(c * 20);
        rec.data.color[c][2] = static_cast<quint8>(c * 30);
        rec.data.color[c][3] = static_cast<quint8>(c * 40);
    }
    rec.data.trailingBytes = QByteArray::fromHex("DEADBEEF");

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
        recHeader.id = 0x112233;
        writer.startRecord('IMGS', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('IMGS'));
        ImgsRecord loaded;
        loaded.load(reader, true);

        QVERIFY(loaded.editorId.startsWith(QStringLiteral("TestIMGS")));
        QCOMPARE(loaded.formId, static_cast<quint32>(0x112233));
        QVERIFY(loaded.data.present);
        for (int i = 0; i < 48; i++)
            QVERIFY(qFuzzyCompare(loaded.data.values[i], static_cast<float>(i) * 0.5f));
        for (int c = 0; c < 6; c++)
        {
            QCOMPARE(loaded.data.color[c][0], static_cast<quint8>(c * 10));
            QCOMPARE(loaded.data.color[c][1], static_cast<quint8>(c * 20));
            QCOMPARE(loaded.data.color[c][2], static_cast<quint8>(c * 30));
            QCOMPARE(loaded.data.color[c][3], static_cast<quint8>(c * 40));
        }
        QCOMPARE(loaded.data.trailingBytes.toHex(), QByteArray("deadbeef"));
        QVERIFY(loaded.rawSubRecords.isEmpty());
    }
}

void TestImgsRecord::testShortData()
{
    ImgsRecord rec;
    rec.editorId = QStringLiteral("ShortIMGS");
    rec.formId = 0x77;
    rec.data.present = true;
    rec.data.values[0] = 1.0f;
    rec.data.values[1] = 2.0f;

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
        writer.startRecord('IMGS', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        reader.readName();
        ImgsRecord loaded;
        loaded.load(reader, true);
        QVERIFY(loaded.editorId.startsWith(QStringLiteral("ShortIMGS")));
        QVERIFY(loaded.data.present);
        QVERIFY(qFuzzyCompare(loaded.data.values[0], 1.0f));
        QVERIFY(qFuzzyCompare(loaded.data.values[1], 2.0f));
        // Colors beyond the written payload stay zero.
        for (int c = 0; c < 6; c++)
            QCOMPARE(loaded.data.color[c][3], 0);
    }
}

void TestImgsRecord::testNoData()
{
    ImgsRecord rec;
    rec.editorId = QStringLiteral("NoDataIMGS");
    rec.formId = 0x42;
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
        recHeader.id = 0x42;
        writer.startRecord('IMGS', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        reader.readName();
        ImgsRecord loaded;
        loaded.load(reader, true);
        QVERIFY(loaded.editorId.startsWith(QStringLiteral("NoDataIMGS")));
        QVERIFY(!loaded.data.present);
    }
}

QTEST_MAIN(TestImgsRecord)
#include "test_imgsrecord.moc"

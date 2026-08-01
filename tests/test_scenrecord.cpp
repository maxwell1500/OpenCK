#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../libs/files/esm/scenrecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/log/logger.hpp"

class TestScenRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRoundTrip();
};

void TestScenRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_scenrecord_log.txt"));
}

void TestScenRecord::testRoundTrip()
{
    ScenRecord rec;
    rec.editorId = QStringLiteral("Quest1Scene");

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
        recHeader.id = 0x30001;
        writer.startRecord('SCEN', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('SCEN'));
        ScenRecord loaded;
        loaded.load(reader, true);

        QVERIFY(loaded.editorId.startsWith("Quest1Scene"));
        QCOMPARE(loaded.formId, static_cast<quint32>(0x30001));
    }
}

QTEST_MAIN(TestScenRecord)
#include "test_scenrecord.moc"

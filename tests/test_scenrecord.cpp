#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../libs/files/esm/scenrecord.hpp"
#include "../../libs/files/esm/conditionrecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/log/logger.hpp"

class TestScenRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRoundTrip();
    void testConditionsRoundTrip();
};

void TestScenRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_scenrecord_log.txt"));
}

void TestScenRecord::testConditionsRoundTrip()
{
    ScenRecord rec;
    rec.editorId = QStringLiteral("Quest1Scene");
    rec.formId = 0x9876;

    CtdaCondition cond;
    cond.functionId = 0x2A;
    cond.param1 = 55;
    cond.comparison = CtdaCondition::Comparison::EqualTo;
    cond.runOn = CtdaCondition::RunOn::Reference;
    cond.reference = 0x1234;
    rec.conditions.append(cond);

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
        recHeader.id = 0x9876;
        writer.startRecord('SCEN', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        reader.readName();
        ScenRecord loaded;
        loaded.load(reader, true);
        QVERIFY(loaded.editorId.startsWith(QStringLiteral("Quest1Scene")));
        QCOMPARE(loaded.conditions.size(), 1);
        QCOMPARE(loaded.conditions[0].functionId, static_cast<quint32>(0x2A));
        QCOMPARE(loaded.conditions[0].param1, static_cast<quint32>(55));
        QCOMPARE(loaded.conditions[0].comparison, CtdaCondition::Comparison::EqualTo);
        QCOMPARE(loaded.conditions[0].runOn, CtdaCondition::RunOn::Reference);
        QCOMPARE(loaded.conditions[0].reference, static_cast<quint32>(0x1234));
    }
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

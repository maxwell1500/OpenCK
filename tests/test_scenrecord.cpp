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
    void testPhdaRawRoundTrip();
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

void TestScenRecord::testPhdaRawRoundTrip()
{
    // PHDA (phase data) is opaque binary: SCEN stores it in rawSubRecords and
    // must preserve it byte-for-byte across a save/load cycle.
    //
    // The bytes below are ARBITRARY. They are not a real PHDA and the offsets
    // carry no known meaning — an earlier version of this test annotated them as
    // "phase count" and "per-phase flags", which was invented and could mislead
    // someone into building a phase editor on top of it. A byte scan of
    // Skyrim.esm, Dawnguard.esm, Dragonborn.esm and HearthFires.esm (329 MB,
    // 9,143 SCEN records) found ZERO PHDA subrecords, so no real phase data
    // exists on this machine to validate a layout against.
    //
    // This test therefore proves exactly one thing: opaque subrecords survive a
    // save/load cycle untouched. It proves nothing about PHDA's structure.
    ScenRecord rec;
    rec.editorId = QStringLiteral("SceneWithPhases");
    rec.formId = 0x30002;

    QByteArray phda(24, 0);
    phda[0] = static_cast<char>(0x01);
    phda[4] = static_cast<char>(0x10);
    phda[8] = static_cast<char>(0x40);
    phda[12] = static_cast<char>(0x01);
    phda[16] = static_cast<char>(0xAB);
    phda[20] = static_cast<char>(0xCD);

    RawSubRecord raw;
    raw.name = NAME('PHDA');
    raw.data = phda;
    rec.rawSubRecords.push_back(raw);

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
        recHeader.id = 0x30002;
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

        QCOMPARE(loaded.rawSubRecords.size(), 1);
        QCOMPARE(loaded.rawSubRecords[0].name, static_cast<quint32>('PHDA'));
        QCOMPARE(loaded.rawSubRecords[0].data, phda);
    }
}

QTEST_MAIN(TestScenRecord)
#include "test_scenrecord.moc"

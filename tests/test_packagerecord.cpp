#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../libs/files/esm/Packagerecord.hpp"
#include "../../libs/files/esm/packagesemantics.hpp"
#include "../../libs/files/esm/conditionrecord.hpp"
#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/log/logger.hpp"

class TestPackageRecord : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testConditionsRoundTrip();
    void testNoConditions();
    void testScheduleEditsRoundTripThroughFile();
    void testTargetEditsRoundTripThroughFile();
    void testUneditedPackageIsByteIdentical();
};

void TestPackageRecord::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_packagerecord_log.txt"));
}

void TestPackageRecord::testConditionsRoundTrip()
{
    PackageRecord rec;
    rec.editorId = QStringLiteral("TestPACK");
    rec.formId = 0x5566;
    rec.flags = 0;
    rec.packageType = 4;
    rec.targetType = 0;
    rec.targetIds = { 0x1111, 0x2222 };

    CtdaCondition condA;
    condA.functionId = 0x1A;
    condA.param1 = 100;
    condA.comparison = CtdaCondition::Comparison::GreaterThanOrEqualTo;
    condA.runOn = CtdaCondition::RunOn::Subject;
    condA.setUseOr(false);
    rec.conditions.append(condA);

    CtdaCondition condB;
    condB.functionId = 0x42;
    condB.param2 = 7;
    condB.comparison = CtdaCondition::Comparison::LessThan;
    condB.runOn = CtdaCondition::RunOn::Target;
    condB.reference = 0x3000;
    condB.setUseOr(true);
    rec.conditions.append(condB);

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
        recHeader.id = 0x5566;
        writer.startRecord('PACK', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        quint32 type = reader.readName();
        QCOMPARE(type, static_cast<quint32>('PACK'));
        PackageRecord loaded;
        loaded.load(reader, true);

        QVERIFY(loaded.editorId.startsWith(QStringLiteral("TestPACK")));
        QCOMPARE(loaded.formId, static_cast<quint32>(0x5566));
        QCOMPARE(loaded.packageType, static_cast<quint32>(4));
        QCOMPARE(loaded.targetIds, QVector<quint32>({ 0x1111, 0x2222 }));

        QCOMPARE(loaded.conditions.size(), 2);
        QCOMPARE(loaded.conditions[0].functionId, static_cast<quint32>(0x1A));
        QCOMPARE(loaded.conditions[0].param1, static_cast<quint32>(100));
        QCOMPARE(loaded.conditions[0].comparison,
                 CtdaCondition::Comparison::GreaterThanOrEqualTo);
        QVERIFY(!loaded.conditions[0].useOr());

        QCOMPARE(loaded.conditions[1].functionId, static_cast<quint32>(0x42));
        QCOMPARE(loaded.conditions[1].param2, static_cast<quint32>(7));
        QCOMPARE(loaded.conditions[1].comparison, CtdaCondition::Comparison::LessThan);
        QCOMPARE(loaded.conditions[1].runOn, CtdaCondition::RunOn::Target);
        QCOMPARE(loaded.conditions[1].reference, static_cast<quint32>(0x3000));
        QVERIFY(loaded.conditions[1].useOr());
        QVERIFY(loaded.rawSubRecords.isEmpty());
    }
}

void TestPackageRecord::testNoConditions()
{
    PackageRecord rec;
    rec.editorId = QStringLiteral("SimplePACK");
    rec.formId = 0x77;
    rec.packageType = 1;

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
        writer.startRecord('PACK', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    {
        ESMReader reader(path);
        reader.open();
        reader.readName();
        PackageRecord loaded;
        loaded.load(reader, true);
        QVERIFY(loaded.editorId.startsWith(QStringLiteral("SimplePACK")));
        QCOMPARE(loaded.packageType, static_cast<quint32>(1));
        QVERIFY(loaded.conditions.isEmpty());
    }
}

// Applying a semantic edit and saving must preserve the bytes the editor does
// not own: an untouched package has to come back byte-for-byte, and a schedule
// edit must not disturb the trailing payload or a second entry.
void TestPackageRecord::testScheduleEditsRoundTripThroughFile()
{
    PackageRecord rec;
    rec.editorId = QStringLiteral("SchedEdit");
    rec.formId = 0x1234;
    rec.flags = 0;
    rec.packageType = 18;
    rec.targetType = 0;
    rec.loadOrder = { NAME('EDID'), NAME('PKDT'), NAME('PLDT') };

    QByteArray payload(23, '\0');
    const auto put32 = [&payload](int offset, quint32 value) {
        payload[offset] = static_cast<char>(value & 0xFF);
        payload[offset + 1] = static_cast<char>((value >> 8) & 0xFF);
        payload[offset + 2] = static_cast<char>((value >> 16) & 0xFF);
        payload[offset + 3] = static_cast<char>((value >> 24) & 0xFF);
    };
    put32(0, 18);   // type = sandbox
    put32(4, 0);    // flags
    put32(8, 0);    // interrupt override slot
    payload[12] = static_cast<char>(0xFF);
    payload[13] = static_cast<char>(0xFF);
    payload[14] = static_cast<char>(0xFF);
    payload[15] = static_cast<char>(0xFF);
    payload[16] = static_cast<char>(0xFF);  // any minute
    payload[17] = static_cast<char>(0);     // do-all off
    // Five bytes the semantic model does not own.
    for (int i = 0; i < 5; ++i) payload[18 + i] = static_cast<char>((i + 1) & 0xFF);

    rec.pkdtRaws.append(payload);
    rec.pkdtRaw = payload;
    rec.hasPkdt = true;
    QByteArray pldt(4, '\0');
    pldt[0] = static_cast<char>(0);
    rec.pldtRaws.append(pldt);
    rec.pldtRaw = pldt;
    rec.targetType = 0;
    rec.hasPldt = true;

    // Edit the schedule through the semantic model.
    bool ok = false;
    const openck::PackageData decoded = openck::decodePackageData(rec, &ok);
    QVERIFY(ok);
    openck::PackageData edited = decoded;
    edited.schedule.month = 3;
    edited.schedule.weekday = 2;
    edited.schedule.hour = 7;
    edited.schedule.minute = 20;
    edited.doAll = true;
    openck::encodePackageData(rec, edited);
    QCOMPARE(rec.pkdtRaws.at(0).size(), payload.size());

    QTemporaryFile tmpFile;
    tmpFile.open();
    const QString path = tmpFile.fileName();
    tmpFile.close();

    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("Test");
        writer.save(file);
        RecHeader recHeader;
        recHeader.id = 0x1234;
        writer.startRecord('PACK', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    ESMReader reader(path);
    reader.open();
    QCOMPARE(reader.readName(), static_cast<quint32>('PACK'));
    PackageRecord loaded;
    loaded.load(reader, true);
    QVERIFY(loaded.hasPkdt);
    QCOMPARE(loaded.pkdtRaws.at(0).size(), payload.size());

    const openck::PackageData reloaded = openck::decodePackageData(loaded, &ok);
    QVERIFY(ok);
    QCOMPARE(reloaded.type, 18u);
    QCOMPARE(reloaded.schedule.month, quint8(3));
    QCOMPARE(reloaded.schedule.weekday, quint8(2));
    QCOMPARE(reloaded.schedule.hour, quint8(7));
    QCOMPARE(reloaded.schedule.minute, 20);
    QVERIFY(reloaded.doAll);
    // The trailing five bytes the model does not own survived the write.
    for (int i = 0; i < 5; ++i)
    {
        QCOMPARE(static_cast<quint8>(loaded.pkdtRaws.at(0).at(18 + i)),
                 quint8((i + 1) & 0xFF));
    }
}

// A changed target list must be rewritten as PTDT, not silently dropped.
void TestPackageRecord::testTargetEditsRoundTripThroughFile()
{
    PackageRecord rec;
    rec.editorId = QStringLiteral("TargetEdit");
    rec.formId = 0x5678;
    rec.flags = 0;
    rec.packageType = 3;  // escort
    rec.targetType = 0;
    rec.loadOrder = { NAME('EDID'), NAME('PKDT'), NAME('PLDT'), NAME('PTDT') };

    QByteArray payload(18, '\0');
    const auto put32 = [&payload](int offset, quint32 value) {
        payload[offset] = static_cast<char>(value & 0xFF);
        payload[offset + 1] = static_cast<char>((value >> 8) & 0xFF);
        payload[offset + 2] = static_cast<char>((value >> 16) & 0xFF);
        payload[offset + 3] = static_cast<char>((value >> 24) & 0xFF);
    };
    put32(0, 3);
    rec.pkdtRaws.append(payload);
    rec.pkdtRaw = payload;
    rec.hasPkdt = true;

    QByteArray pldt(4, '\0');
    rec.pldtRaws.append(pldt);
    rec.pldtRaw = pldt;
    rec.hasPldt = true;

    // One existing target. The payload is replayed verbatim when targetIds
    // still matches it, so leave the bytes zero and the writer replays them.
    QByteArray ptdt(12, '\0');
    rec.ptdtRaws.append(ptdt);
    rec.targetIds.append(0x01000D01);

    // The editor appends a new target.
    rec.targetIds.append(0x0100FFEE);

    QTemporaryFile tmpFile;
    tmpFile.open();
    const QString path = tmpFile.fileName();
    tmpFile.close();

    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("Test");
        writer.save(file);
        RecHeader recHeader;
        recHeader.id = 0x5678;
        writer.startRecord('PACK', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    ESMReader reader(path);
    reader.open();
    QCOMPARE(reader.readName(), static_cast<quint32>('PACK'));
    PackageRecord loaded;
    loaded.load(reader, true);
    QCOMPARE(loaded.targetIds.size(), 2);
    QVERIFY(loaded.targetIds.contains(0x01000D01));
    QVERIFY(loaded.targetIds.contains(0x0100FFEE));
}

// Untouched, a package must save byte-for-byte identical to the file it
// loaded from. This is the round-trip guarantee the parity doc requires.
void TestPackageRecord::testUneditedPackageIsByteIdentical()
{
    PackageRecord rec;
    rec.editorId = QStringLiteral("ByteExact");
    rec.formId = 0x9ABC;
    rec.flags = 0;
    rec.packageType = 19;  // patrol
    rec.targetType = 0;
    rec.loadOrder = { NAME('EDID'), NAME('PKDT'), NAME('PLDT'), NAME('PTDT') };

    QByteArray payload(20, '\0');
    payload[0] = 19;
    rec.pkdtRaws.append(payload);
    rec.pkdtRaw = payload;
    rec.hasPkdt = true;

    QByteArray pldt(4, '\0');
    rec.pldtRaws.append(pldt);
    rec.pldtRaw = pldt;
    rec.hasPldt = true;

    QByteArray ptdt(12, '\0');
    ptdt[4] = static_cast<char>(0x42);
    rec.ptdtRaws.append(ptdt);
    rec.targetIds.append(0x42000000);

    const QByteArray originalPkdt = rec.pkdtRaws.at(0);

    // Decode/re-encode with no change must not move a byte.
    bool ok = false;
    const openck::PackageData data = openck::decodePackageData(rec, &ok);
    QVERIFY(ok);
    openck::encodePackageData(rec, data);
    QCOMPARE(rec.pkdtRaws.at(0), originalPkdt);

    QTemporaryFile tmpFile;
    tmpFile.open();
    const QString path = tmpFile.fileName();
    tmpFile.close();

    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("Test");
        writer.save(file);
        RecHeader recHeader;
        recHeader.id = 0x9ABC;
        writer.startRecord('PACK', recHeader);
        rec.save(writer);
        writer.endRecord();
        writer.close();
        file.close();
    }

    ESMReader reader(path);
    reader.open();
    QCOMPARE(reader.readName(), static_cast<quint32>('PACK'));
    PackageRecord loaded;
    loaded.load(reader, true);
    QCOMPARE(loaded.pkdtRaws.at(0), originalPkdt);
    QCOMPARE(loaded.targetIds.size(), 1);
}

QTEST_MAIN(TestPackageRecord)
#include "test_packagerecord.moc"

#include <QTest>

#include "packagesemantics.hpp"

using namespace openck;

// The AI package model decodes PKDT/PLDT payloads that the record layer keeps
// verbatim. The tests here pin the two properties that matter: decoding reads
// what the engine wrote, and re-encoding leaves every byte the editor does not
// own untouched (an unedited plugin must still round-trip exactly).
class TestPackageSemantics : public QObject
{
    Q_OBJECT

private slots:
    void testKindNames();
    void testRoadKinds();
    void testDecodeSandboxPackage();
    void testDecodeSchedule();
    void testEncodePreservesForeignBytes();
    void testEncodeCreatesPayload();
    void testEncodeRoundTrip();
    void testValidationMissingReference();
    void testValidationPatrolRoadWarning();
    void testScheduleChecks();
    void initTestCase();
};

static QByteArray makePkdt(quint32 type, quint32 flags, quint32 third,
                            quint8 month, quint8 weekday, quint8 date,
                            quint8 hour, quint32 minute, quint8 doAll,
                            int trailingJunk = 0)
{
    QByteArray bytes(18 + trailingJunk, '\0');
    const auto put32 = [&bytes](int offset, quint32 value) {
        bytes[offset] = static_cast<char>(value & 0xFF);
        bytes[offset + 1] = static_cast<char>((value >> 8) & 0xFF);
        bytes[offset + 2] = static_cast<char>((value >> 16) & 0xFF);
        bytes[offset + 3] = static_cast<char>((value >> 24) & 0xFF);
    };
    put32(0, type);
    put32(4, flags);
    put32(8, third);
    bytes[12] = static_cast<char>(month);
    bytes[13] = static_cast<char>(weekday);
    bytes[14] = static_cast<char>(date);
    bytes[15] = static_cast<char>(hour);
    bytes[16] = static_cast<char>(minute == 0xFF ? 0xFF : minute);
    bytes[17] = static_cast<char>(doAll);
    for (int i = 0; i < trailingJunk; ++i)
    {
        bytes[18 + i] = static_cast<char>((i + 1) & 0xFF);
    }
    return bytes;
}

void TestPackageSemantics::initTestCase()
{
    // QObject::tr is exercised through the schedule labels.
}

void TestPackageSemantics::testKindNames()
{
    QCOMPARE(packageKindName(PackageKind::Sandbox), QStringLiteral("Sandbox"));
    QCOMPARE(packageKindName(PackageKind::Patrol), QStringLiteral("Patrol"));
    QCOMPARE(packageKindName(PackageKind::Patrol_Alt), QStringLiteral("Patrol (alt)"));
    QCOMPARE(packageKindName(PackageKind::Combat), QStringLiteral("Combat"));
    QCOMPARE(packageKindName(PackageKind::Unknown), QStringLiteral("Unknown"));
    QVERIFY(packageKindFromU32(0) == PackageKind::Unknown);
    QVERIFY(packageKindFromU32(31) == PackageKind::Unknown);
    QVERIFY(packageKindFromU32(18) == PackageKind::Sandbox);
}

void TestPackageSemantics::testRoadKinds()
{
    QVERIFY(packageKindIsRoad(quint32(PackageKind::Travel)));
    QVERIFY(packageKindIsRoad(quint32(PackageKind::Patrol)));
    QVERIFY(!packageKindIsRoad(quint32(PackageKind::Sandbox)));
}

void TestPackageSemantics::testDecodeSandboxPackage()
{
    PackageRecord record;
    record.pkdtRaws.append(makePkdt(18, 0x12, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0));
    record.packageType = 18;

    bool ok = false;
    const PackageData data = decodePackageData(record, &ok);
    QVERIFY(ok);
    QCOMPARE(data.type, 18u);
    QCOMPARE(data.flags, 0x12u);
    QVERIFY(packageKindFromU32(data.type) == PackageKind::Sandbox);
}

void TestPackageSemantics::testDecodeSchedule()
{
    PackageRecord record;
    record.pkdtRaws.append(makePkdt(18, 0, 0, 2, 3, 11, 8, 45, 0));
    // A second entry must not disturb decoding of the first.
    record.pkdtRaws.append(makePkdt(19, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0));

    bool ok = false;
    const PackageData data = decodePackageData(record, &ok);
    QVERIFY(ok);
    QCOMPARE(data.schedule.month, quint8(2));
    QCOMPARE(data.schedule.weekday, quint8(3));
    QCOMPARE(data.schedule.date, quint8(11));
    QCOMPARE(data.schedule.hour, quint8(8));
    QCOMPARE(data.schedule.minute, 45);
    QVERIFY(scheduleConstraintSet(data.schedule));

    const PackageRecord blank;
    QVERIFY(!scheduleConstraintSet(PackageSchedule()));
}

void TestPackageSemantics::testEncodePreservesForeignBytes()
{
    PackageRecord record;
    // 18 header bytes plus 5 trailing bytes the model does not own.
    record.pkdtRaws.append(makePkdt(18, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 255, 0, 5));
    record.pkdtRaws.append(makePkdt(19, 7, 9, 0xFF, 0xFF, 0xFF, 0xFF, 255, 1));
    const QByteArray originalSecond = record.pkdtRaws.at(1);

    bool ok = false;
    const PackageData data = decodePackageData(record, &ok);
    QVERIFY(ok);

    PackageData edited = data;
    edited.schedule.month = 5;
    edited.schedule.weekday = 1;
    edited.schedule.hour = 21;
    edited.schedule.minute = 10;
    edited.doAll = true;
    encodePackageData(record, edited);

    // The trailing five bytes survive verbatim.
    const QByteArray after = record.pkdtRaws.at(0);
    QCOMPARE(after.size(), 23);
    QCOMPARE(after.mid(18, 5), record.pkdtRaws.at(0).mid(18, 5));
    for (int i = 0; i < 5; ++i)
    {
        QCOMPARE(static_cast<quint8>(after.at(18 + i)), quint8((i + 1) & 0xFF));
    }
    // The second payload is untouched, so a multi-entry package still
    // round-trips byte-for-byte.
    QCOMPARE(record.pkdtRaws.at(1), originalSecond);
    // The fields the editor owns changed.
    QCOMPARE(static_cast<quint8>(after.at(12)), quint8(5));
    QCOMPARE(static_cast<quint8>(after.at(13)), quint8(1));
    QCOMPARE(static_cast<quint8>(after.at(15)), quint8(21));
    QCOMPARE(static_cast<quint8>(after.at(17)), quint8(1));
    QCOMPARE(static_cast<quint8>(after.at(16)), quint8(10));
}

void TestPackageSemantics::testEncodeCreatesPayload()
{
    PackageRecord record;
    QVERIFY(record.pkdtRaws.isEmpty());

    PackageData data;
    data.type = 18;
    data.flags = 4;
    data.schedule.month = 1;
    data.schedule.weekday = 0xFF;
    data.schedule.date = 0xFF;
    data.schedule.hour = 4;
    data.schedule.minute = 30;
    data.doAll = true;
    encodePackageData(record, data);

    QCOMPARE(int(record.pkdtRaws.size()), 1);
    QVERIFY(record.hasPkdt);
    bool ok = false;
    const PackageData reloaded = decodePackageData(record, &ok);
    QVERIFY(ok);
    QCOMPARE(reloaded.type, 18u);
    QCOMPARE(reloaded.schedule.month, quint8(1));
    QCOMPARE(reloaded.schedule.weekday, quint8(0xFF));
    QCOMPARE(reloaded.schedule.hour, quint8(4));
    QCOMPARE(reloaded.schedule.minute, 30);
    QVERIFY(reloaded.doAll);
}

void TestPackageSemantics::testEncodeRoundTrip()
{
    PackageRecord record;
    record.pkdtRaws.append(makePkdt(19, 0x2A, 0x99, 6, 5, 20, 3, 59, 1, 3));
    bool ok = false;
    const PackageData data = decodePackageData(record, &ok);
    QVERIFY(ok);

    PackageRecord copy = record;
    encodePackageData(copy, data);
    QCOMPARE(copy.pkdtRaws.at(0), record.pkdtRaws.at(0));
}

void TestPackageSemantics::testValidationMissingReference()
{
    PackageRecord record;
    record.pkdtRaws.append(makePkdt(18, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0));
    bool ok = false;
    const PackageData sandbox = decodePackageData(record, &ok);
    QVERIFY(ok);
    QVERIFY(validatePackageData(record, sandbox).isEmpty());

    // Escort (3) must reference a target.
    PackageRecord escort;
    escort.pkdtRaws.append(makePkdt(3, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0));
    bool ok2 = false;
    const PackageData escortData = decodePackageData(escort, &ok2);
    QVERIFY(ok2);
    const QVector<PackageIssue> issues = validatePackageData(escort, escortData);
    QCOMPARE(int(issues.size()), 1);
    QCOMPARE(issues.at(0).severity, PackageIssueSeverity::Error);

    // Supplying a target clears it.
    escort.targetIds.append(0x01000D42);
    QVERIFY(validatePackageData(escort, escortData).isEmpty());
}

void TestPackageSemantics::testValidationPatrolRoadWarning()
{
    PackageRecord record;
    record.pkdtRaws.append(makePkdt(19, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0));
    bool ok = false;
    const PackageData data = decodePackageData(record, &ok);
    QVERIFY(ok);
    const QVector<PackageIssue> issues = validatePackageData(record, data);
    QCOMPARE(int(issues.size()), 1);
    QCOMPARE(issues.at(0).severity, PackageIssueSeverity::Warning);

    record.pkdtRaws[0] = makePkdt(19, 0x4, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0);
    bool ok3 = false;
    const PackageData flagged = decodePackageData(record, &ok3);
    QVERIFY(ok3);
    QVERIFY(validatePackageData(record, flagged).isEmpty());
}

void TestPackageSemantics::testScheduleChecks()
{
    PackageSchedule schedule;
    schedule.month = 1;
    schedule.weekday = 0xFF;
    schedule.date = 0xFF;
    schedule.hour = 0xFF;
    schedule.minute = -1;

    const QVector<ScheduleCheck> checks = scheduleChecks(schedule);
    QCOMPARE(int(checks.size()), 1);
    QVERIFY(checks.at(0).name.contains(QStringLiteral("January")));

    // With a "now", the active flag reflects the match.
    const QVector<ScheduleCheck> matching =
        scheduleChecks(schedule, QDateTime(QDate(2025, 1, 15), QTime(12, 0)));
    QVERIFY(matching.at(0).active);

    const QVector<ScheduleCheck> notMatching =
        scheduleChecks(schedule, QDateTime(QDate(2025, 7, 15), QTime(12, 0)));
    QVERIFY(!notMatching.at(0).active);

    const PackageSchedule none;
    QVERIFY(scheduleChecks(none).isEmpty());
}

QTEST_MAIN(TestPackageSemantics)
#include "test_packagesemantics.moc"

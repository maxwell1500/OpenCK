#include <QTest>

#include "../../libs/files/esm/conditionrecord.hpp"

class TestCtdaConditions : public QObject
{
    Q_OBJECT

private slots:
    void testPackUnpackBase();
    void testPackUnpackExtended();
    void testUnpackBadSize();
    void testNames();
    void testListRoundTrip();
    void testOrJoinFlag();
};

void TestCtdaConditions::testPackUnpackBase()
{
    CtdaCondition in;
    in.comparison = CtdaCondition::Comparison::GreaterThanOrEqualTo;
    in.flags = 0x01;
    in.functionId = 0x1A;   // GetValue
    in.param1 = 0x12345678;
    in.param2 = 5;
    in.runOn = CtdaCondition::RunOn::Target;
    in.reference = 0x00001234;
    in.unk1 = 0xAA;
    in.unk2 = 0xBB;

    const QByteArray packed = in.pack();
    QCOMPARE(packed.size(), 28);

    CtdaCondition out;
    QVERIFY(CtdaCondition::unpack(packed, out));
    QVERIFY(!out.extendedBytes);
    QCOMPARE(out.comparison, CtdaCondition::Comparison::GreaterThanOrEqualTo);
    QCOMPARE(out.flags, static_cast<quint8>(0x01));
    QCOMPARE(out.functionId, static_cast<quint32>(0x1A));
    QCOMPARE(out.param1, static_cast<quint32>(0x12345678));
    QCOMPARE(out.param2, static_cast<quint32>(5));
    QCOMPARE(out.runOn, CtdaCondition::RunOn::Target);
    QCOMPARE(out.reference, static_cast<quint32>(0x00001234));
    QCOMPARE(out.unk1, static_cast<quint32>(0xAA));
    QCOMPARE(out.unk2, static_cast<quint32>(0xBB));
}

void TestCtdaConditions::testPackUnpackExtended()
{
    CtdaCondition in;
    in.extendedBytes = true;
    in.comparison = CtdaCondition::Comparison::LessThan;
    in.functionId = 0x40;
    in.unk3 = 0x1111;
    in.unk4 = 0x2222;

    const QByteArray packed = in.pack();
    QCOMPARE(packed.size(), 36);

    CtdaCondition out;
    QVERIFY(CtdaCondition::unpack(packed, out));
    QVERIFY(out.extendedBytes);
    QCOMPARE(out.comparison, CtdaCondition::Comparison::LessThan);
    QCOMPARE(out.functionId, static_cast<quint32>(0x40));
    QCOMPARE(out.unk3, static_cast<quint32>(0x1111));
    QCOMPARE(out.unk4, static_cast<quint32>(0x2222));
}

void TestCtdaConditions::testUnpackBadSize()
{
    CtdaCondition out;
    QVERIFY(!CtdaCondition::unpack(QByteArray(27, '\0'), out));
    QVERIFY(!CtdaCondition::unpack(QByteArray(), out));
    QVERIFY(!CtdaCondition::unpack(QByteArray(28, '\0'), out) == false);
}

void TestCtdaConditions::testNames()
{
    QCOMPARE(CtdaCondition::comparisonName(CtdaCondition::Comparison::EqualTo),
             QStringLiteral("=="));
    QCOMPARE(CtdaCondition::comparisonName(CtdaCondition::Comparison::LessThanOrEqualTo),
             QStringLiteral("<="));
    QCOMPARE(CtdaCondition::runOnName(CtdaCondition::RunOn::Subject),
             QStringLiteral("Subject"));
    QCOMPARE(CtdaCondition::runOnName(CtdaCondition::RunOn::QuestAlias),
             QStringLiteral("Quest Alias"));
}

void TestCtdaConditions::testListRoundTrip()
{
    QVector<CtdaCondition> in;
    CtdaCondition a;
    a.functionId = 1;
    a.param1 = 10;
    CtdaCondition b;
    b.functionId = 2;
    b.comparison = CtdaCondition::Comparison::GreaterThan;
    b.reference = 0x1234;
    in << a << b;

    const QByteArray packed = CtdaCondition::packList(in);
    const QVector<CtdaCondition> out = CtdaCondition::unpackList(packed);

    QCOMPARE(out.size(), 2);
    QCOMPARE(out[0].functionId, static_cast<quint32>(1));
    QCOMPARE(out[0].param1, static_cast<quint32>(10));
    QCOMPARE(out[1].functionId, static_cast<quint32>(2));
    QCOMPARE(out[1].comparison, CtdaCondition::Comparison::GreaterThan);
    QCOMPARE(out[1].reference, static_cast<quint32>(0x1234));

    // Truncated payload still yields the conditions that parsed.
    const QVector<CtdaCondition> partial =
        CtdaCondition::unpackList(packed.left(8));
    QVERIFY(partial.isEmpty());
}

void TestCtdaConditions::testOrJoinFlag()
{
    CtdaCondition condition;
    QVERIFY(!condition.useOr());
    condition.setUseOr(true);
    QVERIFY(condition.useOr());
    QVERIFY((condition.flags & 0x01) != 0);
    condition.setUseOr(false);
    QVERIFY(!condition.useOr());
}

QTEST_MAIN(TestCtdaConditions)
#include "test_conditionrecord.moc"

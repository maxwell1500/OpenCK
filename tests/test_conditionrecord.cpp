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
    void testFunctionNameRoundTrip();
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
}

void TestCtdaConditions::testPackUnpackExtended()
{
    CtdaCondition in;
    in.extendedBytes = true;
    in.comparison = CtdaCondition::Comparison::LessThan;
    in.functionId = 0x40;
    in.unk2 = 0x2222;
    in.unk3 = 0x1111;

    const QByteArray packed = in.pack();
    QCOMPARE(packed.size(), 36);

    CtdaCondition out;
    QVERIFY(CtdaCondition::unpack(packed, out));
    QVERIFY(out.extendedBytes);
    QCOMPARE(out.comparison, CtdaCondition::Comparison::LessThan);
    QCOMPARE(out.functionId, static_cast<quint32>(0x40));
    QCOMPARE(out.unk2, static_cast<quint32>(0x2222));
    QCOMPARE(out.unk3, static_cast<quint32>(0x1111));
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

void TestCtdaConditions::testFunctionNameRoundTrip()
{
    // A known index renders its name and maps straight back.
    QCOMPARE(CtdaCondition::functionName(0x0), QStringLiteral("GetKeywordCount"));
    quint32 id = 0;
    QVERIFY(CtdaCondition::functionIdForName(QStringLiteral("GetKeywordCount"), &id));
    QCOMPARE(id, static_cast<quint32>(0));

    // An unknown index gets a stable "Function <hex>" label that maps back.
    const quint32 unknown = 0x1A2B;
    const QString label = CtdaCondition::functionName(unknown);
    QCOMPARE(label, QStringLiteral("Function 1a2b"));
    QVERIFY(CtdaCondition::functionIdForName(label, &id));
    QCOMPARE(id, unknown);

    // Raw hex (with and without the 0x prefix) also resolves.
    QVERIFY(CtdaCondition::functionIdForName(QStringLiteral("1a2b"), &id));
    QCOMPARE(id, unknown);
    QVERIFY(CtdaCondition::functionIdForName(QStringLiteral("0x1A2B"), &id));
    QCOMPARE(id, unknown);

    // Round-trip holds for a spread of ids (known and unknown).
    const quint32 samples[] = { 0x0, 0x1, 0x1A, 0x40, 0xFFFF, 0x12345678 };
    for (quint32 s : samples)
    {
        const QString n = CtdaCondition::functionName(s);
        QVERIFY(CtdaCondition::functionIdForName(n, &id));
        QCOMPARE(id, s);
    }
}

QTEST_MAIN(TestCtdaConditions)
#include "test_conditionrecord.moc"

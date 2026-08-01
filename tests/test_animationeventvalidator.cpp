#include <QTest>

#include "../../src/model/tools/animationeventvalidator.hpp"

class TestAnimationEventValidator : public QObject
{
    Q_OBJECT

private slots:
    void testBuiltinRules();
    void testIsValidForAnimation();
    void testUnknownEventsAllowed();
    void testMatchingRules();
};

void TestAnimationEventValidator::testBuiltinRules()
{
    const auto rules = AnimationEventValidator::builtinRules();
    QVERIFY(rules.size() >= 10);

    bool sawSyncRightFoot = false;
    bool sawWeaponFire = false;
    for (const auto& rule : rules)
    {
        if (rule.name == QStringLiteral("SyncRightFoot"))
        {
            sawSyncRightFoot = true;
            QVERIFY(rule.validAnimationTypes.contains(QStringLiteral("Run")));
            QVERIFY(rule.validAnimationTypes.contains(QStringLiteral("Walk")));
            QVERIFY(rule.validAnimationTypes.contains(QStringLiteral("Jog")));
        }
        if (rule.name == QStringLiteral("WeaponFire"))
        {
            sawWeaponFire = true;
            QVERIFY(rule.validAnimationTypes.contains(QStringLiteral("FireSingle")));
            QVERIFY(rule.validAnimationTypes.contains(QStringLiteral("FireAuto")));
        }
    }
    QVERIFY(sawSyncRightFoot);
    QVERIFY(sawWeaponFire);
}

void TestAnimationEventValidator::testIsValidForAnimation()
{
    QVERIFY(AnimationEventValidator::isEventValidForAnimation(
        QStringLiteral("HitFrame"), QStringLiteral("MeleeAttack")));
    QVERIFY(!AnimationEventValidator::isEventValidForAnimation(
        QStringLiteral("HitFrame"), QStringLiteral("Run")));

    QVERIFY(AnimationEventValidator::isEventValidForAnimation(
        QStringLiteral("WeaponFire"), QStringLiteral("FireAuto")));
    QVERIFY(!AnimationEventValidator::isEventValidForAnimation(
        QStringLiteral("WeaponFire"), QStringLiteral("Jump")));

    // Case-insensitive matching.
    QVERIFY(AnimationEventValidator::isEventValidForAnimation(
        QStringLiteral("hitframe"), QStringLiteral("MeleeAttack")));
}

void TestAnimationEventValidator::testUnknownEventsAllowed()
{
    // Events not in any rule set are permitted (no false positives).
    QVERIFY(AnimationEventValidator::isEventValidForAnimation(
        QStringLiteral("CustomEvent123"), QStringLiteral("Anything")));
}

void TestAnimationEventValidator::testMatchingRules()
{
    const QStringList names = AnimationEventValidator::matchingRuleNames(
        QStringLiteral("Weapon"));
    QVERIFY(!names.isEmpty());
    QVERIFY(names.contains(QStringLiteral("WeaponFire")));
    QVERIFY(names.contains(QStringLiteral("WeaponSwing")));

    // Exact match works too.
    const QStringList hit = AnimationEventValidator::matchingRuleNames(
        QStringLiteral("HitFrame"));
    QCOMPARE(hit, QStringList({ QStringLiteral("HitFrame") }));
}

QTEST_MAIN(TestAnimationEventValidator)
#include "test_animationeventvalidator.moc"

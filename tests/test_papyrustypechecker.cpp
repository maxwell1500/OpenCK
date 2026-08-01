#include <QTest>

#include "../../src/model/tools/papyrustypechecker.hpp"
#include "../../libs/files/log/logger.hpp"

class TestPapyrusTypeChecker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testArrayLength();
    void testPropertyAccess();
    void testUnknownMember();
    void testArrayIndex();
};

void TestPapyrusTypeChecker::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_typechecker_log.txt"));
}

void TestPapyrusTypeChecker::testArrayLength()
{
    PapyrusTypeChecker tc;
    tc.declareVariable(QStringLiteral("items"),
        PapyrusTypeChecker::TypeInfo{ QStringLiteral("Form"), true });
    tc.registerScriptType(QStringLiteral("Form"));

    const PapyrusTypeChecker::TypeInfo len = tc.resolveMemberAccess(QStringLiteral("items"), QStringLiteral("Length"));
    QVERIFY(len.isValid());
    QCOMPARE(len.name, QStringLiteral("Int"));
    QVERIFY(!len.isArray);

    // Non-array variable has no Length.
    tc.declareVariable(QStringLiteral("count"),
        PapyrusTypeChecker::TypeInfo{ QStringLiteral("Int"), false });
    QVERIFY(!tc.resolveMemberAccess(QStringLiteral("count"), QStringLiteral("Length")).isValid());
}

void TestPapyrusTypeChecker::testPropertyAccess()
{
    PapyrusTypeChecker tc;
    tc.registerScriptType(QStringLiteral("Actor"));
    tc.registerProperty(QStringLiteral("Actor"), QStringLiteral("IsInCombat"),
        PapyrusTypeChecker::TypeInfo{ QStringLiteral("Bool"), false });
    tc.registerProperty(QStringLiteral("Actor"), QStringLiteral("Health"),
        PapyrusTypeChecker::TypeInfo{ QStringLiteral("Float"), false });

    tc.declareVariable(QStringLiteral("actor"),
        PapyrusTypeChecker::TypeInfo{ QStringLiteral("Actor"), false, true });

    QVERIFY(tc.hasProperty(QStringLiteral("Actor"), QStringLiteral("IsInCombat")));
    QCOMPARE(tc.resolveMemberAccess(QStringLiteral("actor"), QStringLiteral("IsInCombat")).name,
        QStringLiteral("Bool"));
    QCOMPARE(tc.resolveMemberAccess(QStringLiteral("actor"), QStringLiteral("Health")).name,
        QStringLiteral("Float"));
}

void TestPapyrusTypeChecker::testUnknownMember()
{
    PapyrusTypeChecker tc;
    tc.registerScriptType(QStringLiteral("Actor"));
    tc.declareVariable(QStringLiteral("actor"),
        PapyrusTypeChecker::TypeInfo{ QStringLiteral("Actor"), false, true });

    // Unknown property on a known object -> invalid type.
    QVERIFY(!tc.resolveMemberAccess(QStringLiteral("actor"), QStringLiteral("NotAProperty")).isValid());
    // Unknown object -> invalid.
    QVERIFY(!tc.resolveMemberAccess(QStringLiteral("ghost"), QStringLiteral("Health")).isValid());
}

void TestPapyrusTypeChecker::testArrayIndex()
{
    PapyrusTypeChecker tc;
    tc.declareVariable(QStringLiteral("items"),
        PapyrusTypeChecker::TypeInfo{ QStringLiteral("Form"), true });

    QVERIFY(tc.checkArrayIndex(tc.variableType(QStringLiteral("items")), 1));

    // Array element type: Form[] -> element Form.
    const PapyrusTypeChecker::TypeInfo elem = tc.elementTypeOf(tc.variableType(QStringLiteral("items")));
    QVERIFY(elem.isValid());
    QCOMPARE(elem.name, QStringLiteral("Form"));
    QVERIFY(!elem.isArray);
}

QTEST_MAIN(TestPapyrusTypeChecker)
#include "test_papyrustypechecker.moc"

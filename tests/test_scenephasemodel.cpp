#include <QTest>

#include "../../src/model/tools/scenephasemodel.hpp"
#include "../../libs/files/log/logger.hpp"

class TestScenePhaseModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testInsertAndClamp();
    void testRemove();
    void testMoveStartClamped();
    void testPack();
};

void TestScenePhaseModel::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_scenephase_log.txt"));
}

void TestScenePhaseModel::testInsertAndClamp()
{
    QVector<ScenePhase> phases;
    ScenePhase a;
    a.name = "A"; a.startTime = 0.0; a.endTime = 2.0;
    phases.append(a);

    // Insert a phase starting before the previous end -> clamped.
    ScenePhase b;
    b.name = "B"; b.startTime = 1.0; b.endTime = 2.0;
    ScenePhaseModel::insertPhase(phases, 1, b);
    QCOMPARE(phases.size(), 2);
    QVERIFY(phases[1].startTime >= phases[0].endTime);
    QVERIFY(phases[1].isValid());

    // Insert with end <= start -> normalized to +1s.
    ScenePhase c;
    c.name = "C"; c.startTime = 5.0; c.endTime = 4.0;
    ScenePhaseModel::insertPhase(phases, 2, c);
    QVERIFY(phases[2].isValid());
    QVERIFY(phases[2].endTime > phases[2].startTime);
}

void TestScenePhaseModel::testRemove()
{
    QVector<ScenePhase> phases;
    for (int i = 0; i < 3; ++i)
    {
        ScenePhase p;
        p.name = QString("P%1").arg(i);
        p.startTime = i; p.endTime = i + 1;
        phases.append(p);
    }
    ScenePhaseModel::removePhase(phases, 1);
    QCOMPARE(phases.size(), 2);
    QCOMPARE(phases[0].name, QStringLiteral("P0"));
    QCOMPARE(phases[1].name, QStringLiteral("P2"));

    ScenePhaseModel::removePhase(phases, -1);   // no-op
    ScenePhaseModel::removePhase(phases, 99);   // no-op
    QCOMPARE(phases.size(), 2);
}

void TestScenePhaseModel::testMoveStartClamped()
{
    QVector<ScenePhase> phases;
    for (int i = 0; i < 3; ++i)
    {
        ScenePhase p;
        p.name = QString("P%1").arg(i);
        p.startTime = i * 2.0; p.endTime = i * 2.0 + 1.0;
        phases.append(p);
    }
    // Move phase 1 start way back -> clamped to previous phase end (1.0).
    ScenePhaseModel::movePhaseStart(phases, 1, -100.0);
    QCOMPARE(phases[1].startTime, 1.0);
    QCOMPARE(phases[1].endTime, 2.0); // duration preserved
    // Move it way forward -> clamped before next phase's end minus duration.
    ScenePhaseModel::movePhaseStart(phases, 1, 100.0);
    QVERIFY(phases[1].endTime <= phases[2].endTime);
    QVERIFY(phases[1].startTime >= phases[0].endTime);
}

void TestScenePhaseModel::testPack()
{
    QVector<ScenePhase> phases;
    ScenePhase a; a.name = "A"; a.startTime = 0.0; a.endTime = 2.0;
    ScenePhase b; b.name = "B"; b.startTime = 10.0; b.endTime = 12.0;
    phases.append(a);
    phases.append(b);

    ScenePhaseModel::pack(phases, 0.5);
    QCOMPARE(phases[0].startTime, 0.0);
    QVERIFY(qFuzzyCompare(phases[0].endTime, 2.0));
    QVERIFY(qFuzzyCompare(phases[1].startTime, 2.5));
    QVERIFY(qFuzzyCompare(phases[1].endTime, 4.5));
}

QTEST_MAIN(TestScenePhaseModel)
#include "test_scenephasemodel.moc"

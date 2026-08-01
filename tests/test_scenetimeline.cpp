#include <QTest>

#include "../../src/model/tools/scenephasemodel.hpp"

class TestSceneTimeline : public QObject
{
    Q_OBJECT

private slots:
    void testPack();
    void testInsertClamps();
    void testMoveClamps();
    void testRemoveGap();
};

void TestSceneTimeline::testPack()
{
    QVector<ScenePhase> phases;
    ScenePhase a; a.name = "A"; a.startTime = 0.0; a.endTime = 5.0;
    ScenePhase b; b.name = "B"; b.startTime = 6.0; b.endTime = 8.0;
    ScenePhase c; c.name = "C"; c.startTime = 10.0; c.endTime = 12.0;
    phases = { a, b, c };

    ScenePhaseModel::pack(phases, 0.1);
    QCOMPARE(phases.size(), 3);
    QCOMPARE(phases[0].startTime, 0.0);
    QVERIFY(qFuzzyCompare(phases[1].startTime, 5.1));
    QVERIFY(qFuzzyCompare(phases[2].startTime, 7.2));  // B: 5.1..7.1 + 0.1 gap
    QVERIFY(qFuzzyCompare(phases[2].endTime, 9.2));
}

void TestSceneTimeline::testInsertClamps()
{
    QVector<ScenePhase> phases;
    ScenePhase a; a.name = "A"; a.startTime = 0.0; a.endTime = 2.0;
    ScenePhase b; b.name = "B"; b.startTime = 3.0; b.endTime = 4.0;
    phases = { a, b };

    // Insert at index 1; the start must clamp to >= previous end (2.0).
    ScenePhase mid; mid.name = "M"; mid.startTime = 1.0; mid.endTime = 2.5;
    ScenePhaseModel::insertPhase(phases, 1, mid);
    QCOMPARE(phases.size(), 3);
    QVERIFY(phases[1].startTime >= 2.0);
    QVERIFY(phases[1].isValid());
}

void TestSceneTimeline::testMoveClamps()
{
    QVector<ScenePhase> phases;
    ScenePhase a; a.name = "A"; a.startTime = 0.0; a.endTime = 2.0;
    ScenePhase b; b.name = "B"; b.startTime = 3.0; b.endTime = 5.0;
    phases = { a, b };

    // Moving B too far left clamps to >= A's end.
    ScenePhaseModel::movePhaseStart(phases, 1, 0.5);
    QVERIFY(phases[1].startTime >= 2.0);
    QVERIFY(phases[1].isValid());

    // Moving A's start later clamps to <= B's start.
    ScenePhaseModel::movePhaseStart(phases, 0, 4.0);
    QVERIFY(phases[0].startTime <= 3.0);
    QVERIFY(phases[0].isValid());
}

void TestSceneTimeline::testRemoveGap()
{
    QVector<ScenePhase> phases;
    ScenePhase a; a.name = "A"; a.startTime = 0.0; a.endTime = 2.0;
    ScenePhase b; b.name = "B"; b.startTime = 2.0; b.endTime = 3.0;
    ScenePhase c; c.name = "C"; c.startTime = 3.0; c.endTime = 5.0;
    phases = { a, b, c };

    ScenePhaseModel::removePhase(phases, 1);
    QCOMPARE(phases.size(), 2);
    QCOMPARE(phases[0].name, QStringLiteral("A"));
    QCOMPARE(phases[1].name, QStringLiteral("C"));
    // The successor's start closes the gap left by the removed phase.
    QVERIFY(phases[1].startTime >= 2.0);
    QVERIFY(phases[1].isValid());
}

QTEST_MAIN(TestSceneTimeline)
#include "test_scenetimeline.moc"

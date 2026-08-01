#include <QTest>

#include "../../src/model/tools/referencebatchactions.hpp"
#include "../../libs/files/esm/cellreferencedata.hpp"
#include "../../libs/files/log/logger.hpp"

class TestReferenceBatchActions : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testMoveByOffset();
    void testSnapToGrid();
    void testSetScale();
    void testSetFlag();
    void testResetRotation();
};

void TestReferenceBatchActions::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_refbatch_log.txt"));
}

void TestReferenceBatchActions::testMoveByOffset()
{
    QVector<CellRefEntry> refs;
    CellRefEntry a;
    a.posX = 1.0f; a.posY = 2.0f; a.posZ = 3.0f;
    CellRefEntry b;
    b.posX = 10.0f; b.posY = 20.0f; b.posZ = 30.0f;
    refs.append(a);
    refs.append(b);

    ReferenceBatchActions::moveByOffset(refs, 5.0f, -2.0f, 0.5f);

    QVERIFY(qFuzzyCompare(refs[0].posX, 6.0f));
    QVERIFY(qFuzzyCompare(refs[0].posY, 0.0f));
    QVERIFY(qFuzzyCompare(refs[0].posZ, 3.5f));
    QVERIFY(qFuzzyCompare(refs[1].posX, 15.0f));
    QVERIFY(qFuzzyCompare(refs[1].posY, 18.0f));
    QVERIFY(qFuzzyCompare(refs[1].posZ, 30.5f));
}

void TestReferenceBatchActions::testSnapToGrid()
{
    QVector<CellRefEntry> refs;
    CellRefEntry a;
    a.posX = 13.4f; a.posY = 7.1f; a.posZ = -2.6f;
    refs.append(a);

    ReferenceBatchActions::snapToGrid(refs, 5.0f);

    QVERIFY(qFuzzyCompare(refs[0].posX, 15.0f));
    QVERIFY(qFuzzyCompare(refs[0].posY, 5.0f));
    QVERIFY(qFuzzyCompare(refs[0].posZ, -5.0f));

    // Zero/negative grid is a no-op.
    ReferenceBatchActions::snapToGrid(refs, 0.0f);
    QVERIFY(qFuzzyCompare(refs[0].posX, 15.0f));
}

void TestReferenceBatchActions::testSetScale()
{
    QVector<CellRefEntry> refs;
    CellRefEntry a;
    a.scale = 1.0f;
    CellRefEntry b;
    b.scale = 0.5f;
    refs.append(a);
    refs.append(b);

    ReferenceBatchActions::setScale(refs, 2.0f);

    QVERIFY(qFuzzyCompare(refs[0].scale, 2.0f));
    QVERIFY(qFuzzyCompare(refs[1].scale, 2.0f));
}

void TestReferenceBatchActions::testSetFlag()
{
    QVector<CellRefEntry> refs;
    CellRefEntry a;
    a.flags = 0;
    refs.append(a);

    ReferenceBatchActions::setFlag(refs, ReferenceBatchActions::Flag::Disabled, true);
    QVERIFY(refs[0].isDisabled());
    QVERIFY(!refs[0].isHidden());

    ReferenceBatchActions::setFlag(refs, ReferenceBatchActions::Flag::Hidden, true);
    QVERIFY(refs[0].isHidden());

    ReferenceBatchActions::setFlag(refs, ReferenceBatchActions::Flag::Disabled, false);
    QVERIFY(!refs[0].isDisabled());
    QVERIFY(refs[0].isHidden());
}

void TestReferenceBatchActions::testResetRotation()
{
    QVector<CellRefEntry> refs;
    CellRefEntry a;
    a.rotX = 0.5f; a.rotY = 1.0f; a.rotZ = -0.25f;
    refs.append(a);

    ReferenceBatchActions::resetRotation(refs);

    QCOMPARE(refs[0].rotX, 0.0f);
    QCOMPARE(refs[0].rotY, 0.0f);
    QCOMPARE(refs[0].rotZ, 0.0f);
}

QTEST_MAIN(TestReferenceBatchActions)
#include "test_referencebatchactions.moc"

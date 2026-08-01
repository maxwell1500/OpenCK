#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../src/model/tools/mastermanagement.hpp"

class TestMasterManagement : public QObject
{
    Q_OBJECT

private slots:
    void testUpdateSourceRoundTrip();
    void testApplyIniLine();
    void testLoadIni();
    void testAllocateSequential();
    void testAllocateReuseDeleted();
    void testAllocateStartId();

private:
    static QVector<quint32> makeUsed(const QVector<int>& localIds)
    {
        QVector<quint32> out;
        for (int id : localIds)
            out.append(static_cast<quint32>(id));
        return out;
    }
};

void TestMasterManagement::testUpdateSourceRoundTrip()
{
    QCOMPARE(MasterManagement::updateSourceToString(MasterManagement::UpdateSource::MastersOnly),
             QStringLiteral("Masters Only"));
    QCOMPARE(MasterManagement::stringToUpdateSource(QStringLiteral("MastersAndMods")),
             MasterManagement::UpdateSource::MastersAndMods);
    QCOMPARE(MasterManagement::stringToUpdateSource(QStringLiteral("MASTERSONLY")),
             MasterManagement::UpdateSource::MastersOnly);
    QCOMPARE(MasterManagement::stringToUpdateSource(QStringLiteral("2")),
             MasterManagement::UpdateSource::LocalOnly);
}

void TestMasterManagement::testApplyIniLine()
{
    MasterManagement mms;
    QVERIFY(mms.applyIniLine(QStringLiteral("MMS_UpdateMasterFromFile"),
                             QStringLiteral("MastersOnly")));
    QCOMPARE(mms.updateSource, MasterManagement::UpdateSource::MastersOnly);

    QVERIFY(mms.applyIniLine(QStringLiteral("MMS_ReuseDeletedRecordIDs"),
                             QStringLiteral("1")));
    QVERIFY(mms.allocation.reuseDeleted);

    QVERIFY(mms.applyIniLine(QStringLiteral("MMS_StartLocalID"),
                             QStringLiteral("100")));
    QCOMPARE(mms.allocation.startLocalId, 100u);

    QVERIFY(mms.applyIniLine(QStringLiteral("MMS_UseLocalFileForUpdate"),
                             QStringLiteral("true")));
    QVERIFY(mms.useLocalFileForUpdate);

    QVERIFY(mms.applyIniLine(QStringLiteral("MMS_UpdateFile"),
                             QStringLiteral("myfile.esm")));
    QCOMPARE(mms.updateFile, QStringLiteral("myfile.esm"));

    QVERIFY(!mms.applyIniLine(QStringLiteral("UnrelatedKey"), QStringLiteral("x")));
}

void TestMasterManagement::testLoadIni()
{
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    tmp.write("[MMS]\r\n");
    tmp.write("MMS_UpdateMasterFromFile=MastersOnly\r\n");
    tmp.write("MMS_ReuseDeletedRecordIDs=0\r\n");
    tmp.write("MMS_StartLocalID=42\r\n");
    tmp.write("[OtherSection]\r\n");
    tmp.write("MMS_StartLocalID=999\r\n");
    const QString path = tmp.fileName();
    tmp.close();

    MasterManagement mms;
    QVERIFY(mms.loadIni(path));
    QCOMPARE(mms.updateSource, MasterManagement::UpdateSource::MastersOnly);
    QVERIFY(!mms.allocation.reuseDeleted);
    QCOMPARE(mms.allocation.startLocalId, 42u);
    // Keys outside the [MMS] section are ignored.
    QVERIFY(mms.rawKeys.value(QStringLiteral("MMS_StartLocalID")) == QStringLiteral("42"));
}

void TestMasterManagement::testAllocateSequential()
{
    MasterManagement mms;
    QVector<quint32> used = makeUsed({ 1, 2, 4, 5 });

    QCOMPARE(mms.allocateLocalId(used), 3u);
    used.append(3);
    QCOMPARE(mms.allocateLocalId(used), 6u);
}

void TestMasterManagement::testAllocateReuseDeleted()
{
    MasterManagement mms;
    mms.allocation.reuseDeleted = true;
    QVector<quint32> used = makeUsed({ 1, 3, 5 });

    QCOMPARE(mms.allocateLocalId(used), 2u);
    used.append(2);
    QCOMPARE(mms.allocateLocalId(used), 4u);
}

void TestMasterManagement::testAllocateStartId()
{
    MasterManagement mms;
    mms.allocation.startLocalId = 500;
    mms.allocation.nextLocalId = 500;
    QVector<quint32> used = makeUsed({ 1, 500 });

    QCOMPARE(mms.allocateLocalId(used), 501u);

    // Without reuse, a gap beyond the threshold is skipped.
    MasterManagement mms2;
    mms2.allocation.startLocalId = 10;
    mms2.allocation.nextLocalId = 10;
    QVector<quint32> used2 = makeUsed({ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22 });
    // 10..22 = 13 used, threshold 16 -> skips the whole block to 23.
    QCOMPARE(mms2.allocateLocalId(used2), 23u);
}

QTEST_MAIN(TestMasterManagement)
#include "test_mastermanagement.moc"

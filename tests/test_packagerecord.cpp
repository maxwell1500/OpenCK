#include <QTest>
#include <QTemporaryFile>
#include <QFile>

#include "../../libs/files/esm/Packagerecord.hpp"
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

QTEST_MAIN(TestPackageRecord)
#include "test_packagerecord.moc"

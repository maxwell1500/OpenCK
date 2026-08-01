#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../../src/model/tools/brushdefinition.hpp"
#include "../../libs/files/log/logger.hpp"

class TestBrushDefinition : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testOperationRoundTrip();
    void testBuiltin();
    void testFromJson();
    void testLoadFile();
};

void TestBrushDefinition::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_brushdefinition_log.txt"));
}

void TestBrushDefinition::testOperationRoundTrip()
{
    QCOMPARE(BrushDefinition::operationToString(BrushDefinition::Operation::Sculpt), QStringLiteral("Sculpt"));
    QCOMPARE(BrushDefinition::operationToString(BrushDefinition::Operation::Flatten), QStringLiteral("Flatten"));
    QCOMPARE(BrushDefinition::operationToString(BrushDefinition::Operation::Smooth), QStringLiteral("Smooth"));
    QCOMPARE(BrushDefinition::operationToString(BrushDefinition::Operation::Stamp), QStringLiteral("Stamp"));
    QCOMPARE(BrushDefinition::operationToString(BrushDefinition::Operation::BuildUp), QStringLiteral("BuildUp"));
    QCOMPARE(BrushDefinition::operationToString(BrushDefinition::Operation::Subtractive), QStringLiteral("Subtractive"));

    bool ok = false;
    QCOMPARE(BrushDefinition::stringToOperation(QStringLiteral("sculpt"), &ok), BrushDefinition::Operation::Sculpt);
    QVERIFY(ok);
    QCOMPARE(BrushDefinition::stringToOperation(QStringLiteral("FLATTEN"), &ok), BrushDefinition::Operation::Flatten);
    QVERIFY(ok);
    QCOMPARE(BrushDefinition::stringToOperation(QStringLiteral("nonsense"), &ok), BrushDefinition::Operation::Sculpt);
    QVERIFY(!ok);
}

void TestBrushDefinition::testBuiltin()
{
    const QVector<BrushDefinition> brushes = BrushDefinition::builtin();
    QCOMPARE(brushes.size(), 6);
    QSet<QString> names;
    for (const BrushDefinition& b : brushes) {
        QVERIFY(!b.name.isEmpty());
        QVERIFY(!names.contains(b.name));
        names.insert(b.name);
        QVERIFY(b.radius > 0.0);
    }
    QVERIFY(names.contains(QStringLiteral("Sculpt")));
    QVERIFY(names.contains(QStringLiteral("Flatten")));
    QVERIFY(names.contains(QStringLiteral("Smooth")));
    QVERIFY(names.contains(QStringLiteral("Stamp")));
    QVERIFY(names.contains(QStringLiteral("BuildUp")));
    QVERIFY(names.contains(QStringLiteral("Subtractive")));
}

void TestBrushDefinition::testFromJson()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("name"), QStringLiteral("River"));
    obj.insert(QStringLiteral("operation"), QStringLiteral("Sculpt"));
    obj.insert(QStringLiteral("radius"), 7.5);
    obj.insert(QStringLiteral("strength"), 25.0);
    obj.insert(QStringLiteral("falloff"), 0.8);
    obj.insert(QStringLiteral("invert"), true);

    const BrushDefinition b = BrushDefinition::fromJson(obj);
    QCOMPARE(b.name, QStringLiteral("River"));
    QCOMPARE(b.operation, BrushDefinition::Operation::Sculpt);
    QVERIFY(qFuzzyCompare(b.radius, 7.5));
    QVERIFY(qFuzzyCompare(b.strength, 25.0));
    QVERIFY(qFuzzyCompare(b.falloff, 0.8));
    QVERIFY(b.invert);

    // Unknown operation defaults to Sculpt
    QJsonObject bad;
    bad.insert(QStringLiteral("name"), QStringLiteral("Weird"));
    bad.insert(QStringLiteral("operation"), QStringLiteral("NotAThing"));
    const BrushDefinition b2 = BrushDefinition::fromJson(bad);
    QCOMPARE(b2.operation, BrushDefinition::Operation::Sculpt);
}

void TestBrushDefinition::testLoadFile()
{
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    tmpFile.write(R"({
        "brushes": [
            { "name": "OrganicFlatten", "operation": "Flatten", "radius": 8, "targetHeight": 120.0 },
            { "name": "RiverBrush", "operation": "Sculpt", "radius": 12, "invert": true },
            { "name": "", "operation": "Sculpt" },
            "not an object"
        ]
    })");
    tmpFile.close();

    QVector<BrushDefinition> out;
    QVERIFY(BrushDefinition::loadFile(tmpFile.fileName(), out));
    QCOMPARE(out.size(), 2);
    QCOMPARE(out[0].name, QStringLiteral("OrganicFlatten"));
    QCOMPARE(out[0].operation, BrushDefinition::Operation::Flatten);
    QVERIFY(qFuzzyCompare(out[0].targetHeight, 120.0));
    QCOMPARE(out[1].name, QStringLiteral("RiverBrush"));
    QVERIFY(out[1].invert);

    // Missing file returns false
    QVector<BrushDefinition> none;
    QVERIFY(!BrushDefinition::loadFile(QStringLiteral("C:/does/not/exist.lbr"), none));

    // Invalid JSON returns false
    QTemporaryFile badFile;
    QVERIFY(badFile.open());
    badFile.write("this is { not json");
    badFile.close();
    QVERIFY(!BrushDefinition::loadFile(badFile.fileName(), none));
}

QTEST_MAIN(TestBrushDefinition)
#include "test_brushdefinition.moc"

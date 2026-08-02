#include <QTest>

#include "../../src/model/tools/fbximporter.hpp"

class TestFbxImporter : public QObject
{
    Q_OBJECT

private slots:
    void testBlenderArguments();
    void testCustomProcessing();
    void testSummary();
};

void TestFbxImporter::testBlenderArguments()
{
    FbxImporter::Settings settings;
    settings.scriptPath = QStringLiteral("C:/scripts/nif_export.py");

    const QStringList args = FbxImporter::blenderArguments(
        QStringLiteral("C:/blender/blender.exe"),
        QStringLiteral("C:/models/weapon.fbx"),
        QStringLiteral("C:/out/weapon.nif"),
        settings);

    QCOMPARE(args.size(), 8);
    QCOMPARE(args[0], QStringLiteral("C:/blender/blender.exe"));
    QCOMPARE(args[1], QStringLiteral("--background"));
    QCOMPARE(args[2], QStringLiteral("--python"));
    QCOMPARE(args[3], QStringLiteral("C:/scripts/nif_export.py"));
    QCOMPARE(args[4], QStringLiteral("--"));
    QCOMPARE(args[5], QStringLiteral("C:/models/weapon.fbx"));
    QCOMPARE(args[6], QStringLiteral("C:/out/weapon.nif"));
    QCOMPARE(args[7], QStringLiteral("SKYRIM"));
}

void TestFbxImporter::testCustomProcessing()
{
    FbxImporter::Settings defaults;
    QVERIFY(!FbxImporter::hasCustomProcessing(defaults));

    FbxImporter::Settings noWeld = defaults;
    noWeld.weldSkin = false;
    QVERIFY(FbxImporter::hasCustomProcessing(noWeld));

    FbxImporter::Settings lod = defaults;
    lod.physicsLod = 2;
    QVERIFY(FbxImporter::hasCustomProcessing(lod));
}

void TestFbxImporter::testSummary()
{
    FbxImporter::Settings settings;
    settings.weldSkin = false;
    settings.physicsLod = 1;

    const QString summary = FbxImporter::summary(settings);
    QVERIFY(summary.contains(QStringLiteral("weld skin: off")));
    QVERIFY(summary.contains(QStringLiteral("physics LOD: LOD1")));
}

QTEST_MAIN(TestFbxImporter)
#include "test_fbximporter.moc"

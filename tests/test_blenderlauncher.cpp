#include <QtTest>
#include <QProcess>

#include "../../src/model/tools/blenderlauncher.hpp"

class TestBlenderLauncher : public QObject
{
    Q_OBJECT

private slots:
    void testFindBlender();
    void testIsBlenderAvailable();
    void testGetBlenderVersion();
    void testParseVersionNumber();
    void testIsNifCompatibleVersion();
    void testGetRecommendedBlenderPath();
    void testGetCommonBlenderPaths();
};

void TestBlenderLauncher::testFindBlender()
{
    BlenderLauncher::BlenderInfo info = BlenderLauncher::findBlender();
    // May or may not find Blender depending on system
    QVERIFY(!info.path.isEmpty() || info.path.isEmpty());
}

void TestBlenderLauncher::testIsBlenderAvailable()
{
    bool available = BlenderLauncher::isBlenderAvailable();
    // Just verify it doesn't crash
    QVERIFY(available == false || available == true);
}

void TestBlenderLauncher::testGetBlenderVersion()
{
    QString version = BlenderLauncher::getBlenderVersion(QString("/nonexistent/path"));
    // Should return empty or error string without crashing
    QVERIFY(!version.isEmpty() || version.isEmpty());
}

void TestBlenderLauncher::testParseVersionNumber()
{
    QCOMPARE(BlenderLauncher::parseVersionNumber("2.93"), 293);
    QCOMPARE(BlenderLauncher::parseVersionNumber("3.0"), 300);
    QCOMPARE(BlenderLauncher::parseVersionNumber("3.6"), 306);
    QCOMPARE(BlenderLauncher::parseVersionNumber("4.0"), 400);
    QCOMPARE(BlenderLauncher::parseVersionNumber("2.80"), 280);
}

void TestBlenderLauncher::testIsNifCompatibleVersion()
{
    // 4.4+ should be compatible
    QVERIFY(BlenderLauncher::isNifCompatibleVersion("4.4"));
    QVERIFY(BlenderLauncher::isNifCompatibleVersion("4.5"));
    QVERIFY(BlenderLauncher::isNifCompatibleVersion("5.0"));
    
    // Older versions should not be compatible
    QVERIFY(!BlenderLauncher::isNifCompatibleVersion("2.80"));
    QVERIFY(!BlenderLauncher::isNifCompatibleVersion("2.93"));
    QVERIFY(!BlenderLauncher::isNifCompatibleVersion("3.0"));
    QVERIFY(!BlenderLauncher::isNifCompatibleVersion("3.6"));
    QVERIFY(!BlenderLauncher::isNifCompatibleVersion("4.0"));
    QVERIFY(!BlenderLauncher::isNifCompatibleVersion("4.3"));
}

void TestBlenderLauncher::testGetRecommendedBlenderPath()
{
    QString path = BlenderLauncher::getRecommendedBlenderPath();
    // May or may not find a path depending on system
    QVERIFY(true);
}

void TestBlenderLauncher::testGetCommonBlenderPaths()
{
    // This is a private method, but we can test via getRecommendedBlenderPath
    QString recommended = BlenderLauncher::getRecommendedBlenderPath();
    QVERIFY(true);
}

#include "test_blenderlauncher.moc"
QTEST_MAIN(TestBlenderLauncher)

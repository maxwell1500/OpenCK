#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QColor>

#include "../../libs/files/ini/ckconfiginspector.hpp"
#include "../../libs/files/log/logger.hpp"

class TestCkConfigInspector : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testSyntheticConfigAndColors();
    void testArchiveListsAndDisplay();
    void testRealStarfieldCkConfigIfPresent();
};

void TestCkConfigInspector::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/test_ckconfiginspector_log.txt"));
}

void TestCkConfigInspector::testSyntheticConfigAndColors()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // CreationKit.ini
    {
        QFile ini(dir.filePath(QStringLiteral("CreationKit.ini")));
        QVERIFY(ini.open(QIODevice::WriteOnly | QIODevice::Text));
        ini.write(
            "[General]\n"
            "sLanguage = en\n"
            "[Archive]\n"
            "SResourceArchiveList = Base01.ba2, Base02.ba2\n"
            "sResourceIndexFileList = Textures01.ba2\n"
            "[Display]\n"
            "fDefaultFOV = 80.0\n"
            "fCameraSpeed = 2.5\n"
            "[Papyrus]\n"
            "sScriptCompiler = Tools/Papyrus Compiler/PapyrusCompiler.exe\n"
            "sScriptSourceFolder = Data/Scripts/Source\n"
            "sAdditionalImports = Data/Scripts/Source/Base\n"
        );
    }

    // EditorColors.xml
    {
        QFile xml(dir.filePath(QStringLiteral("EditorColors.xml")));
        QVERIFY(xml.open(QIODevice::WriteOnly | QIODevice::Text));
        xml.write(
            "<root>\n"
            "  <GraphBackground>\n"
            "    <Primary><R>84</R><G>90</G><B>97</B><A>255</A></Primary>\n"
            "  </GraphBackground>\n"
            "  <NodeOutline>\n"
            "    <Primary><R>10</R><G>20</G><B>30</B><A>200</A></Primary>\n"
            "    <Secondary><R>40</R><G>50</G><B>60</B><A>255</A></Secondary>\n"
            "  </NodeOutline>\n"
            "</root>\n"
        );
    }

    CkConfigInspector inspector;
    QVERIFY(inspector.loadFromDirectory(dir.path()));
    QCOMPARE(inspector.loadedDirectory(), dir.path());

    // Verify archives
    QCOMPARE(inspector.resourceArchiveList().size(), 2);
    QCOMPARE(inspector.resourceArchiveList()[0], QStringLiteral("Base01.ba2"));
    QCOMPARE(inspector.resourceIndexFileList().size(), 1);
    QCOMPARE(inspector.resourceIndexFileList()[0], QStringLiteral("Textures01.ba2"));
    QCOMPARE(inspector.allArchives().size(), 3);

    // Verify display settings
    QVERIFY(qFuzzyCompare(inspector.fov(), 80.0f));
    QVERIFY(qFuzzyCompare(inspector.cameraSpeed(), 2.5f));

    // Verify papyrus settings
    QCOMPARE(inspector.papyrusCompiler(), QStringLiteral("Tools/Papyrus Compiler/PapyrusCompiler.exe"));
    QCOMPARE(inspector.papyrusSourceFolders().size(), 1);
    QCOMPARE(inspector.papyrusAdditionalImports().size(), 1);

    // Verify colors
    QVERIFY(inspector.hasColors());
    const QColor bg = inspector.color(QStringLiteral("GraphBackground"), QStringLiteral("Primary"));
    QCOMPARE(bg, QColor(84, 90, 97, 255));

    const QColor outlineSec = inspector.color(QStringLiteral("NodeOutline"), QStringLiteral("Secondary"));
    QCOMPARE(outlineSec, QColor(40, 50, 60, 255));

    // Fallback on missing element
    const QColor fallback = inspector.color(QStringLiteral("MissingElement"), QStringLiteral("Primary"), QColor(1, 2, 3));
    QCOMPARE(fallback, QColor(1, 2, 3));
}

void TestCkConfigInspector::testArchiveListsAndDisplay()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // CreationKitPrefs.ini overrides CreationKit.ini for display
    {
        QFile ini(dir.filePath(QStringLiteral("CreationKit.ini")));
        QVERIFY(ini.open(QIODevice::WriteOnly | QIODevice::Text));
        ini.write("[Display]\nfDefaultFOV = 70.0\n");
    }
    {
        QFile prefs(dir.filePath(QStringLiteral("CreationKitPrefs.ini")));
        QVERIFY(prefs.open(QIODevice::WriteOnly | QIODevice::Text));
        prefs.write("[Display]\nfDefaultFOV = 95.0\n");
    }

    CkConfigInspector inspector;
    QVERIFY(inspector.loadFromDirectory(dir.path()));
    QVERIFY(qFuzzyCompare(inspector.fov(), 95.0f));
}

void TestCkConfigInspector::testRealStarfieldCkConfigIfPresent()
{
    const QString detected = CkConfigInspector::detectCreationKitDirectory();
    if (detected.isEmpty()) {
        QSKIP("No installed Creation Kit directory found on machine");
    }

    CkConfigInspector inspector;
    QVERIFY(inspector.loadFromDirectory(detected));
    QVERIFY(!inspector.resourceArchiveList().isEmpty());
    QVERIFY(inspector.hasColors());

    const QColor graphBg = inspector.color(QStringLiteral("GraphBackground"), QStringLiteral("Primary"));
    QVERIFY(graphBg.isValid());
}

QTEST_MAIN(TestCkConfigInspector)
#include "test_ckconfiginspector.moc"

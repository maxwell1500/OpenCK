#include <QtTest>
#include <QApplication>

#include "../../src/view/window/thememanager.hpp"

class TestThemeManager : public QObject
{
    Q_OBJECT

private slots:
    void testThemeName();
    void testThemeFromName();
    void testApplyDarkTheme();
    void testApplyLightTheme();
    void testCurrentTheme();
    void testScaleName();
    void testScaleFromName();
    void testTouchScaleGrowsFontAndHitTargets();
    void testCompactScaleShrinksFont();
    void testScaleSurvivesThemeSwitch();
};

void TestThemeManager::testThemeName()
{
    QCOMPARE(ThemeManager::themeName(ThemeManager::Theme::Dark), QString("Dark"));
    QCOMPARE(ThemeManager::themeName(ThemeManager::Theme::Light), QString("Light"));
    QCOMPARE(ThemeManager::themeName(ThemeManager::Theme::System), QString("System"));
}

void TestThemeManager::testThemeFromName()
{
    QCOMPARE(ThemeManager::themeFromName("Dark"), ThemeManager::Theme::Dark);
    QCOMPARE(ThemeManager::themeFromName("Light"), ThemeManager::Theme::Light);
    QCOMPARE(ThemeManager::themeFromName("System"), ThemeManager::Theme::System);
    QCOMPARE(ThemeManager::themeFromName("Invalid"), ThemeManager::Theme::Dark);
    QCOMPARE(ThemeManager::themeFromName(""), ThemeManager::Theme::Dark);
}

void TestThemeManager::testApplyDarkTheme()
{
    QApplication* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(app != nullptr);
    ThemeManager::applyDarkTheme(*app);
    QCOMPARE(ThemeManager::currentTheme(), ThemeManager::Theme::Dark);
}

void TestThemeManager::testApplyLightTheme()
{
    QApplication* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(app != nullptr);
    ThemeManager::applyLightTheme(*app);
    QCOMPARE(ThemeManager::currentTheme(), ThemeManager::Theme::Light);
}

void TestThemeManager::testCurrentTheme()
{
    QApplication* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(app != nullptr);
    
    ThemeManager::applyDarkTheme(*app);
    QCOMPARE(ThemeManager::currentTheme(), ThemeManager::Theme::Dark);

    ThemeManager::applyLightTheme(*app);
    QCOMPARE(ThemeManager::currentTheme(), ThemeManager::Theme::Light);

    ThemeManager::applyDefaultTheme(*app);
    QVERIFY(ThemeManager::currentTheme() == ThemeManager::Theme::Dark
         || ThemeManager::currentTheme() == ThemeManager::Theme::Light
         || ThemeManager::currentTheme() == ThemeManager::Theme::System);
}


// ─── Density presets (Phase 13.2: Steam Deck / Compact) ───────────────────

void TestThemeManager::testScaleName()
{
    QCOMPARE(ThemeManager::scaleName(ThemeManager::Scale::Desktop), QString("Desktop"));
    QCOMPARE(ThemeManager::scaleName(ThemeManager::Scale::Touch), QString("Touch"));
    QCOMPARE(ThemeManager::scaleName(ThemeManager::Scale::Compact), QString("Compact"));
}

void TestThemeManager::testScaleFromName()
{
    QCOMPARE(ThemeManager::scaleFromName("Desktop"), ThemeManager::Scale::Desktop);
    QCOMPARE(ThemeManager::scaleFromName("Touch"), ThemeManager::Scale::Touch);
    QCOMPARE(ThemeManager::scaleFromName("Compact"), ThemeManager::Scale::Compact);
    QCOMPARE(ThemeManager::scaleFromName("bogus"), ThemeManager::Scale::Desktop);
}

void TestThemeManager::testTouchScaleGrowsFontAndHitTargets()
{
    QApplication* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    ThemeManager::applyScale(*app, ThemeManager::Scale::Desktop);
    const int basePointSize = app->font().pointSize();
    const QString baseSheet = app->styleSheet();

    ThemeManager::applyScale(*app, ThemeManager::Scale::Touch);

    // Font must actually grow: a "touch preset" that leaves the type the same
    // size is the exact failure mode this guards.
    QVERIFY2(app->font().pointSize() > basePointSize,
             "Touch density must enlarge the application font");

    // And the hit-target rules must be appended on top of the palette sheet,
    // not replace it (a replacement silently drops the theme's colours).
    QVERIFY2(app->styleSheet() != baseSheet,
             "Touch density must add hit-target rules to the stylesheet");
    QVERIFY2(app->styleSheet().contains(baseSheet),
             "Touch density must augment the theme sheet, not replace it");
    QVERIFY2(app->styleSheet().contains(QStringLiteral("min-height: 34px")),
             "Touch density must enlarge button hit targets");
    QCOMPARE(ThemeManager::currentScale(), ThemeManager::Scale::Touch);

    ThemeManager::applyScale(*app, ThemeManager::Scale::Desktop);
    QCOMPARE(app->styleSheet(), baseSheet);
}

void TestThemeManager::testCompactScaleShrinksFont()
{
    QApplication* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    ThemeManager::applyScale(*app, ThemeManager::Scale::Desktop);
    const int basePointSize = app->font().pointSize();

    ThemeManager::applyScale(*app, ThemeManager::Scale::Compact);
    QVERIFY2(app->font().pointSize() < basePointSize,
             "Compact density must shrink the application font");
    QCOMPARE(ThemeManager::currentScale(), ThemeManager::Scale::Compact);

    ThemeManager::applyScale(*app, ThemeManager::Scale::Desktop);
}

void TestThemeManager::testScaleSurvivesThemeSwitch()
{
    QApplication* app = qobject_cast<QApplication*>(QCoreApplication::instance());
    QVERIFY(app != nullptr);

    // The scale lives outside the palette: picking Touch then Light must keep
    // both the enlarged font and the touch rules.
    ThemeManager::applyScale(*app, ThemeManager::Scale::Touch);
    const int touchPointSize = app->font().pointSize();

    ThemeManager::setTheme(*app, ThemeManager::Theme::Dark);
    ThemeManager::setTheme(*app, ThemeManager::Theme::Light);

    QCOMPARE(ThemeManager::currentScale(), ThemeManager::Scale::Touch);
    QCOMPARE(app->font().pointSize(), touchPointSize);
    QVERIFY2(app->styleSheet().contains(QStringLiteral("min-height: 34px")),
             "Switching theme must not drop the density rules");

    ThemeManager::setTheme(*app, ThemeManager::Theme::Dark);
    ThemeManager::applyScale(*app, ThemeManager::Scale::Desktop);
}

#include "test_thememanager.moc"
QTEST_MAIN(TestThemeManager)

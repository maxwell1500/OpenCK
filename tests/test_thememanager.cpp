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

#include "test_thememanager.moc"
QTEST_MAIN(TestThemeManager)

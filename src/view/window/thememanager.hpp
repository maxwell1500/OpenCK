#ifndef THEMEMANAGER_HPP
#define THEMEMANAGER_HPP

#include <QString>
#include <QApplication>

class ThemeManager
{
public:
    enum class Theme { Dark, Light, System };

    static void applyTheme(QApplication& app, Theme theme);
    static void applyDarkTheme(QApplication& app);
    static void applyLightTheme(QApplication& app);
    static void applyDefaultTheme(QApplication& app);

    static Theme currentTheme();
    static QString themeName(Theme theme);
    static Theme themeFromName(const QString& name);

    static void setTheme(QApplication& app, Theme theme);

private:
    static Theme sCurrentTheme;
};

#endif // THEMEMANAGER_HPP

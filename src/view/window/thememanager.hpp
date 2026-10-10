#ifndef THEMEMANAGER_HPP
#define THEMEMANAGER_HPP

#include <QString>
#include <QApplication>

class ThemeManager
{
public:
    enum class Theme { Dark, Light, System };

    /// UI density preset. Desktop is the mouse-and-keyboard default; Touch
    /// enlarges fonts, hit targets and spacing for a Steam Deck screen; Compact
    /// trades hit target size for rows on a 1080p desktop.
    enum class Scale { Desktop, Touch, Compact };

    static void applyTheme(QApplication& app, Theme theme);
    static void applyDarkTheme(QApplication& app);
    static void applyLightTheme(QApplication& app);
    static void applyDefaultTheme(QApplication& app);

    static Theme currentTheme();
    static QString themeName(Theme theme);
    static Theme themeFromName(const QString& name);

    static void setTheme(QApplication& app, Theme theme);

    // Density. The scale is applied on top of the current theme (it augments
    // the stylesheet rather than replacing it), so switching themes keeps it.
    static Scale currentScale();
    static QString scaleName(Scale scale);
    static Scale scaleFromName(const QString& name);
    static void setScale(QApplication& app, Scale scale);
    static void applyScale(QApplication& app, Scale scale);

private:
    static void reapply(QApplication& app);
    static Theme sCurrentTheme;
    static Scale sCurrentScale;
};

#endif // THEMEMANAGER_HPP

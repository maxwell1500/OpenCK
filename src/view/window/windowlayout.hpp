#ifndef WINDOW_LAYOUT_HPP
#define WINDOW_LAYOUT_HPP

#include <QMainWindow>
#include <QSettings>

class WindowLayout
{
public:
    static void applyDefaultLayout(QMainWindow* window);
    static void saveLayout(QMainWindow* window, QSettings& settings);
    static void restoreLayout(QMainWindow* window, QSettings& settings);
};

#endif // WINDOW_LAYOUT_HPP
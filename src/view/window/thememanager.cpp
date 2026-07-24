#include "thememanager.hpp"
#include "logger.hpp"
#include "filepaths.hpp"

#include <QSettings>
#include <QCoreApplication>
#include <QPalette>

ThemeManager::Theme ThemeManager::sCurrentTheme = Theme::Dark;

ThemeManager::Theme ThemeManager::currentTheme()
{
    return sCurrentTheme;
}

QString ThemeManager::themeName(Theme theme)
{
    switch (theme) {
    case Theme::Dark: return "Dark";
    case Theme::Light: return "Light";
    case Theme::System: return "System";
    }
    return "Dark";
}

ThemeManager::Theme ThemeManager::themeFromName(const QString& name)
{
    if (name == "Light") return Theme::Light;
    if (name == "System") return Theme::System;
    return Theme::Dark;
}

void ThemeManager::applyTheme(QApplication& app, Theme theme)
{
    sCurrentTheme = theme;
    switch (theme) {
    case Theme::Dark:
        applyDarkTheme(app);
        break;
    case Theme::Light:
    case Theme::System:
        applyLightTheme(app);
        break;
    }
}

void ThemeManager::setTheme(QApplication& app, Theme theme)
{
    applyTheme(app, theme);

    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("OpenCK");
    conf.setValue("Theme", themeName(theme));
    conf.endGroup();
    conf.sync();

    LOG_INFO(QString("Theme changed to: %1").arg(themeName(theme)));
}

void ThemeManager::applyDarkTheme(QApplication& app)
{
    sCurrentTheme = Theme::Dark;
    LOG_INFO("Applying dark theme");
    app.setStyleSheet(R"(
        QMainWindow {
            background-color: #2b2b2b;
        }
        QMenuBar {
            background-color: #3c3c3c;
            color: #d4d4d4;
        }
        QMenuBar::item:selected {
            background-color: #4a4a4a;
        }
        QMenu {
            background-color: #3c3c3c;
            color: #d4d4d4;
        }
        QMenu::item:selected {
            background-color: #4a4a4a;
        }
        QToolBar {
            background-color: #3c3c3c;
            border-bottom: 1px solid #555;
        }
        QDockWidget {
            titlebar-close-icon: none;
        }
        QDockWidget::title {
            background-color: #3c3c3c;
            color: #d4d4d4;
            padding: 4px;
        }
        QTabWidget::pane {
            border: 1px solid #555;
            background-color: #2b2b2b;
        }
        QTabBar::tab {
            background-color: #3c3c3c;
            color: #d4d4d4;
            padding: 6px 12px;
        }
        QTabBar::tab:selected {
            background-color: #2b2b2b;
        }
        QTableWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
            gridline-color: #555;
            selection-background-color: #264f78;
        }
        QTableWidget::item:selected {
            background-color: #264f78;
        }
        QHeaderView::section {
            background-color: #3c3c3c;
            color: #d4d4d4;
            padding: 4px;
            border: 1px solid #555;
        }
        QTreeView, QTreeWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
            alternate-background-color: #252525;
        }
        QTreeView::item:selected, QTreeWidget::item:selected {
            background-color: #264f78;
        }
        QLineEdit, QTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #1e1e1e;
            color: #d4d4d4;
            border: 1px solid #555;
            padding: 2px;
        }
        QPushButton {
            background-color: #3c3c3c;
            color: #d4d4d4;
            border: 1px solid #555;
            padding: 4px 8px;
        }
        QPushButton:hover {
            background-color: #4a4a4a;
        }
        QPushButton:pressed {
            background-color: #555;
        }
        QLabel {
            color: #d4d4d4;
        }
        QGroupBox {
            color: #d4d4d4;
            border: 1px solid #555;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title {
            color: #d4d4d4;
        }
        QProgressBar {
            border: 1px solid #555;
            text-align: center;
            color: #d4d4d4;
        }
        QProgressBar::chunk {
            background-color: #0078d4;
        }
        QScrollBar:vertical {
            background-color: #2b2b2b;
            width: 12px;
        }
        QScrollBar::handle:vertical {
            background-color: #555;
            min-height: 20px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QStatusBar {
            background-color: #007acc;
            color: #ffffff;
        }
    )");
}

void ThemeManager::applyLightTheme(QApplication& app)
{
    sCurrentTheme = Theme::Light;
    LOG_INFO("Applying light theme");
    app.setStyleSheet("");
}

void ThemeManager::applyDefaultTheme(QApplication& app)
{
    sCurrentTheme = Theme::System;
    LOG_INFO("Applying default theme");
    app.setStyleSheet("");
}

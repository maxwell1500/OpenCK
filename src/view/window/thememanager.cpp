#include "thememanager.hpp"
#include "logger.hpp"
#include "filepaths.hpp"

#include <QSettings>
#include <QCoreApplication>
#include <QApplication>
#include <QPalette>
#include <QFont>

ThemeManager::Theme ThemeManager::sCurrentTheme = Theme::Dark;
ThemeManager::Scale ThemeManager::sCurrentScale = Scale::Desktop;

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
    reapply(app);

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

// ─── Density presets ──────────────────────────────────────────────────────
//
// Scale is a stylesheet *addition*, never a replacement: applyTheme() installs
// the colour sheet and applyScale() appends the hit-target rules that the
// current scale calls for. reapply() re-runs the palette and then re-appends,
// so a theme switch never loses the density choice.

ThemeManager::Scale ThemeManager::currentScale()
{
    return sCurrentScale;
}

QString ThemeManager::scaleName(Scale scale)
{
    switch (scale) {
    case Scale::Desktop: return "Desktop";
    case Scale::Touch:   return "Touch";
    case Scale::Compact: return "Compact";
    }
    return "Desktop";
}

ThemeManager::Scale ThemeManager::scaleFromName(const QString& name)
{
    if (name == "Touch")   return Scale::Touch;
    if (name == "Compact") return Scale::Compact;
    return Scale::Desktop;
}

void ThemeManager::applyScale(QApplication& app, Scale scale)
{
    sCurrentScale = scale;

    QFont f = app.font();
    if (f.pointSize() <= 0)
        f.setPointSize(9);
    switch (scale)
    {
    case Scale::Touch:
        // A 1280x800 Steam Deck screen is ~7" across; a mouse-sized hit
        // target is a miss, so enlarge text and controls together.
        f.setPointSize(qRound(f.pointSize() * 1.5));
        break;
    case Scale::Compact:
        f.setPointSize(qMax(7, qRound(f.pointSize() * 0.9)));
        break;
    case Scale::Desktop:
    default:
        break;
    }
    app.setFont(f);

    reapply(app);
    LOG_INFO(QString("UI scale set to: %1").arg(scaleName(scale)));
}

void ThemeManager::setScale(QApplication& app, Scale scale)
{
    applyScale(app, scale);

    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("OpenCK");
    conf.setValue("UiScale", scaleName(scale));
    conf.endGroup();
    conf.sync();
}

void ThemeManager::reapply(QApplication& app)
{
    // Rebuild the sheet: palette first, then this scale's additions.
    switch (sCurrentTheme)
    {
    case Theme::Dark:
        applyDarkTheme(app);
        break;
    case Theme::Light:
    case Theme::System:
        applyLightTheme(app);
        break;
    }

    if (sCurrentScale != Scale::Desktop)
    {
        // The sizeable block: every widget class that carries its own padding
        // gets bigger padding, taller rows and a fatter button. Keep this in
        // one place so the presets stay comparable.
        const QByteArray touchSheet = R"(
            QPushButton, QToolButton {
                min-height: 34px;
                padding: 8px 16px;
                font-size: 15px;
            }
            QMenuBar { padding: 4px; }
            QMenuBar::item { padding: 8px 12px; }
            QMenu::item { padding: 10px 28px 10px 20px; min-height: 30px; }
            QTabBar::tab { padding: 10px 18px; min-width: 90px; }
            QTreeView, QTreeWidget, QTableView, QTableWidget { font-size: 15px; }
            QTreeView::item, QTreeWidget::item { min-height: 30px; }
            QTableView, QTableWidget { gridline-color: #666; }
            QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { min-height: 30px; padding: 4px; font-size: 15px; }
            QScrollBar:vertical { width: 24px; }
            QScrollBar:horizontal { height: 24px; }
            QStatusBar { min-height: 28px; }
        )";
        const QByteArray compactSheet = R"(
            QPushButton, QToolButton { min-height: 20px; padding: 2px 6px; }
            QTreeView::item, QTreeWidget::item { min-height: 0px; }
            QTabBar::tab { padding: 3px 8px; }
        )";

        if (sCurrentScale == Scale::Touch)
            app.setStyleSheet(app.styleSheet() + QString::fromUtf8(touchSheet));
        else
            app.setStyleSheet(app.styleSheet() + QString::fromUtf8(compactSheet));
    }
}

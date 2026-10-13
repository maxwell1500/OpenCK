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

    // Base palette for widgets and custom delegates
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(0x1e, 0x1e, 0x1e));
    pal.setColor(QPalette::WindowText, QColor(0xcc, 0xcc, 0xcc));
    pal.setColor(QPalette::Base, QColor(0x1e, 0x1e, 0x1e));
    pal.setColor(QPalette::AlternateBase, QColor(0x22, 0x22, 0x23));
    pal.setColor(QPalette::ToolTipBase, QColor(0x25, 0x25, 0x26));
    pal.setColor(QPalette::ToolTipText, QColor(0xcc, 0xcc, 0xcc));
    pal.setColor(QPalette::Text, QColor(0xcc, 0xcc, 0xcc));
    pal.setColor(QPalette::Button, QColor(0x33, 0x33, 0x33));
    pal.setColor(QPalette::ButtonText, QColor(0xcc, 0xcc, 0xcc));
    pal.setColor(QPalette::BrightText, Qt::red);
    pal.setColor(QPalette::Link, QColor(0x00, 0x78, 0xd4));
    pal.setColor(QPalette::Highlight, QColor(0x09, 0x47, 0x71));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    pal.setColor(QPalette::Disabled, QPalette::Text, QColor(0x65, 0x65, 0x65));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x65, 0x65, 0x65));
    pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x65, 0x65, 0x65));
    app.setPalette(pal);

    app.setStyleSheet(R"(
        QMainWindow, QDialog {
            background-color: #1e1e1e;
            color: #cccccc;
        }
        QWidget {
            background-color: #1e1e1e;
            color: #cccccc;
            selection-background-color: #094771;
            selection-color: #ffffff;
        }
        QMenuBar {
            background-color: #1e1e1e;
            color: #cccccc;
            border-bottom: 1px solid #2d2d30;
            padding: 2px;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 4px 8px;
            border-radius: 3px;
        }
        QMenuBar::item:selected {
            background-color: #2d2d30;
            color: #ffffff;
        }
        QMenu {
            background-color: #252526;
            color: #cccccc;
            border: 1px solid #3e3e42;
            border-radius: 4px;
            padding: 4px;
        }
        QMenu::item {
            padding: 4px 24px 4px 20px;
            border-radius: 2px;
        }
        QMenu::item:selected {
            background-color: #094771;
            color: #ffffff;
        }
        QMenu::separator {
            height: 1px;
            background-color: #3e3e42;
            margin: 4px 6px;
        }
        QToolBar {
            background-color: #1e1e1e;
            border-bottom: 1px solid #2d2d30;
            border-top: none;
            border-left: none;
            border-right: none;
            spacing: 4px;
            padding: 2px 4px;
        }
        QToolButton {
            background-color: transparent;
            color: #cccccc;
            border: none;
            border-radius: 3px;
            padding: 3px 5px;
        }
        QToolButton:hover {
            background-color: #2d2d30;
            color: #ffffff;
        }
        QToolButton:pressed, QToolButton:checked {
            background-color: #3e3e42;
            color: #ffffff;
        }
        QDockWidget {
            titlebar-close-icon: none;
        }
        QDockWidget::title {
            background-color: #252526;
            color: #cccccc;
            padding: 4px 8px;
            border-bottom: 1px solid #2d2d30;
        }
        QTabWidget::pane {
            border: 1px solid #2d2d30;
            background-color: #1e1e1e;
            top: -1px;
        }
        QTabBar::tab {
            background-color: #252526;
            color: #969696;
            padding: 5px 12px;
            border: 1px solid #2d2d30;
            border-bottom: none;
            border-top-left-radius: 3px;
            border-top-right-radius: 3px;
            margin-right: 2px;
        }
        QTabBar::tab:hover {
            background-color: #2d2d30;
            color: #cccccc;
        }
        QTabBar::tab:selected {
            background-color: #1e1e1e;
            color: #ffffff;
            border-top: 2px solid #0078d4;
        }
        QTableWidget, QTableView {
            background-color: #1e1e1e;
            color: #cccccc;
            gridline-color: #282829;
            alternate-background-color: #222223;
            selection-background-color: #094771;
            selection-color: #ffffff;
            border: 1px solid #2d2d30;
        }
        QTableWidget::item, QTableView::item {
            padding: 3px 4px;
        }
        QTableWidget::item:selected, QTableView::item:selected {
            background-color: #094771;
            color: #ffffff;
        }
        QTableWidget::item:hover, QTableView::item:hover {
            background-color: #2a2d2e;
        }
        QHeaderView::section {
            background-color: #252526;
            color: #aaaaaa;
            padding: 4px 8px;
            border: none;
            border-right: 1px solid #2d2d30;
            border-bottom: 1px solid #2d2d30;
        }
        QHeaderView::section:hover {
            background-color: #2d2d30;
            color: #ffffff;
        }
        QTreeView, QTreeWidget, QListView, QListWidget {
            background-color: #1e1e1e;
            color: #cccccc;
            alternate-background-color: #222223;
            selection-background-color: #094771;
            selection-color: #ffffff;
            border: 1px solid #2d2d30;
        }
        QTreeView::item, QTreeWidget::item, QListView::item, QListWidget::item {
            padding: 3px 4px;
        }
        QTreeView::item:selected, QTreeWidget::item:selected, QListView::item:selected, QListWidget::item:selected {
            background-color: #094771;
            color: #ffffff;
        }
        QTreeView::item:hover, QTreeWidget::item:hover, QListView::item:hover, QListWidget::item:hover {
            background-color: #2a2d2e;
        }
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #1e1e1e;
            color: #cccccc;
            border: 1px solid #3e3e42;
            border-radius: 3px;
            padding: 3px 6px;
            selection-background-color: #094771;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
            border: 1px solid #0078d4;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 18px;
            border-left: 1px solid #2d2d30;
        }
        QComboBox QAbstractItemView {
            background-color: #252526;
            color: #cccccc;
            border: 1px solid #3e3e42;
            selection-background-color: #094771;
        }
        QPushButton {
            background-color: #333333;
            color: #cccccc;
            border: 1px solid #3e3e42;
            border-radius: 3px;
            padding: 4px 12px;
        }
        QPushButton:hover {
            background-color: #3e3e42;
            border-color: #4f4f54;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: #282828;
            border-color: #0078d4;
        }
        QPushButton:disabled {
            background-color: #252526;
            color: #656565;
            border-color: #2d2d30;
        }
        QLabel {
            color: #cccccc;
        }
        QGroupBox {
            color: #cccccc;
            border: 1px solid #333333;
            border-radius: 4px;
            margin-top: 10px;
            padding-top: 10px;
            padding-left: 6px;
            padding-right: 6px;
            padding-bottom: 6px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 0 4px;
            color: #9cdcfe;
        }
        QCheckBox, QRadioButton {
            color: #cccccc;
            spacing: 6px;
        }
        QCheckBox:hover, QRadioButton:hover {
            color: #ffffff;
        }
        QProgressBar {
            border: 1px solid #3e3e42;
            border-radius: 3px;
            text-align: center;
            color: #cccccc;
            background-color: #1e1e1e;
        }
        QProgressBar::chunk {
            background-color: #0078d4;
            border-radius: 2px;
        }
        QScrollBar:vertical {
            background-color: transparent;
            width: 10px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background-color: #424242;
            min-height: 20px;
            border-radius: 5px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #555555;
        }
        QScrollBar::handle:vertical:pressed {
            background-color: #686868;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
            height: 0px;
        }
        QScrollBar:horizontal {
            background-color: transparent;
            height: 10px;
            margin: 0px;
        }
        QScrollBar::handle:horizontal {
            background-color: #424242;
            min-width: 20px;
            border-radius: 5px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #555555;
        }
        QScrollBar::handle:horizontal:pressed {
            background-color: #686868;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: transparent;
            width: 0px;
        }
        QSplitter::handle {
            background-color: #252526;
        }
        QSplitter::handle:hover {
            background-color: #0078d4;
        }
        QSplitter::handle:horizontal {
            width: 3px;
        }
        QSplitter::handle:vertical {
            height: 3px;
        }
        QToolTip {
            background-color: #252526;
            color: #cccccc;
            border: 1px solid #3e3e42;
            border-radius: 3px;
            padding: 4px 8px;
        }
        QStatusBar {
            background-color: #181818;
            color: #858585;
            border-top: 1px solid #2d2d30;
            min-height: 24px;
        }
        QStatusBar QLabel {
            color: #9d9d9d;
            background-color: #252526;
            border: 1px solid #2d2d30;
            border-radius: 3px;
            padding: 2px 8px;
            margin: 1px 2px;
        }

        /* Qt Advanced Docking System (ADS) */
        ads--CDockContainerWidget {
            background-color: #1e1e1e;
        }
        ads--CDockAreaWidget {
            background-color: #1e1e1e;
            border: 1px solid #2d2d30;
        }
        ads--CDockAreaTitleBar {
            background-color: #252526;
            border-bottom: 1px solid #2d2d30;
            min-height: 22px;
        }
        ads--CDockWidgetTab {
            background-color: #252526;
            color: #969696;
            border: none;
            border-right: 1px solid #1e1e1e;
            padding: 4px 10px;
        }
        ads--CDockWidgetTab:hover {
            background-color: #2d2d30;
            color: #cccccc;
        }
        ads--CDockWidgetTab[activeTab="true"] {
            background-color: #1e1e1e;
            color: #ffffff;
            border-bottom: 2px solid #0078d4;
        }
        ads--CDockWidgetTab QLabel {
            color: inherit;
            background: transparent;
        }
        ads--CTitleBarButton {
            background: transparent;
            border: none;
            border-radius: 2px;
            padding: 2px;
        }
        ads--CTitleBarButton:hover {
            background-color: #3e3e42;
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

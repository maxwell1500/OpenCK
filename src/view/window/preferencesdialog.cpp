#include "preferencesdialog.hpp"

#include "logger.hpp"
#include "thememanager.hpp"
#include "filepaths.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QSettings>
#include <QCoreApplication>
#include <QMessageBox>
#include <QApplication>

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Preferences");
    setMinimumSize(500, 400);
    setupUI();
    loadSettings();
}

void PreferencesDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    auto* generalGroup = new QGroupBox("General Settings");
    auto* generalLayout = new QFormLayout(generalGroup);

    mDataDirEdit = new QLineEdit();
    mDataDirEdit->setPlaceholderText("Path to game Data directory...");
    auto* browseBtn = new QPushButton("Browse...");
    auto* dirLayout = new QHBoxLayout();
    dirLayout->addWidget(mDataDirEdit, 1);
    dirLayout->addWidget(browseBtn);

    auto* dirContainer = new QWidget();
    dirContainer->setLayout(dirLayout);
    generalLayout->addRow("Data Directory:", dirContainer);
    connect(browseBtn, &QPushButton::clicked, this, &PreferencesDialog::browseDataDir);

    mGameCombo = new QComboBox();
    mGameCombo->addItem("Auto Detect", -1);
    mGameCombo->addItem("Morrowind", 1);
    mGameCombo->addItem("Oblivion", 2);
    mGameCombo->addItem("Skyrim", 3);
    mGameCombo->addItem("Skyrim Special Edition", 4);
    mGameCombo->addItem("Skyrim Anniversary Edition", 5);
    mGameCombo->addItem("Fallout 3", 6);
    mGameCombo->addItem("Fallout: New Vegas", 7);
    mGameCombo->addItem("Fallout 4", 8);
    mGameCombo->addItem("Starfield", 9);
    generalLayout->addRow("Game:", mGameCombo);

    mLanguageCombo = new QComboBox();
    mLanguageCombo->addItem("English");
    mLanguageCombo->addItem("French");
    mLanguageCombo->addItem("German");
    mLanguageCombo->addItem("Italian");
    mLanguageCombo->addItem("Spanish");
    mLanguageCombo->addItem("Japanese");
    mLanguageCombo->addItem("Polish");
    mLanguageCombo->addItem("Russian");
    mLanguageCombo->addItem("Chinese (Traditional)");
    mLanguageCombo->addItem("Chinese (Simplified)");
    mLanguageCombo->addItem("Korean");
    generalLayout->addRow("Language:", mLanguageCombo);

    mainLayout->addWidget(generalGroup);

    auto* appearanceGroup = new QGroupBox("Appearance");
    auto* appearanceLayout = new QFormLayout(appearanceGroup);

    mThemeCombo = new QComboBox();
    mThemeCombo->addItem("Dark", "Dark");
    mThemeCombo->addItem("Light", "Light");
    mThemeCombo->addItem("System", "System");
    appearanceLayout->addRow("Theme:", mThemeCombo);

    mainLayout->addWidget(appearanceGroup);

    auto* miscGroup = new QGroupBox("Miscellaneous");
    auto* miscLayout = new QFormLayout(miscGroup);

    mAutoSaveSpin = new QSpinBox();
    mAutoSaveSpin->setRange(0, 60);
    mAutoSaveSpin->setSuffix(" minutes");
    mAutoSaveSpin->setSpecialValueText("Disabled");
    miscLayout->addRow("Auto Save Every:", mAutoSaveSpin);

    mSkipCellLoadCheck = new QCheckBox("Skip initial cell load on editor start");
    miscLayout->addRow("", mSkipCellLoadCheck);

    mainLayout->addWidget(miscGroup);
    mainLayout->addStretch();

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* saveBtn = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);

    mainLayout->addLayout(buttonLayout);

    connect(saveBtn, &QPushButton::clicked, this, &PreferencesDialog::saveSettings);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void PreferencesDialog::loadSettings()
{
    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("OpenCK");

    mDataDirEdit->setText(conf.value("DataDirectory", "").toString());
    int gameId = conf.value("GameId", -1).toInt();
    mGameCombo->setCurrentIndex(mGameCombo->findData(gameId));
    mAutoSaveSpin->setValue(conf.value("AutoSaveMinutes", 0).toInt());
    mSkipCellLoadCheck->setChecked(conf.value("SkipInitialCellLoad", false).toBool());

    QString language = conf.value("Language", "English").toString();
    int langIndex = mLanguageCombo->findText(language);
    if (langIndex >= 0)
        mLanguageCombo->setCurrentIndex(langIndex);

    QString theme = conf.value("Theme", "Dark").toString();
    int themeIndex = mThemeCombo->findData(theme);
    if (themeIndex >= 0)
        mThemeCombo->setCurrentIndex(themeIndex);

    conf.endGroup();
}

void PreferencesDialog::saveSettings()
{
    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("OpenCK");

    conf.setValue("DataDirectory", mDataDirEdit->text());
    conf.setValue("GameId", mGameCombo->currentData().toInt());
    conf.setValue("AutoSaveMinutes", mAutoSaveSpin->value());
    conf.setValue("SkipInitialCellLoad", mSkipCellLoadCheck->isChecked());
    conf.setValue("Language", mLanguageCombo->currentText());
    conf.setValue("Theme", mThemeCombo->currentData().toString());

    conf.endGroup();
    conf.sync();

    QString themeName = mThemeCombo->currentData().toString();
    ThemeManager::Theme theme = ThemeManager::themeFromName(themeName);
    auto* app = qobject_cast<QApplication*>(QApplication::instance());
    if (app) {
        ThemeManager::applyTheme(*app, theme);
    }

    LOG_INFO("Preferences saved");
    accept();
}

void PreferencesDialog::browseDataDir()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Game Data Directory",
        mDataDirEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dir.isEmpty())
    {
        mDataDirEdit->setText(dir);
    }
}
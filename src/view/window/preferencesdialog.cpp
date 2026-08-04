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
#include <QSplitter>
#include <QListWidgetItem>
#include <QHeaderView>

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Preferences");
    setMinimumSize(720, 480);
    setupUI();
    loadSettings();
}

void PreferencesDialog::setupUI()
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    outer->addWidget(splitter, 1);

    mCategoryTree = new QTreeWidget();
    mCategoryTree->setHeaderHidden(true);
    mCategoryTree->setRootIsDecorated(false);
    mCategoryTree->setMinimumWidth(180);
    mCategoryTree->setMaximumWidth(260);
    splitter->addWidget(mCategoryTree);

    mPageStack = new QStackedWidget();
    splitter->addWidget(mPageStack);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({200, 520});

    struct CatEntry { const char* label; QWidget* (PreferencesDialog::*factory)(); };
    const CatEntry cats[] = {
        {"General",  &PreferencesDialog::createGeneralPage},
        {"Display",  &PreferencesDialog::createDisplayPage},
        {"Edit",     &PreferencesDialog::createEditPage},
        {"Sound",    &PreferencesDialog::createSoundPage},
        {"Archive",  &PreferencesDialog::createArchivePage},
        {"Papyrus",  &PreferencesDialog::createPapyrusPage},
        {"LOD",      &PreferencesDialog::createLODPage},
        {"Network",  &PreferencesDialog::createNetworkPage},
    };

    for (const auto& c : cats)
    {
        auto* item = new QTreeWidgetItem();
        item->setText(0, c.label);
        mCategoryTree->addTopLevelItem(item);
        QWidget* page = (this->*c.factory)();
        mPageStack->addWidget(page);
    }

    mCategoryTree->setCurrentItem(mCategoryTree->topLevelItem(0));
    mCategoryTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect(mCategoryTree, &QTreeWidget::currentItemChanged,
        this, [this](QTreeWidgetItem* cur, QTreeWidgetItem*) {
            if (!cur) return;
            int idx = mCategoryTree->indexOfTopLevelItem(cur);
            if (idx >= 0 && idx < mPageStack->count())
                mPageStack->setCurrentIndex(idx);
        });

    auto* buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    auto* saveBtn   = new QPushButton("Save");
    auto* cancelBtn = new QPushButton("Cancel");
    buttonRow->addWidget(saveBtn);
    buttonRow->addWidget(cancelBtn);
    outer->addLayout(buttonRow);

    connect(saveBtn,   &QPushButton::clicked, this, &PreferencesDialog::saveSettings);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QWidget* PreferencesDialog::createGeneralPage()
{
    auto* page = new QWidget();
    auto* form = new QFormLayout(page);
    form->setContentsMargins(12, 12, 12, 12);

    auto* generalGroup = new QGroupBox("General");
    auto* generalForm  = new QFormLayout(generalGroup);

    mDataDirEdit = new QLineEdit();
    mDataDirEdit->setPlaceholderText("Path to game Data directory...");
    auto* browseBtn = new QPushButton("Browse...");
    auto* dirLayout = new QHBoxLayout();
    dirLayout->setContentsMargins(0,0,0,0);
    dirLayout->addWidget(mDataDirEdit, 1);
    dirLayout->addWidget(browseBtn);
    auto* dirContainer = new QWidget();
    dirContainer->setLayout(dirLayout);
    generalForm->addRow("Data Directory:", dirContainer);
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
    generalForm->addRow("Game:", mGameCombo);

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
    generalForm->addRow("Language:", mLanguageCombo);

    form->addRow(generalGroup);

    auto* appearanceGroup = new QGroupBox("Appearance");
    auto* appearanceForm  = new QFormLayout(appearanceGroup);
    mThemeCombo = new QComboBox();
    mThemeCombo->addItem("Dark", "Dark");
    mThemeCombo->addItem("Light", "Light");
    mThemeCombo->addItem("System", "System");
    appearanceForm->addRow("Theme:", mThemeCombo);
    form->addRow(appearanceGroup);

    return page;
}

QWidget* PreferencesDialog::createDisplayPage()
{
    auto* page = new QWidget();
    auto* form = new QFormLayout(page);
    form->setContentsMargins(12, 12, 12, 12);

    auto* group = new QGroupBox("Display");
    auto* gform = new QFormLayout(group);

    mFovSpin = new QDoubleSpinBox();
    mFovSpin->setRange(10.0, 170.0);
    mFovSpin->setSingleStep(1.0);
    mFovSpin->setDecimals(1);
    mFovSpin->setSuffix(" deg");
    gform->addRow("Field of View:", mFovSpin);

    mCameraSpeedSpin = new QDoubleSpinBox();
    mCameraSpeedSpin->setRange(0.0, 1000.0);
    mCameraSpeedSpin->setSingleStep(0.5);
    mCameraSpeedSpin->setDecimals(2);
    gform->addRow("Camera Speed:", mCameraSpeedSpin);

    mRenderDistanceSpin = new QSpinBox();
    mRenderDistanceSpin->setRange(128, 65536);
    mRenderDistanceSpin->setSingleStep(128);
    mRenderDistanceSpin->setSuffix(" u");
    gform->addRow("Render Distance:", mRenderDistanceSpin);

    mVsyncCheck = new QCheckBox("Enable vertical sync");
    gform->addRow("", mVsyncCheck);

    form->addRow(group);
    return page;
}

QWidget* PreferencesDialog::createEditPage()
{
    auto* page = new QWidget();
    auto* form = new QFormLayout(page);
    form->setContentsMargins(12, 12, 12, 12);

    auto* group = new QGroupBox("Edit");
    auto* gform = new QFormLayout(group);

    mAutoSaveSpin = new QSpinBox();
    mAutoSaveSpin->setRange(0, 60);
    mAutoSaveSpin->setSuffix(" minutes");
    mAutoSaveSpin->setSpecialValueText("Disabled");
    gform->addRow("Auto Save Every:", mAutoSaveSpin);

    mUndoDepthSpin = new QSpinBox();
    mUndoDepthSpin->setRange(0, 100000);
    mUndoDepthSpin->setSingleStep(10);
    mUndoDepthSpin->setSpecialValueText("Unlimited");
    gform->addRow("Undo Depth:", mUndoDepthSpin);

    mSnapSpin = new QDoubleSpinBox();
    mSnapSpin->setRange(0.0, 4096.0);
    mSnapSpin->setSingleStep(1.0);
    mSnapSpin->setDecimals(3);
    mSnapSpin->setSuffix(" u");
    gform->addRow("Default Snap:", mSnapSpin);

    mWarnMoveStaticCheck = new QCheckBox("Warn when moving static objects");
    gform->addRow("", mWarnMoveStaticCheck);

    mSkipCellLoadCheck = new QCheckBox("Skip initial cell load on editor start");
    gform->addRow("", mSkipCellLoadCheck);

    form->addRow(group);
    return page;
}

QWidget* PreferencesDialog::createSoundPage()
{
    auto* page = new QWidget();
    auto* form = new QFormLayout(page);
    form->setContentsMargins(12, 12, 12, 12);

    auto* group = new QGroupBox("Sound");
    auto* gform = new QFormLayout(group);

    mVolumeSpin = new QSpinBox();
    mVolumeSpin->setRange(0, 100);
    mVolumeSpin->setSingleStep(5);
    mVolumeSpin->setSuffix("%");
    gform->addRow("Volume:", mVolumeSpin);

    mWwiseCodecSpin = new QSpinBox();
    mWwiseCodecSpin->setRange(0, 0xFFFF);
    mWwiseCodecSpin->setDisplayIntegerBase(16);
    mWwiseCodecSpin->setPrefix("0x");
    gform->addRow("Wwise Codec ID:", mWwiseCodecSpin);

    form->addRow(group);
    return page;
}

QWidget* PreferencesDialog::createArchivePage()
{
    auto* page = new QWidget();
    auto* form = new QFormLayout(page);
    form->setContentsMargins(12, 12, 12, 12);

    auto* group = new QGroupBox("Loaded Archives");
    auto* vlay  = new QVBoxLayout(group);
    mArchiveList = new QListWidget();
    mArchiveList->setSelectionMode(QAbstractItemView::NoSelection);
    mArchiveList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vlay->addWidget(mArchiveList);

    auto* hint = new QLabel("Archives loaded by the active document are listed here (read-only).");
    hint->setWordWrap(true);
    vlay->addWidget(hint);

    form->addRow(group);
    return page;
}

QWidget* PreferencesDialog::createPapyrusPage()
{
    auto* page = new QWidget();
    auto* form = new QFormLayout(page);
    form->setContentsMargins(12, 12, 12, 12);

    auto* group = new QGroupBox("Papyrus");
    auto* gform = new QFormLayout(group);

    mCompilerDirEdit = new QLineEdit();
    mCompilerDirEdit->setPlaceholderText("Papyrus compiler directory...");
    auto* compBrowse = new QPushButton("Browse...");
    auto* compLayout = new QHBoxLayout();
    compLayout->setContentsMargins(0,0,0,0);
    compLayout->addWidget(mCompilerDirEdit, 1);
    compLayout->addWidget(compBrowse);
    auto* compCont = new QWidget();
    compCont->setLayout(compLayout);
    gform->addRow("Compiler Folder:", compCont);
    connect(compBrowse, &QPushButton::clicked, this, &PreferencesDialog::browseCompilerDir);

    mScriptSourceDirEdit = new QLineEdit();
    mScriptSourceDirEdit->setPlaceholderText("Script source directory...");
    auto* srcBrowse = new QPushButton("Browse...");
    auto* srcLayout = new QHBoxLayout();
    srcLayout->setContentsMargins(0,0,0,0);
    srcLayout->addWidget(mScriptSourceDirEdit, 1);
    srcLayout->addWidget(srcBrowse);
    auto* srcCont = new QWidget();
    srcCont->setLayout(srcLayout);
    gform->addRow("Script Source Folder:", srcCont);
    connect(srcBrowse, &QPushButton::clicked, this, &PreferencesDialog::browseScriptSourceDir);

    form->addRow(group);
    return page;
}

QWidget* PreferencesDialog::createLODPage()
{
    auto* page = new QWidget();
    auto* form = new QFormLayout(page);
    form->setContentsMargins(12, 12, 12, 12);

    auto* group = new QGroupBox("Level of Detail");
    auto* gform = new QFormLayout(group);

    mMeshLODCheck = new QCheckBox("Enable mesh LOD streaming");
    gform->addRow("", mMeshLODCheck);

    mDynamicLODCheck = new QCheckBox("Enable dynamic LOD");
    gform->addRow("", mDynamicLODCheck);

    form->addRow(group);
    return page;
}

QWidget* PreferencesDialog::createNetworkPage()
{
    auto* page = new QWidget();
    auto* form = new QFormLayout(page);
    form->setContentsMargins(12, 12, 12, 12);

    auto* group = new QGroupBox("Network / Version Control");
    auto* gform = new QFormLayout(group);

    mVersionControlCheck = new QCheckBox("Enable version control integration");
    mVersionControlCheck->setEnabled(false);
    gform->addRow("", mVersionControlCheck);

    mVcServerEdit = new QLineEdit();
    mVcServerEdit->setPlaceholderText("Not implemented");
    mVcServerEdit->setEnabled(false);
    gform->addRow("VC Server:", mVcServerEdit);

    auto* note = new QLabel("Version control: OpenCK uses Git or Perforce for Check In/Out (configured per-project). The VC settings below are reserved for future use.");
    note->setWordWrap(true);
    gform->addRow("", note);

    form->addRow(group);
    return page;
}

void PreferencesDialog::changePage(int index)
{
    if (index >= 0 && index < mPageStack->count())
        mPageStack->setCurrentIndex(index);
}

void PreferencesDialog::loadSettings()
{
    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);

    conf.beginGroup("General");
    mDataDirEdit->setText(conf.value("DataDirectory", "").toString());
    int gameId = conf.value("GameId", -1).toInt();
    mGameCombo->setCurrentIndex(mGameCombo->findData(gameId));
    QString language = conf.value("Language", "English").toString();
    int langIndex = mLanguageCombo->findText(language);
    if (langIndex >= 0)
        mLanguageCombo->setCurrentIndex(langIndex);
    QString theme = conf.value("Theme", "Dark").toString();
    int themeIndex = mThemeCombo->findData(theme);
    if (themeIndex >= 0)
        mThemeCombo->setCurrentIndex(themeIndex);
    conf.endGroup();

    conf.beginGroup("Display");
    mFovSpin->setValue(conf.value("FOV", 75.0).toDouble());
    mCameraSpeedSpin->setValue(conf.value("CameraSpeed", 1.0).toDouble());
    mRenderDistanceSpin->setValue(conf.value("RenderDistance", 4096).toInt());
    mVsyncCheck->setChecked(conf.value("VSync", true).toBool());
    conf.endGroup();

    conf.beginGroup("Edit");
    mAutoSaveSpin->setValue(conf.value("AutoSaveMinutes", 0).toInt());
    mUndoDepthSpin->setValue(conf.value("UndoDepth", 0).toInt());
    mSnapSpin->setValue(conf.value("DefaultSnap", 1.0).toDouble());
    mWarnMoveStaticCheck->setChecked(conf.value("WarnMoveStatic", true).toBool());
    mSkipCellLoadCheck->setChecked(conf.value("SkipInitialCellLoad", false).toBool());
    conf.endGroup();

    conf.beginGroup("Sound");
    mVolumeSpin->setValue(conf.value("Volume", 100).toInt());
    mWwiseCodecSpin->setValue(conf.value("WwiseCodecID", 0x190).toInt());
    conf.endGroup();

    conf.beginGroup("Archive");
    mArchiveList->clear();
    int n = conf.beginReadArray("Archives");
    for (int i = 0; i < n; ++i) {
        conf.setArrayIndex(i);
        mArchiveList->addItem(conf.value("path").toString());
    }
    conf.endArray();
    if (mArchiveList->count() == 0)
        mArchiveList->addItem("(no archives loaded)");
    conf.endGroup();

    conf.beginGroup("Papyrus");
    mCompilerDirEdit->setText(conf.value("sScriptCompilerFolder", "").toString());
    mScriptSourceDirEdit->setText(conf.value("sScriptSourceFolder", "").toString());
    conf.endGroup();

    conf.beginGroup("LOD");
    mMeshLODCheck->setChecked(conf.value("bEnableMeshLODStreaming", false).toBool());
    mDynamicLODCheck->setChecked(conf.value("bEnableDynamicLOD", false).toBool());
    conf.endGroup();

    conf.beginGroup("Network");
    mVersionControlCheck->setChecked(conf.value("bEnableVersionControl", false).toBool());
    mVcServerEdit->setText(conf.value("sVCServer", "").toString());
    conf.endGroup();
}

void PreferencesDialog::saveSettings()
{
    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);

    conf.beginGroup("General");
    conf.setValue("DataDirectory", mDataDirEdit->text());
    conf.setValue("GameId", mGameCombo->currentData().toInt());
    conf.setValue("Language", mLanguageCombo->currentText());
    conf.setValue("Theme", mThemeCombo->currentData().toString());
    conf.endGroup();

    conf.beginGroup("Display");
    conf.setValue("FOV", mFovSpin->value());
    conf.setValue("CameraSpeed", mCameraSpeedSpin->value());
    conf.setValue("RenderDistance", mRenderDistanceSpin->value());
    conf.setValue("VSync", mVsyncCheck->isChecked());
    conf.endGroup();

    conf.beginGroup("Edit");
    conf.setValue("AutoSaveMinutes", mAutoSaveSpin->value());
    conf.setValue("UndoDepth", mUndoDepthSpin->value());
    conf.setValue("DefaultSnap", mSnapSpin->value());
    conf.setValue("WarnMoveStatic", mWarnMoveStaticCheck->isChecked());
    conf.setValue("SkipInitialCellLoad", mSkipCellLoadCheck->isChecked());
    conf.endGroup();

    conf.beginGroup("Sound");
    conf.setValue("Volume", mVolumeSpin->value());
    conf.setValue("WwiseCodecID", mWwiseCodecSpin->value());
    conf.endGroup();

    conf.beginGroup("Papyrus");
    conf.setValue("sScriptCompilerFolder", mCompilerDirEdit->text());
    conf.setValue("sScriptSourceFolder", mScriptSourceDirEdit->text());
    conf.endGroup();

    conf.beginGroup("LOD");
    conf.setValue("bEnableMeshLODStreaming", mMeshLODCheck->isChecked());
    conf.setValue("bEnableDynamicLOD", mDynamicLODCheck->isChecked());
    conf.endGroup();

    conf.beginGroup("Network");
    conf.setValue("bEnableVersionControl", mVersionControlCheck->isChecked());
    conf.setValue("sVCServer", mVcServerEdit->text());
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
        mDataDirEdit->setText(dir);
}

void PreferencesDialog::browseCompilerDir()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Papyrus Compiler Directory",
        mCompilerDirEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!dir.isEmpty())
        mCompilerDirEdit->setText(dir);
}

void PreferencesDialog::browseScriptSourceDir()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Papyrus Script Source Directory",
        mScriptSourceDirEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!dir.isEmpty())
        mScriptSourceDirEdit->setText(dir);
}
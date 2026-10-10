#ifndef PREFERENCESDIALOG_HPP
#define PREFERENCESDIALOG_HPP

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QStackedWidget>

/// Modal dialog for editing application preferences across paged categories.
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);

private slots:
    void saveSettings();
    void loadSettings();
    void browseDataDir();
    void browseCompilerDir();
    void browseScriptSourceDir();
    void changePage(int index);
    void onGitRepoChanged();
    void refreshGitPanel();
    void onGitRefresh();
    void onGitCommit();
    void onGitPull();
    void onGitPush();

private:
    void setupUI();
    QWidget* createGeneralPage();
    QWidget* createDisplayPage();
    QWidget* createEditPage();
    QWidget* createSoundPage();
    QWidget* createArchivePage();
    QWidget* createPapyrusPage();
    QWidget* createLODPage();
    QWidget* createNetworkPage();

    QTreeWidget*   mCategoryTree;
    QStackedWidget* mPageStack;

    // General
    QLineEdit* mDataDirEdit;
    QComboBox* mGameCombo;
    QComboBox* mLanguageCombo;

    // Appearance (moved under General page as Theme)
    QComboBox* mThemeCombo;
    QComboBox* mUiScaleCombo;

    // Display
    QDoubleSpinBox* mFovSpin;
    QDoubleSpinBox* mCameraSpeedSpin;
    QSpinBox*       mRenderDistanceSpin;
    QCheckBox*      mVsyncCheck;

    // Edit
    QSpinBox*  mAutoSaveSpin;
    QSpinBox*  mUndoDepthSpin;
    QDoubleSpinBox* mSnapSpin;
    QCheckBox* mWarnMoveStaticCheck;
    QCheckBox* mSkipCellLoadCheck;

    // Sound
    QSpinBox*  mVolumeSpin;
    QSpinBox*  mWwiseCodecSpin;

    // Archive
    QListWidget* mArchiveList;
    QLineEdit* mToolIniEdit;
    QCheckBox* mAutoCollectCheck;

    // Papyrus
    QLineEdit* mCompilerDirEdit;
    QLineEdit* mScriptSourceDirEdit;

    // LOD
    QCheckBox* mMeshLODCheck;
    QCheckBox* mDynamicLODCheck;

    // Network
    QCheckBox* mVersionControlCheck;
    QLineEdit* mGitRepoEdit;
    QLabel* mGitStatusLabel;
    QComboBox* mGitBranchCombo;
    QPushButton* mGitRefreshBtn;
    QPushButton* mGitCommitBtn;
    QPushButton* mGitPullBtn;
    QPushButton* mGitPushBtn;
    bool mGitBranchUpdating = false;
};

#endif // PREFERENCESDIALOG_HPP
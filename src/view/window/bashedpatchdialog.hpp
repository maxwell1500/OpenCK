#ifndef BASHEDPATCHDIALOG_HPP
#define BASHEDPATCHDIALOG_HPP

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>

class Data;
class BashedPatchGenerator;

class BashedPatchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BashedPatchDialog(Data* data, QWidget* parent = nullptr);
    ~BashedPatchDialog();

private slots:
    void onGeneratePatch();
    void onSelectAll();
    void onDeselectAll();
    void onBrowseOutput();

private:
    void setupUI();

    Data* mData;
    BashedPatchGenerator* mGenerator;

    QCheckBox* mNpcCheckBox;
    QCheckBox* mWeaponCheckBox;
    QCheckBox* mArmorCheckBox;
    QCheckBox* mSpellCheckBox;
    QCheckBox* mAlchemyCheckBox;
    QCheckBox* mIngredientsCheckBox;
    QCheckBox* mBooksCheckBox;
    QCheckBox* mEnchantmentsCheckBox;
    QCheckBox* mContainersCheckBox;
    QCheckBox* mMiscCheckBox;
    QCheckBox* mActivatorsCheckBox;
    QCheckBox* mRaceCheckBox;
    QCheckBox* mClassCheckBox;
    QCheckBox* mQuestCheckBox;
    QCheckBox* mPackageCheckBox;
    QCheckBox* mFactCheckBox;
    QCheckBox* mPerkCheckBox;

    QLineEdit* mOutputPathEdit;
    QPushButton* mBrowseButton;
    QPushButton* mGenerateButton;
    QPushButton* mSelectAllButton;
    QPushButton* mDeselectAllButton;
    QTextEdit* mLogTextEdit;
    QProgressBar* mProgressBar;
    QLabel* mStatsLabel;
};

#endif // BASHEDPATCHDIALOG_HPP

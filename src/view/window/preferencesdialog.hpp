#ifndef PREFERENCESDIALOG_HPP
#define PREFERENCESDIALOG_HPP

#include <QDialog>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);

private slots:
    void saveSettings();
    void loadSettings();
    void browseDataDir();

private:
    void setupUI();

    QLineEdit* mDataDirEdit;
    QSpinBox* mAutoSaveSpin;
    QCheckBox* mSkipCellLoadCheck;
    QComboBox* mGameCombo;
    QComboBox* mLanguageCombo;
    QComboBox* mThemeCombo;
};

#endif // PREFERENCESDIALOG_HPP
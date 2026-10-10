#ifndef MIGRATIONDIALOG_HPP
#define MIGRATIONDIALOG_HPP

#include <QDialog>

class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;
class QTabWidget;
class Data;

// Phase 9.2: cross-game migration. Archives tab rebuilds an archive in
// another game's container (BSA 0x67/0x68/0x69 <-> BA2); Records tab copies
// a plugin's records into another game's plugin, carrying only the codes the
// TES4 family shares.
class MigrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MigrationDialog(QWidget* parent = nullptr);
    ~MigrationDialog() override;

private slots:
    void onBrowseArchiveSource();
    void onBrowseArchiveOutput();
    void onConvertArchive();
    void onBrowseRecordSource();
    void onBrowseRecordOutput();
    void onConvertRecords();

private:
    void setupUi();

    QTabWidget* mTabs;

    QLineEdit* mArchiveSourceEdit;
    QComboBox* mArchiveTargetCombo;
    QPushButton* mArchiveConvertBtn;
    QLabel* mArchiveResultLabel;

    QLineEdit* mRecordSourceEdit;
    QComboBox* mRecordTargetCombo;
    QLineEdit* mRecordOutputEdit;
    QPushButton* mRecordConvertBtn;
    QLabel* mRecordResultLabel;
};

#endif // MIGRATIONDIALOG_HPP

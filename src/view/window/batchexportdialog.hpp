#ifndef BATCHEXPORTDIALOG_HPP
#define BATCHEXPORTDIALOG_HPP

#include <QDialog>
#include <QListWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QProgressDialog>

class Data;
class AssetConverter;

struct ExportTemplate;

class BatchExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BatchExportDialog(Data* data, QWidget* parent = nullptr);

private slots:
    void onExportRecords();
    void onConvertAssets();
    void onAddFiles();
    void onRemoveFiles();
    void onClearFiles();
    void onTemplateSelected();
    void onUpdateProgress(int value, const QString& text = QString());
    void onComplete();

private:
    void setupUI();
    void populateRecordTypes();
    void populateAssetTypes();
    void exportRecordsToJson(const QStringList& recordTypes, const QString& outputDir);
    void exportRecordsToCsv(const QStringList& recordTypes, const QString& outputDir, const ExportTemplate* template_ = nullptr);
    void exportRecordsToXml(const QStringList& recordTypes, const QString& outputDir);
    QString selectExportDir();
    QString selectInputFiles(const QString& filter);
    void logMessage(const QString& message);

    Data* mData;

    // Record export tab widgets
    QTableWidget* mRecordTypeTable;
    QComboBox* mRecordFormatCombo;
    QComboBox* mTemplateCombo;
    QLineEdit* mEditorIdFilter;
    QSpinBox* mFormIdMin;
    QSpinBox* mFormIdMax;
    QCheckBox* mOnlyModified;
    QCheckBox* mOnlyDeleted;
    QPushButton* mExportRecordsBtn;

    // Asset conversion tab widgets
    QListWidget* mAssetFileList;
    QComboBox* mAssetTypeCombo;
    QComboBox* mAssetTargetFormatCombo;
    QPushButton* mAddFilesBtn;
    QPushButton* mRemoveFilesBtn;
    QPushButton* mClearFilesBtn;
    QPushButton* mConvertAssetsBtn;

    // Shared widgets
    QPlainTextEdit* mLogEdit;
    QProgressDialog* mProgress;
};

#endif // BATCHEXPORTDIALOG_HPP

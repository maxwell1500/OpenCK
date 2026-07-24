#ifndef EXPORTDIALOG_HPP
#define EXPORTDIALOG_HPP

#include <QDialog>
#include <QPlainTextEdit>
#include <QTextStream>
#include <QString>
#include <QComboBox>
#include <QCheckBox>
#include <QListWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>

#include "../../model/world/idcollection.hpp"

class Data;
struct DialRecord;
struct InfoRecord;
struct NpcRecord;

struct ImportPreviewInfo
{
    QString recordType;
    int count;
    QString action;
};

class ImportPreviewDialog : public QDialog
{
    Q_OBJECT
public:
    ImportPreviewDialog(const QList<ImportPreviewInfo>& records, const QString& filePath, QWidget* parent = nullptr);
};

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    ExportDialog(Data* data, QWidget* parent = nullptr);

private slots:
    void onExportClicked();
    void onImportClicked();
    void exportGenericData();
    void importData();

public:
    void exportDialogue();
    void exportScripts();
    void exportTextures();

private:
    void setupUI();
    QString selectExportDir();
    QString selectExportFile(const QString& filter);
    QStringList selectImportFiles(const QString& filter);
    QList<ImportPreviewInfo> scanImportFile(const QString& filePath);
    void writeDialogueTopic(const QString& exportDir, const DialRecord& dial, const IdCollection<InfoRecord>& infos);
    void writeManifestEntry(QTextStream& out, const NpcRecord& npc);
    void logMessage(const QString& message);

    Data* mData;
    QPlainTextEdit* mLogEdit;
    QComboBox* mFormatCombo;
    QListWidget* mTypeList;
    QLineEdit* mEditorIdFilter;
    QSpinBox* mFormIdMin;
    QSpinBox* mFormIdMax;
    QCheckBox* mOnlyModified;
    QCheckBox* mOnlyDeleted;
};

#endif // EXPORTDIALOG_HPP

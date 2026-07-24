#ifndef EXPORTTEMPLATESDIALOG_HPP
#define EXPORTTEMPLATESDIALOG_HPP

#include <QDialog>
#include <QListWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVector>
#include <QPair>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class Data;

struct ExportTemplate
{
    QString name;
    QString recordType;
    QString delimiter = ",";
    QString quoteChar = "\"";
    bool includeHeader = true;
    QVector<QPair<QString, QString>> fields;

    QJsonObject toJson() const;
    static ExportTemplate fromJson(const QJsonObject& json);
};

class TemplateManager
{
public:
    static QVector<ExportTemplate> loadTemplates(const QString& path = QString());
    static void saveTemplates(const QVector<ExportTemplate>& templates, const QString& path = QString());
    static QVector<ExportTemplate> builtinTemplates();
    static QString defaultPath();
};

class TemplateEditDialog : public QDialog
{
    Q_OBJECT
public:
    TemplateEditDialog(const ExportTemplate& existing = ExportTemplate(), QWidget* parent = nullptr);
    ExportTemplate result() const;

private slots:
    void onRecordTypeChanged(const QString& type);
    void onSelectAll();
    void onDeselectAll();

private:
    void populateFields(const QString& recordType);

    QComboBox* mRecordTypeCombo;
    QLineEdit* mNameEdit;
    QLineEdit* mDelimiterEdit;
    QLineEdit* mQuoteCharEdit;
    QCheckBox* mIncludeHeaderCheck;
    QListWidget* mFieldList;
    ExportTemplate mResult;
};

class ExportTemplatesDialog : public QDialog
{
    Q_OBJECT
public:
    ExportTemplatesDialog(Data* data, QWidget* parent = nullptr);

private slots:
    void onNewClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onExportClicked();
    void onTemplateSelected();

private:
    void refreshList();
    void exportWithTemplate(const ExportTemplate& tmplt);

    Data* mData;
    QListWidget* mTemplateList;
    QPushButton* mEditBtn;
    QPushButton* mDeleteBtn;
    QPushButton* mExportBtn;
    QVector<ExportTemplate> mTemplates;
};

#endif // EXPORTTEMPLATESDIALOG_HPP

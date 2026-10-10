#ifndef MATERIALEDITOR_H
#define MATERIALEDITOR_H

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QLabel>
#include <QVector>

struct MaterialRecord;
#include "../../model/tools/materialruletemplate.hpp"
class Data;
class MaterialPreviewWidget;

class MaterialEditor : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialEditor(Data* data, MaterialRecord* record, QWidget* parent = nullptr);
    ~MaterialEditor();

    void setRecord(MaterialRecord* record);

private slots:
    void saveChanges();
    void cancelEdit();
    void applyTemplate();
    void compileAndPreview();
    void browseSlotTexture();
    void addSlotRow();
    void removeSlotRow();

private:
    bool validate();
    void setupUI();
    void setupRuleTemplateTab();
    void loadFromMaterial();
    void saveToMaterial();
    void loadTemplates();
    void refreshSlotTable();
    void updatePreview();
    QMap<QString, QString> slotTableAsMap();

    Data* mData;
    MaterialRecord* mRecord;
    QTabWidget* mTabWidget;
    QLineEdit* mEditorIdEdit;
    QLineEdit* mFormIdEdit;
    QLineEdit* mMaterialNameEdit;
    QLineEdit* mBnamEdit;
    QLineEdit* mCnamEdit;
    QLineEdit* mTexturePathEdit;
    QPushButton* mSaveButton;
    QPushButton* mCancelButton;

    // Rule templates tab
    QComboBox* mTemplateCombo;
    QTableWidget* mSlotTable;
    QPushButton* mApplyTemplateButton;
    QPushButton* mCompileButton;
    QPushButton* mBrowseButton;
    QPushButton* mAddSlotButton;
    QPushButton* mRemoveSlotButton;
    QLabel* mStatusLabel;
    MaterialPreviewWidget* mPreview;
    QVector<MaterialRuleTemplate> mTemplates;
};

#endif // MATERIALEDITOR_H

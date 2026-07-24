#ifndef MATERIALEDITOR_H
#define MATERIALEDITOR_H

#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>

struct MaterialRecord;
class Data;

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

private:
    bool validate();
    void setupUI();
    void loadFromMaterial();
    void saveToMaterial();

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
};

#endif // MATERIALEDITOR_H

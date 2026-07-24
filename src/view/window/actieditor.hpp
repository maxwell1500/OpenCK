#ifndef ACTIEDITOR_HPP
#define ACTIEDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

class Data;
struct ActiRecord;

class ActiEditor : public QDialog
{
    Q_OBJECT

public:
    ActiEditor(Data* data, ActiRecord* record, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromActi();

    Data* mData;
    ActiRecord* mRecord;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mIconPathEdit;
    QLineEdit* mModelPathEdit;
    QSpinBox* mEnchantmentSpin;
};

#endif // ACTIEDITOR_HPP

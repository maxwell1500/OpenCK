#ifndef CONT_EDITOR_HPP
#define CONT_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>

class Data;
struct ContRecord;

class ContEditor : public QDialog
{
    Q_OBJECT

public:
    ContEditor(Data* data, ContRecord* cont, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromCont();

    Data* mData;
    ContRecord* mCont;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mIconPathEdit;
    QLineEdit* mModelPathEdit;
    QSpinBox* mContentsSpin;
    QSpinBox* mInventoryControlSpin;
    QDoubleSpinBox* mWeightSpin;
    QSpinBox* mValueSpin;
};

#endif // CONT_EDITOR_HPP

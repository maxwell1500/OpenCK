#ifndef MISCEDITOR_HPP
#define MISCEDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>

class Data;
struct MiscRecord;

class MiscEditor : public QDialog
{
    Q_OBJECT

public:
    MiscEditor(Data* data, MiscRecord* record, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromMisc();

    Data* mData;
    MiscRecord* mRecord;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mIconPathEdit;
    QLineEdit* mModelPathEdit;
    QSpinBox* mEnchantmentSpin;
    QDoubleSpinBox* mWeightSpin;
    QSpinBox* mValueSpin;
};

#endif // MISCEDITOR_HPP

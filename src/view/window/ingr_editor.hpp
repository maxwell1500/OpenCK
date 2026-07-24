#ifndef INGR_EDITOR_HPP
#define INGR_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>

class Data;
struct IngrRecord;

class IngrEditor : public QDialog
{
    Q_OBJECT

public:
    IngrEditor(Data* data, IngrRecord* ingr, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromIngr();

    Data* mData;
    IngrRecord* mIngr;
    QLineEdit* mEditorIdEdit;
    QLineEdit* mIconPathEdit;
    QLineEdit* mModelPathEdit;
    QSpinBox* mEnchantmentSpin;
    QDoubleSpinBox* mWeightSpin;
    QSpinBox* mValueSpin;
};

#endif // INGR_EDITOR_HPP

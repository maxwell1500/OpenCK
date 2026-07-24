#ifndef GLOBVAR_EDITOR_HPP
#define GLOBVAR_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QComboBox>

class Data;
struct GlobalVariable;

class GlobVarEditor : public QDialog
{
    Q_OBJECT

public:
    GlobVarEditor(Data* data, GlobalVariable* glob, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromGlob();
    void saveToGlob();

    Data* mData;
    GlobalVariable* mGlob;
    QLineEdit* mEditorIdEdit;
    QDoubleSpinBox* mValueSpin;
    QComboBox* mFlagCombo;
};

#endif // GLOBVAR_EDITOR_HPP

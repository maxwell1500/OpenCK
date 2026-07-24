#ifndef GLOBEDITOR_HPP
#define GLOBEDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>

class Data;
struct GlobalVariable;

class GlobEditor : public QDialog
{
    Q_OBJECT

public:
    GlobEditor(Data* data, GlobalVariable* record, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    void setupUI();
    void loadFromGlob();

    Data* mData;
    GlobalVariable* mRecord;

    QLineEdit* mEditorIdEdit;
    QDoubleSpinBox* mValueSpin;
    QComboBox* mFlagsCombo;
};

#endif // GLOBEDITOR_HPP

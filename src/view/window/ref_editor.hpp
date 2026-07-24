#ifndef REF_EDITOR_HPP
#define REF_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>

class Data;
struct RefrRecord;

class RefEditor : public QDialog
{
    Q_OBJECT

public:
    RefEditor(Data* data, RefrRecord* ref, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromRef();

    Data* mData;
    RefrRecord* mRef;

    QLineEdit* mEditorIdEdit;
    QSpinBox* mBaseIdSpin;
    QDoubleSpinBox* mPosXSpin;
    QDoubleSpinBox* mPosYSpin;
    QDoubleSpinBox* mPosZSpin;
    QDoubleSpinBox* mRotXDouble;
    QDoubleSpinBox* mRotYDouble;
    QDoubleSpinBox* mRotZDouble;
    QDoubleSpinBox* mScaleDouble;
    QSpinBox* mOwnerSpin;
    QSpinBox* mLockLevelSpin;
    QCheckBox* mInitiallyDisabledCheck;
};

#endif // REF_EDITOR_HPP

#ifndef WEAPONEDITOR_HPP
#define WEAPONEDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>

class Data;
struct WeaponRecord;

class WeaponEditor : public QDialog
{
    Q_OBJECT

public:
    WeaponEditor(Data* data, WeaponRecord* weapon, QWidget* parent = nullptr);

    WeaponRecord* getRecord() const { return mRecord; }

private slots:
    void saveRecord();
    void compareWeapon();

private:
    void setupUI();
    void loadFromWeapon();
    bool validateWeapon();

    Data* mData;
    WeaponRecord* mRecord;
    QString mOriginalEditorId;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mFullNameEdit;
    QDoubleSpinBox* mDamageSpin;
    QDoubleSpinBox* mSpeedSpin;
    QDoubleSpinBox* mReachSpin;
    QDoubleSpinBox* mWeightSpin;
    QSpinBox* mAttackSpin;
    QSpinBox* mValueSpin;
    QComboBox* mTypeCombo;
};

#endif // WEAPONEDITOR_HPP

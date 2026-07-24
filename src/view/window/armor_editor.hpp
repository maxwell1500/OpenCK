#ifndef ARMOR_EDITOR_HPP
#define ARMOR_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>

class Data;
struct ArmorRecord;

class ArmorEditor : public QDialog
{
    Q_OBJECT

public:
    ArmorEditor(Data* data, ArmorRecord* armor, QWidget* parent = nullptr);

    ArmorRecord* getRecord() const { return mRecord; }

private slots:
    void saveRecord();
    void compareArmor();

private:
    bool validate();
    void setupUI();
    void loadFromArmor();

    Data* mData;
    ArmorRecord* mRecord;
    QString mOriginalEditorId;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mFullNameEdit;
    QSpinBox* mValueSpin;
    QSpinBox* mWeightSpin;
    QSpinBox* mArmorSpin;
    QSpinBox* mHealthSpin;
};

#endif // ARMOR_EDITOR_HPP

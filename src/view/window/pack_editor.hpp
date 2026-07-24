#ifndef PACK_EDITOR_HPP
#define PACK_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

class Data;
struct PackageRecord;

class PackEditor : public QDialog
{
    Q_OBJECT

public:
    PackEditor(Data* data, PackageRecord* pack, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromPack();
    void saveToPack();

    Data* mData;
    PackageRecord* mPack;

    QLineEdit* mEditorIdEdit;
    QSpinBox* mPackageTypeSpin;
    QSpinBox* mTargetTypeSpin;
    QSpinBox* mTargetIdsSpin;
    QSpinBox* mParametersSpin;
};

#endif // PACK_EDITOR_HPP

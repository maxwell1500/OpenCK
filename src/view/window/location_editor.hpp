#ifndef LOCATION_EDITOR_HPP
#define LOCATION_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

class Data;
struct LocationRecord;

class LocationEditor : public QDialog
{
    Q_OBJECT

public:
    LocationEditor(Data* data, LocationRecord* location, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromLocation();

    Data* mData;
    LocationRecord* mLocation;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mLocationNameEdit;
    QSpinBox* mParentIdSpin;
    QSpinBox* mXSpin;
    QSpinBox* mYSpin;
    QSpinBox* mZSpin;
};

#endif // LOCATION_EDITOR_HPP

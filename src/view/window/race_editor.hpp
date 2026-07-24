#ifndef RACE_EDITOR_HPP
#define RACE_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

class Data;
struct RaceRecord;

class RaceEditor : public QDialog
{
    Q_OBJECT

public:
    RaceEditor(Data* data, RaceRecord* race, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromRace();

    Data* mData;
    RaceRecord* mRace;

    QLineEdit* mEditorIdEdit;
    QSpinBox* mRaceFlagsSpin;
    QSpinBox* mNpcVariablesSpin;
    QSpinBox* mFaceDataSpin;
    QSpinBox* mHeadDataSpin;
};

#endif // RACE_EDITOR_HPP

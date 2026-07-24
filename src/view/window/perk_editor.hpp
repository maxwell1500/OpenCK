#ifndef PERK_EDITOR_HPP
#define PERK_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPlainTextEdit>

class Data;
struct PerkRecord;

class PerkEditor : public QDialog
{
    Q_OBJECT

public:
    PerkEditor(Data* data, PerkRecord* perk, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromPerk();
    void saveToPerk();

    Data* mData;
    PerkRecord* mPerk;

    QLineEdit* mEditorIdEdit;
    QPlainTextEdit* mDescriptionEdit;
    QPlainTextEdit* mRequirementsEdit;
    QLineEdit* mIconPathEdit;
    QSpinBox* mConditionsSpin;
};

#endif // PERK_EDITOR_HPP

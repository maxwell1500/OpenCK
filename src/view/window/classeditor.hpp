#ifndef CLASS_EDITOR_HPP
#define CLASS_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPlainTextEdit>

class Data;
struct ClassRecord;

class ClassEditor : public QDialog
{
    Q_OBJECT

public:
    ClassEditor(Data* data, ClassRecord* classRec, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromClass();

    Data* mData;
    ClassRecord* mClass;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mClassNameEdit;
    QPlainTextEdit* mDescriptionEdit;
    QSpinBox* mFactionSpin;
    QLineEdit* mIconPathEdit;
};

#endif // CLASS_EDITOR_HPP

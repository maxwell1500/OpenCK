#ifndef GENERICEDITOR_HPP
#define GENERICEDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>

class Data;
class BaseRecord;

class GenericRecordEditor : public QDialog
{
    Q_OBJECT

public:
    GenericRecordEditor(Data* data, BaseRecord* record, const QString& recordTypeName, QWidget* parent = nullptr);

private slots:
    void saveRecord();
    void loadRecord();

private:
    void setupUI();

    Data* mData;
    BaseRecord* mRecord;
    QString mRecordTypeName;
    
    QLineEdit* mEditorIdEdit;
    QLineEdit* mFullNameEdit;
    QLineEdit* mDescEdit;
    QSpinBox* mLevelSpin;
    QSpinBox* mWeightSpin;
    QSpinBox* mValueSpin;
    QCheckBox* mMarkedCheck;
    QCheckBox* mVisibleCheck;
    QTextEdit* mNotesEdit;
};

#endif // GENERICEDITOR_HPP

#ifndef FACTEDITOR_HPP
#define FACTEDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QListWidget>

class Data;
struct FactRecord;

class FactEditor : public QDialog
{
    Q_OBJECT

public:
    FactEditor(Data* data, FactRecord* fact, QWidget* parent = nullptr);

private slots:
    void saveRecord();
    void addRank();
    void loadFromFact();

private:
    void setupUI();
    bool validateFact();

    Data* mData;
    FactRecord* mFact;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mNameEdit;
    QPlainTextEdit* mDescriptionEdit;
    QLineEdit* mIconPathEdit;
    QListWidget* mRanksList;
    QPushButton* mAddRankButton;
    QPushButton* mRemoveRankButton;
    QPushButton* mSaveButton;
    QPushButton* mCancelButton;
};

#endif // FACTEDITOR_HPP

#ifndef BOOK_EDITOR_HPP
#define BOOK_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPlainTextEdit>

class Data;
struct BookRecord;

class BookEditor : public QDialog
{
    Q_OBJECT

public:
    BookEditor(Data* data, BookRecord* book, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromBook();

    Data* mData;
    BookRecord* mBook;
    QLineEdit* mEditorIdEdit;
    QLineEdit* mIconPathEdit;
    QLineEdit* mModelPathEdit;
    QSpinBox* mPageCountSpin;
    QPlainTextEdit* mPagesEdit;
};

#endif // BOOK_EDITOR_HPP

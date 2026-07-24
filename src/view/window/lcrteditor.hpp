#ifndef LCRT_EDITOR_HPP
#define LCRT_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

class Data;
struct LocationRefType;

class LcrtEditor : public QDialog
{
    Q_OBJECT

public:
    LcrtEditor(Data* data, LocationRefType* lcrt, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromLcrt();

    Data* mData;
    LocationRefType* mLcrt;

    QLineEdit* mEditorIdEdit;
    QLineEdit* mColorEdit;
};

#endif // LCRT_EDITOR_HPP

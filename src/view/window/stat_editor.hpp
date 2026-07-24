#ifndef STAT_EDITOR_HPP
#define STAT_EDITOR_HPP

#include <QDialog>
#include <QLineEdit>

class Data;
struct StatRecord;

class StatEditor : public QDialog
{
    Q_OBJECT

public:
    StatEditor(Data* data, StatRecord* stat, QWidget* parent = nullptr);

private slots:
    void saveRecord();

private:
    bool validate();
    void setupUI();
    void loadFromStat();
    void saveToStat();

    Data* mData;
    StatRecord* mStat;
    QLineEdit* mEditorIdEdit;
    QLineEdit* mModelEdit;
};

#endif // STAT_EDITOR_HPP

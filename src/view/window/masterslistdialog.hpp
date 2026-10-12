#ifndef MASTERSLISTDIALOG_H
#define MASTERSLISTDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLabel>

class Data;

class MastersListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MastersListDialog(Data* data, QWidget* parent = nullptr);
    ~MastersListDialog();
private slots:
    void onAddMaster();
    void onRemoveMaster();

private:
    void populateMasters();
    
    Data* mData;
    QListWidget* mastersList;
    QLabel* statusLabel;
    QPushButton* addMasterBtn;
    QPushButton* removeMasterBtn;
};

#endif // MASTERSLISTDIALOG_H

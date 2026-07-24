#ifndef CONFLICTDIALOG_H
#define CONFLICTDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QString>

class Data;

class ConflictDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConflictDialog(Data* data, QWidget* parent = nullptr);
    ~ConflictDialog();

private:
    void checkConflicts();
    void saveReport();
    quint32 getFormIdForRecord(const QString& typeName, const QString& editorId);
    
    Data* mData;
    QListWidget* conflictsList;
    QLabel* statusLabel;
};

#endif // CONFLICTDIALOG_H

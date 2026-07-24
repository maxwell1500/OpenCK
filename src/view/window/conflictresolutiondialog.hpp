#ifndef CONFLICTRESOLUTIONDIALOG_HPP
#define CONFLICTRESOLUTIONDIALOG_HPP

#include "../../model/world/data.hpp"

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>

class ConflictResolutionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConflictResolutionDialog(Data* data, QWidget* parent = nullptr);
    ~ConflictResolutionDialog();

private slots:
    void onConflictSelected(int row);
    void onKeepA();
    void onKeepB();
    void onRemoveBoth();
    void onSaveChanges();

private:
    void setupUI();
    void loadConflicts();
    void showConflictDetails(const Data::ConflictInfo& conflict);
    QString getRecordSummary(const Data::ConflictInfo& conflict, int pluginIndex);
    bool removeRecordByPlugin(CkId::Type type, const QString& editorId, int pluginIndex);
    quint32 getFormIdByPlugin(CkId::Type type, const QString& editorId, int pluginIndex);

    Data* mData;
    QListWidget* mConflictList;
    QListWidget* mDetailA;
    QListWidget* mDetailB;
    QPushButton* mKeepAButton;
    QPushButton* mKeepBButton;
    QPushButton* mRemoveButton;
    QPushButton* mSaveButton;
    QLabel* mStatusLabel;

    QList<Data::ConflictInfo> mConflicts;
};

#endif // CONFLICTRESOLUTIONDIALOG_HPP

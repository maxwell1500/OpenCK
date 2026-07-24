#ifndef WORLDSACESDIALOG_HPP
#define WORLDSACESDIALOG_HPP

#include <QDialog>
#include <QTreeView>
#include <QAbstractItemModel>

class Data;

class WorldspacesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WorldspacesDialog(Data* data, QWidget* parent = nullptr);

private:
    Data* mData;
    QTreeView* mTreeView;
    QAbstractItemModel* mModel;
};

#endif // WORLDSACESDIALOG_HPP
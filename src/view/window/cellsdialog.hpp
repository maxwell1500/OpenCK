#ifndef CELLS_DIALOG_HPP
#define CELLS_DIALOG_HPP

#include <QDialog>
#include <QTreeView>
#include <QAbstractItemModel>

class Data;

class CellsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CellsDialog(Data* data, QWidget* parent = nullptr);

private:
    Data* mData;
    QTreeView* mTreeView;
    QAbstractItemModel* mModel;
};

#endif // CELLS_DIALOG_HPP
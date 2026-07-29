#ifndef CELLS_DIALOG_HPP
#define CELLS_DIALOG_HPP

#include <QWidget>

class Data;
class QComboBox;
class QListView;
class QTableView;
class QModelIndex;
class CellMapCanvas;
class CellListModel;
class RefrTableModel;

class CellViewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CellViewPanel(Data* data, QWidget* parent = nullptr);

private slots:
    void onWorldspaceChanged(int index);
    void onCellSelected(const QModelIndex& index);

private:
    Data* mData;
    QComboBox* mWorldspaceCombo;
    QListView* mCellList;
    QTableView* mRefrTable;
    CellMapCanvas* mMapCanvas;
    CellListModel* mCellModel;
    RefrTableModel* mRefrModel;
};

#endif // CELLS_DIALOG_HPP
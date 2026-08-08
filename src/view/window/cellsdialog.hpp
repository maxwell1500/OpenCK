#ifndef CELLS_DIALOG_HPP
#define CELLS_DIALOG_HPP

#include <QPointF>
#include <QVector>
#include <QWidget>

struct RefrRecord;
class Data;
class QComboBox;
class QListView;
class QTableView;
class QModelIndex;
class QLineEdit;
class CellMapCanvas;
class CellListModel;
class RefrTableModel;

/// Panel listing worldspaces, cells, and a cell's references on a map.
class CellViewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CellViewPanel(Data* data, QWidget* parent = nullptr);

signals:
    void refSelected(const RefrRecord* record);   // may be nullptr
    void cursorWorldPos(const QPointF& worldPos);
    void viewChanged();

private slots:
    void onWorldspaceChanged(int index);
    void onCellSelected(const QModelIndex& index);
    void onRefrTableSelectionChanged(const QModelIndex& current, const QModelIndex&);

private:
    void syncTableToCanvas(int canvasRow);

    Data* mData;
    QComboBox* mWorldspaceCombo;
    QListView* mCellList;
    QTableView* mRefrTable;
    QLineEdit* mFilterEdit;
    CellMapCanvas* mMapCanvas;
    CellListModel* mCellModel;
    RefrTableModel* mRefrModel;
};

#endif // CELLS_DIALOG_HPP
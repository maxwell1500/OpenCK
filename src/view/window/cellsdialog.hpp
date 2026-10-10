#ifndef CELLS_DIALOG_HPP
#define CELLS_DIALOG_HPP

#include <QPointF>
#include <QVector>
#include <QWidget>
#include <QAbstractTableModel>
#include <vector>

struct RefrRecord;
struct CellRecord;
struct WorldspaceRecord;
class Data;
class QComboBox;
class QTableView;
class QModelIndex;
class QLineEdit;
class QMenu;
class CellMapCanvas;

class CellTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CellTableModel(Data* data, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    const CellRecord* recordAt(int row) const;
    int collectionRowAt(int row) const;
    int rowForIndex(int collectionRow) const;

    void setWorldspace(const WorldspaceRecord* ws, bool interiorsOnly = false);
    void setFilter(const QString& filter);

private:
    void applyFilter();

    struct Row {
        const CellRecord* rec = nullptr;
        int collectionRow = -1;
        const CellRecord& get() const { return *rec; }
    };

    Data* mData;
    bool mInteriorsOnly = false;
    const WorldspaceRecord* mCurrentWs = nullptr;
    QString mFilterText;
    std::vector<Row> mAllRows;
    std::vector<Row> mRows;
};

class RefrTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit RefrTableModel(Data* data, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    const RefrRecord* recordAt(int row) const;
    int count() const;
    void setCell(const CellRecord* cell);
    void setFilter(const QString& filter);
    const QVector<QPointF>& points() const { return mPoints; }

private:
    void applyFilter();
    QString resolveBaseName(quint32 baseId, QString* outType) const;

    Data* mData;
    QString mFilterText;
    std::vector<const RefrRecord*> mAllRows;
    std::vector<const RefrRecord*> mRows;
    QVector<QPointF> mAllPoints;
    QVector<QPointF> mPoints;
    mutable QMap<quint32, QPair<QString, QString>> mBaseCache;
};

/// Panel listing worldspaces, cells, and placed references with Creation Kit parity.
class CellViewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CellViewPanel(Data* data, QWidget* parent = nullptr);
    ~CellViewPanel() override = default;

    CellTableModel* cellModel() const { return mCellModel; }
    RefrTableModel* refrModel() const { return mRefrModel; }
    QTableView* cellTable() const { return mCellTable; }
    QTableView* refrTable() const { return mRefrTable; }

    const CellRecord* currentCell() const;
    const RefrRecord* currentRef() const;

signals:
    void refSelected(const RefrRecord* record);   // may be nullptr
    void refDoubleClicked(quint32 refrFormId);
    void cellSelected(quint32 cellFormId);
    void cellDoubleClicked(quint32 cellFormId);
    void cursorWorldPos(const QPointF& worldPos);
    void viewChanged();

public slots:
    void selectCellByFormId(quint32 cellFormId);
    void selectRefByFormId(quint32 refrFormId);

private slots:
    void onWorldspaceChanged(int index);
    void onCellSelected(const QModelIndex& index);
    void onCellDoubleClicked(const QModelIndex& index);
    void onRefrTableSelectionChanged(const QModelIndex& current, const QModelIndex&);
    void onRefrTableDoubleClicked(const QModelIndex& index);
    void onRefrContextMenu(const QPoint& pos);
    void editSelectedRef();
    void duplicateSelectedRef();
    void deleteSelectedRef();
    void useInfoSelectedRef();

private:
    void syncTableToCanvas(int canvasRow);

    Data* mData;
    QComboBox* mWorldspaceCombo;
    QLineEdit* mCellFilterEdit;
    QTableView* mCellTable;
    QTableView* mRefrTable;
    QLineEdit* mFilterEdit;
    CellMapCanvas* mMapCanvas;
    CellTableModel* mCellModel;
    RefrTableModel* mRefrModel;
    QMenu* mRefrContextMenu;
};

#endif // CELLS_DIALOG_HPP

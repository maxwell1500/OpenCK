#include "cellsdialog.hpp"

#include "logger.hpp"

#include "cellmapview.hpp"
#include "model/world/data.hpp"
#include "model/world/collection.hpp"
#include "model/world/record.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/refrecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"

#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QTableView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QPainter>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QRect>
#include <QSet>

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
constexpr int kCellUnits = 4096;
}

class CellMapCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit CellMapCanvas(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(200, 200);
        setMouseTracking(true);
        mView.setWidgetSize(size());
    }

    void setReferences(const QVector<QPointF>& pts)
    {
        mPoints = pts;
        clearSelection();
        update();
    }

    void setCellGrid(qint32 x, qint32 y)
    {
        mCellX = x;
        mCellY = y;
        mView.setWidgetSize(size());
        mView.fitCell(x, y, size());
        update();
    }

    QVector<int> selectedRows() const { return mSelectedRows; }

    void setSelectedRows(const QVector<int>& rows)
    {
        QSet<int> unique;
        for (int r : rows)
            unique.insert(r);
        mSelectedRows = unique.values();
        std::sort(mSelectedRows.begin(), mSelectedRows.end());
        update();
    }

    void clearSelection()
    {
        mSelectedRows.clear();
        update();
    }

signals:
    void markerClicked(int row);
    void selectionChanged(const QVector<int>& rows);
    void hoverChanged(int row);
    void cursorWorldPos(const QPointF& worldPos);
    void viewChanged();

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), Qt::black);

        const QPointF tl = mView.worldToScreen(QPointF(mCellX * kCellUnits, mCellY * kCellUnits));
        const double px = mView.pxPerUnit();
        const QRectF cellRect(tl.x(), tl.y(), kCellUnits * px, kCellUnits * px);
        if (cellRect.intersects(QRectF(rect())))
        {
            p.setPen(QPen(Qt::white, 1));
            p.drawRect(cellRect);

            if (kCellUnits * px <= 32768)
            {
                p.setPen(QPen(QColor(50, 50, 50), 1));
                for (int u = 512; u < kCellUnits; u += 512)
                {
                    const double s = u * px;
                    if (s < 6)
                        continue;
                    p.drawLine(QPointF(tl.x() + s, tl.y()),
                               QPointF(tl.x() + s, tl.y() + kCellUnits * px));
                    p.drawLine(QPointF(tl.x(), tl.y() + s),
                               QPointF(tl.x() + kCellUnits * px, tl.y() + s));
                }
            }
        }

        for (int i = 0; i < mPoints.size(); ++i)
        {
            const QPointF sp = mView.worldToScreen(mPoints[i]);
            if (mSelectedRows.contains(i))
            {
                p.fillRect(QRectF(sp.x() - 3.5, sp.y() - 3.5, 7.0, 7.0),
                           QColor(240, 220, 60));
            }
            else
            {
                p.fillRect(QRectF(sp.x() - 2.0, sp.y() - 2.0, 4.0, 4.0),
                           QColor(80, 200, 120));
            }
            if (i == mHoverRow)
            {
                p.setPen(QPen(Qt::white, 1));
                p.setBrush(Qt::NoBrush);
                p.drawEllipse(QPointF(sp.x(), sp.y()), 3.5, 3.5);
            }
        }

        if (mMarqueeActive)
        {
            p.fillRect(mMarqueeRect, QColor(60, 120, 255, 60));
            p.setPen(QPen(QColor(120, 170, 255), 1));
            p.drawRect(mMarqueeRect);
        }

        p.setPen(QPen(QColor(255, 220, 0), 1));
        p.drawText(8, 14, QStringLiteral("Cell (%1, %2)  Refs: %3")
            .arg(mCellX).arg(mCellY).arg(mPoints.size()));
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            const int hit = mView.hitTest(mPoints, event->pos());
            if (hit >= 0)
            {
                if (event->modifiers() & Qt::ShiftModifier)
                {
                    if (mSelectedRows.contains(hit))
                        mSelectedRows.removeAll(hit);
                    else
                        mSelectedRows.push_back(hit);
                    std::sort(mSelectedRows.begin(), mSelectedRows.end());
                }
                else if (event->modifiers() & Qt::ControlModifier)
                {
                    if (!mSelectedRows.contains(hit))
                        mSelectedRows.push_back(hit);
                    std::sort(mSelectedRows.begin(), mSelectedRows.end());
                }
                else
                {
                    mSelectedRows = { hit };
                }
                update();
                emit markerClicked(hit);
                emit selectionChanged(selectedRows());
            }
            else
            {
                mMarqueeActive = true;
                mMarqueeOrigin = event->pos();
                mMarqueeRect = QRect(mMarqueeOrigin, QSize());
                update();
            }
        }
        else if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton)
        {
            mPanning = true;
            mLastPanPos = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (mPanning)
        {
            mView.panByPixels(QPointF(event->pos() - mLastPanPos));
            mLastPanPos = event->pos();
            update();
            emit viewChanged();
        }
        else if (mMarqueeActive)
        {
            mMarqueeRect = QRect(mMarqueeOrigin, event->pos()).normalized();
            mSelectedRows.clear();
            for (int i = 0; i < mPoints.size(); ++i)
            {
                if (mMarqueeRect.contains(mView.worldToScreen(mPoints[i]).toPoint()))
                    mSelectedRows.push_back(i);
            }
            update();
            emit selectionChanged(selectedRows());
        }
        else
        {
            const int hit = mView.hitTest(mPoints, event->pos());
            if (hit != mHoverRow)
            {
                mHoverRow = hit;
                update();
                emit hoverChanged(hit);
            }
            emit cursorWorldPos(mView.worldAt(event->pos()));
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        mPanning = false;
        unsetCursor();
        mMarqueeActive = false;
        update();
        QWidget::mouseReleaseEvent(event);
    }

    void wheelEvent(QWheelEvent* event) override
    {
        const double factor = event->angleDelta().y() > 0 ? 1.25 : 0.8;
        mView.zoomAt(event->position(), factor);
        update();
        emit viewChanged();
        QWidget::wheelEvent(event);
    }

    void resizeEvent(QResizeEvent* event) override
    {
        mView.setWidgetSize(size());
        QWidget::resizeEvent(event);
        update();
    }

private:
    CellMapView mView;
    QVector<QPointF> mPoints;
    QVector<int> mSelectedRows;
    int mHoverRow = -1;
    bool mPanning = false;
    QPoint mLastPanPos;
    bool mMarqueeActive = false;
    QPoint mMarqueeOrigin;
    QRect mMarqueeRect;
    qint32 mCellX = 0;
    qint32 mCellY = 0;
};

class CellListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CellListModel(Data* data, QObject* parent = nullptr)
        : QAbstractListModel(parent), mData(data)
    {
    }

    int rowCount(const QModelIndex& parent) const override
    {
        if (parent.isValid()) return 0;
        return static_cast<int>(mRows.size());
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole)
            return {};
        if (index.row() < 0 || index.row() >= static_cast<int>(mRows.size()))
            return {};
        const auto& rec = mRows[index.row()].get();
        QString label = rec.editorId.isEmpty() ? rec.cellName : rec.editorId;
        return label;
    }

    int rowForIndex(int collectionRow) const
    {
        for (int i = 0; i < static_cast<int>(mRows.size()); ++i)
            if (mRows[i].collectionRow == collectionRow)
                return i;
        return -1;
    }

    const CellRecord* recordAt(int row) const
    {
        if (row < 0 || row >= static_cast<int>(mRows.size()))
            return nullptr;
        return &mRows[row].get();
    }

    int collectionRowAt(int row) const
    {
        if (row < 0 || row >= static_cast<int>(mRows.size()))
            return -1;
        return mRows[row].collectionRow;
    }

    void setWorldspace(const WorldspaceRecord* ws)
    {
        beginResetModel();
        mRows.clear();
        if (!mData) { endResetModel(); return; }

        const auto& cells = mData->getCellCollection();
        const int n = cells.size();
        if (!ws)
        {
            for (int i = 0; i < n; ++i)
            {
                const auto& rec = cells.getRecord(i);
                if (rec.isDeleted()) continue;
                mRows.push_back(Row{ &rec.get(), i });
            }
        }
        else
        {
            const QVector<quint32> cellIds =
                mData ? mData->cellsInWorldspace(ws->formId) : QVector<quint32>();
            for (int i = 0; i < n; ++i)
            {
                const auto& rec = cells.getRecord(i);
                if (rec.isDeleted()) continue;
                if (cellIds.contains(rec.get().formId))
                    mRows.push_back(Row{ &rec.get(), i });
            }
        }
        endResetModel();
    }

private:
    struct Row
    {
        const CellRecord* rec = nullptr;
        int collectionRow = -1;
        const CellRecord& get() const { return *rec; }
    };

    Data* mData;
    std::vector<Row> mRows;
};

class RefrTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit RefrTableModel(Data* data, QObject* parent = nullptr)
        : QAbstractTableModel(parent), mData(data)
    {
    }

    int rowCount(const QModelIndex& parent) const override
    {
        if (parent.isValid()) return 0;
        return static_cast<int>(mRows.size());
    }

    int columnCount(const QModelIndex& parent) const override
    {
        Q_UNUSED(parent)
        return 4;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return {};
        switch (section)
        {
        case 0: return QStringLiteral("Editor ID");
        case 1: return QStringLiteral("Form ID");
        case 2: return QStringLiteral("Position (X, Y, Z)");
        case 3: return QStringLiteral("Base Object");
        }
        return {};
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole)
            return {};
        if (index.row() < 0 || index.row() >= static_cast<int>(mRows.size()))
            return {};
        const auto* r = mRows[index.row()];
        switch (index.column())
        {
        case 0: return r->editorId;
        case 1: return QStringLiteral("0x%1").arg(r->formId, 8, 16, QChar('0'));
        case 2: return QStringLiteral("%1, %2, %3").arg(r->posX, 0, 'f', 1).arg(r->posY, 0, 'f', 1).arg(r->posZ, 0, 'f', 1);
        case 3: return QStringLiteral("0x%1").arg(r->baseId, 8, 16, QChar('0'));
        }
        return {};
    }

    const RefrRecord* recordAt(int row) const
    {
        if (row < 0 || row >= static_cast<int>(mRows.size()))
            return nullptr;
        return mRows[row];
    }

    int count() const { return static_cast<int>(mRows.size()); }

    void setCell(const CellRecord* cell)
    {
        beginResetModel();
        mAllRows.clear();
        mAllPoints.clear();
        if (!mData || !cell) { endResetModel(); return; }

        const auto& refrs = mData->getRefrCollection();
        const int n = refrs.size();
        const qint32 cx = static_cast<qint32>(cell->cellX);
        const qint32 cy = static_cast<qint32>(cell->cellY);
        for (int i = 0; i < n; ++i)
        {
            const auto& rec = refrs.getRecord(i);
            if (rec.isDeleted()) continue;
            const auto& r = rec.get();
            const qint32 gx = static_cast<qint32>(std::floor(r.posX / kCellUnits));
            const qint32 gy = static_cast<qint32>(std::floor(r.posY / kCellUnits));
            if (gx != cx || gy != cy) continue;
            mAllRows.push_back(&r);
            mAllPoints.push_back(QPointF(r.posX, r.posY));
        }
        applyFilter();
        endResetModel();
    }

    void setFilter(const QString& filter)
    {
        beginResetModel();
        mFilterText = filter.trimmed();
        applyFilter();
        endResetModel();
    }

    const QVector<QPointF>& points() const { return mPoints; }

private:
    void applyFilter()
    {
        mRows.clear();
        mPoints.clear();
        for (int i = 0; i < static_cast<int>(mAllRows.size()); ++i)
        {
            const auto* r = mAllRows[i];
            const QString formId = QStringLiteral("0x%1").arg(r->formId, 8, 16, QChar('0'));
            if (mFilterText.isEmpty()
                || r->editorId.contains(mFilterText, Qt::CaseInsensitive)
                || formId.contains(mFilterText, Qt::CaseInsensitive))
            {
                mRows.push_back(r);
                mPoints.push_back(mAllPoints[i]);
            }
        }
    }

    Data* mData;
    QString mFilterText;
    std::vector<const RefrRecord*> mAllRows;
    std::vector<const RefrRecord*> mRows;
    QVector<QPointF> mAllPoints;
    QVector<QPointF> mPoints;
};

CellViewPanel::CellViewPanel(Data* data, QWidget* parent)
    : QWidget(parent), mData(data)
{
    LOG_INFO(QStringLiteral("CellViewPanel created"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(2, 2, 2, 2);
    root->setSpacing(2);

    auto* topBar = new QHBoxLayout();
    topBar->setContentsMargins(0, 0, 0, 0);
    topBar->addWidget(new QLabel(QStringLiteral("Worldspace:"), this));
    mWorldspaceCombo = new QComboBox(this);
    mWorldspaceCombo->addItem(QStringLiteral("(All cells)"), QString());
    if (mData)
    {
        const auto& ws = mData->getWorldspaceCollection();
        const int n = ws.size();
        for (int i = 0; i < n; ++i)
        {
            const auto& rec = ws.getRecord(i);
            if (rec.isDeleted()) continue;
            const auto& w = rec.get();
            QString label = w.editorId.isEmpty() ? w.name : w.editorId;
            mWorldspaceCombo->addItem(label, w.editorId);
        }
    }
    topBar->addWidget(mWorldspaceCombo, 1);
    root->addLayout(topBar);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    mCellList = new QListView(splitter);
    mCellModel = new CellListModel(mData, this);
    mCellList->setModel(mCellModel);
    mCellList->setAlternatingRowColors(true);
    splitter->addWidget(mCellList);

    auto* rightSplit = new QSplitter(Qt::Vertical, splitter);
    rightSplit->setChildrenCollapsible(false);

    mMapCanvas = new CellMapCanvas(rightSplit);
    rightSplit->addWidget(mMapCanvas);

    mRefrTable = new QTableView(rightSplit);
    mRefrModel = new RefrTableModel(mData, this);
    mRefrTable->setModel(mRefrModel);
    mRefrTable->setAlternatingRowColors(true);
    mRefrTable->horizontalHeader()->setStretchLastSection(false);
    mRefrTable->verticalHeader()->setVisible(false);
    rightSplit->addWidget(mRefrTable);

    splitter->addWidget(rightSplit);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    root->addWidget(splitter, 1);

    auto* bottomBar = new QToolBar(this);
    auto* goToAct = new QAction(QStringLiteral("Go to cell"), bottomBar);
    bottomBar->addAction(goToAct);
    auto* filterAct = new QAction(QStringLiteral("Filter"), bottomBar);
    bottomBar->addAction(filterAct);
    mFilterEdit = new QLineEdit(this);
    mFilterEdit->setPlaceholderText(QStringLiteral("Filter refs by Editor ID or Form ID..."));
    mFilterEdit->setClearButtonEnabled(true);
    bottomBar->addWidget(mFilterEdit);
    bottomBar->addSeparator();
    auto* refreshAct = new QAction(QStringLiteral("Refresh"), bottomBar);
    bottomBar->addAction(refreshAct);
    root->addWidget(bottomBar);

    setMinimumSize(500, 400);

    mCellModel->setWorldspace(nullptr);
    connect(mWorldspaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &CellViewPanel::onWorldspaceChanged);
    connect(mCellList->selectionModel(), &QItemSelectionModel::currentChanged,
        this, &CellViewPanel::onCellSelected);
    connect(mRefrTable->selectionModel(), &QItemSelectionModel::currentChanged,
        this, &CellViewPanel::onRefrTableSelectionChanged);
    connect(mMapCanvas, &CellMapCanvas::markerClicked, this, [this](int row)
    {
        syncTableToCanvas(row);
    });
    connect(mMapCanvas, &CellMapCanvas::selectionChanged, this, [this](const QVector<int>& rows)
    {
        if (!rows.isEmpty()) syncTableToCanvas(rows.first());
    });
    connect(mMapCanvas, &CellMapCanvas::cursorWorldPos, this, &CellViewPanel::cursorWorldPos);
    connect(mMapCanvas, &CellMapCanvas::viewChanged, this, &CellViewPanel::viewChanged);
    connect(refreshAct, &QAction::triggered, this, [this]()
    {
        mCellModel->setWorldspace(nullptr);
        mWorldspaceCombo->setCurrentIndex(0);
    });
    connect(goToAct, &QAction::triggered, this, [this]()
    {
        auto idx = mCellList->currentIndex();
        if (idx.isValid())
            mCellList->scrollTo(idx);
    });
    connect(filterAct, &QAction::triggered, this, [this]()
    {
        if (mFilterEdit)
            mFilterEdit->setFocus();
    });
    connect(mFilterEdit, &QLineEdit::textChanged, this, [this](const QString& text)
    {
        if (mRefrModel)
            mRefrModel->setFilter(text);
    });
}

void CellViewPanel::onWorldspaceChanged(int index)
{
    if (!mData) return;
    if (index < 0) return;
    const QString editorId = mWorldspaceCombo->itemData(index).toString();

    const WorldspaceRecord* ws = nullptr;
    if (!editorId.isEmpty())
    {
        const auto& wsc = mData->getWorldspaceCollection();
        int row = wsc.searchId(editorId);
        if (row >= 0)
            ws = &wsc.getRecord(row).get();
    }
    mCellModel->setWorldspace(ws);
}

void CellViewPanel::syncTableToCanvas(int canvasRow)
{
    if (!mRefrModel) return;
    if (canvasRow < 0 || canvasRow >= mRefrModel->count()) return;
    QModelIndex idx = mRefrModel->index(canvasRow, 0);
    mRefrTable->setCurrentIndex(idx);
    mRefrTable->scrollTo(idx);
    emit refSelected(mRefrModel->recordAt(canvasRow));
}

void CellViewPanel::onRefrTableSelectionChanged(const QModelIndex& current, const QModelIndex&)
{
    if (!current.isValid()) return;
    const int row = current.row();
    if (row < 0 || row >= mRefrModel->count()) return;
    mMapCanvas->setSelectedRows({ row });
    emit refSelected(mRefrModel->recordAt(row));
}

void CellViewPanel::onCellSelected(const QModelIndex& index)
{
    if (!index.isValid())
    {
        mRefrModel->setCell(nullptr);
        mMapCanvas->setReferences({});
        mMapCanvas->clearSelection();
        mMapCanvas->setCellGrid(0, 0);
        return;
    }
    const auto* cell = mCellModel->recordAt(index.row());
    if (!cell)
    {
        mRefrModel->setCell(nullptr);
        return;
    }
    mRefrModel->setCell(cell);
    mMapCanvas->setReferences(mRefrModel->points());
    mMapCanvas->setCellGrid(static_cast<qint32>(cell->cellX), static_cast<qint32>(cell->cellY));
}

#include "cellsdialog.moc"
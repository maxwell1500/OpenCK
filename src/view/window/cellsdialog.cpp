#include "cellsdialog.hpp"

#include "logger.hpp"

#include "model/world/data.hpp"
#include "model/world/collection.hpp"
#include "model/world/record.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/refrecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"

#include <QComboBox>
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
    }

    void setReferences(const QVector<QPair<float, float>>& pts)
    {
        mPoints = pts;
        update();
    }

    void setCellGrid(qint32 x, qint32 y)
    {
        mCellX = x;
        mCellY = y;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.fillRect(rect(), Qt::black);

        const int w = width();
        const int h = height();
        const int grid = 32;

        p.setPen(QPen(QColor(60, 60, 60), 1));
        for (int gx = 0; gx <= w; gx += grid)
            p.drawLine(gx, 0, gx, h);
        for (int gy = 0; gy <= h; gy += grid)
            p.drawLine(0, gy, w, gy);

        p.setPen(QPen(QColor(120, 120, 120), 1));
        p.drawLine(w / 2, 0, w / 2, h);
        p.drawLine(0, h / 2, w, h / 2);

        if (mPoints.isEmpty())
            return;

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        for (const auto& pt : mPoints)
        {
            minX = std::min(minX, pt.first);
            maxX = std::max(maxX, pt.first);
            minY = std::min(minY, pt.second);
            maxY = std::max(maxY, pt.second);
        }
        if (maxX - minX < 1.0f) { maxX = minX + 1.0f; }
        if (maxY - minY < 1.0f) { maxY = minY + 1.0f; }

        const float scaleX = (w - 20.0f) / (maxX - minX);
        const float scaleY = (h - 20.0f) / (maxY - minY);
        const float scale = std::min(scaleX, scaleY);
        const float ox = (w - (maxX - minX) * scale) / 2.0f;
        const float oy = (h - (maxY - minY) * scale) / 2.0f;

        p.setBrush(QBrush(QColor(80, 200, 120)));
        p.setPen(QPen(QColor(200, 255, 200), 1));
        for (const auto& pt : mPoints)
        {
            float px = ox + (pt.first - minX) * scale;
            float py = oy + (pt.second - minY) * scale;
            p.drawRect(QRectF(px - 2.0, py - 2.0, 4.0, 4.0));
        }

        p.setPen(QPen(QColor(255, 220, 0), 1));
        p.drawText(8, 14, QStringLiteral("Cell (%1, %2)  Refs: %3")
            .arg(mCellX).arg(mCellY).arg(mPoints.size()));
    }

private:
    QVector<QPair<float, float>> mPoints;
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
            const auto& cellIds = ws->cellIds;
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
        mRows.clear();
        mPoints.clear();
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
            mRows.push_back(&r);
            mPoints.push_back({ r.posX, r.posY });
        }
        endResetModel();
    }

    const QVector<QPair<float, float>>& points() const { return mPoints; }

private:
    Data* mData;
    std::vector<const RefrRecord*> mRows;
    QVector<QPair<float, float>> mPoints;
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
        LOG_DEBUG(QStringLiteral("Cell View filter requested (placeholder)"));
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

void CellViewPanel::onCellSelected(const QModelIndex& index)
{
    if (!index.isValid())
    {
        mRefrModel->setCell(nullptr);
        mMapCanvas->setReferences({});
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
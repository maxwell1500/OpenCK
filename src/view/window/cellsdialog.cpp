#include "cellsdialog.hpp"

#include "logger.hpp"

#include "cellmapview.hpp"
#include "model/world/data.hpp"
#include "model/world/collection.hpp"
#include "model/world/record.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/refrecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"
#include "useinfodialog.hpp"
#include "qtformdialogmanager.hpp"
#include "recordeditsession.hpp"
#include "../../model/world/ckid.hpp"
#include "../../model/tools/formcomponentsresolver.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/tools/addrecordcommand.hpp"

#include <QComboBox>
#include <QLineEdit>
#include <QTableView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QMenu>
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

// ============================================================================
// CellTableModel Implementation
// ============================================================================

CellTableModel::CellTableModel(Data* data, QObject* parent)
    : QAbstractTableModel(parent), mData(data)
{
}

int CellTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(mRows.size());
}

int CellTableModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 6;
}

QVariant CellTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};
    switch (section)
    {
    case 0: return QStringLiteral("Editor ID");
    case 1: return QStringLiteral("Form ID");
    case 2: return QStringLiteral("Grid (X, Y)");
    case 3: return QStringLiteral("Name");
    case 4: return QStringLiteral("Location");
    case 5: return QStringLiteral("Has Water");
    }
    return {};
}

QVariant CellTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};
    if (index.row() < 0 || index.row() >= static_cast<int>(mRows.size()))
        return {};
    const auto& rec = mRows[index.row()].get();
    switch (index.column())
    {
    case 0: return rec.editorId;
    case 1: return QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
    case 2:
        if (rec.flags & 1)
            return QStringLiteral("Interior");
        return QString("%1, %2").arg(static_cast<qint32>(rec.cellX)).arg(static_cast<qint32>(rec.cellY));
    case 3: return rec.cellName;
    case 4:
        if (rec.owner != 0)
            return QString("0x%1").arg(rec.owner, 8, 16, QChar('0'));
        return {};
    case 5:
        return rec.hasWaterHeight ? QString("Yes (%1)").arg(rec.waterHeight, 0, 'f', 1)
                                  : QStringLiteral("No");
    }
    return {};
}

int CellTableModel::rowForIndex(int collectionRow) const
{
    for (int i = 0; i < static_cast<int>(mRows.size()); ++i)
        if (mRows[i].collectionRow == collectionRow)
            return i;
    return -1;
}

const CellRecord* CellTableModel::recordAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(mRows.size()))
        return nullptr;
    return &mRows[row].get();
}

int CellTableModel::collectionRowAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(mRows.size()))
        return -1;
    return mRows[row].collectionRow;
}

void CellTableModel::setWorldspace(const WorldspaceRecord* ws, bool interiorsOnly)
{
    beginResetModel();
    mAllRows.clear();
    mInteriorsOnly = interiorsOnly;
    mCurrentWs = ws;
    if (!mData) { endResetModel(); return; }

    const auto& cells = mData->getCellCollection();
    const int n = cells.size();
    if (interiorsOnly)
    {
        for (int i = 0; i < n; ++i)
        {
            const auto& rec = cells.getRecord(i);
            if (rec.isDeleted()) continue;
            if ((rec.get().flags & 1) != 0)
                mAllRows.push_back(Row{ &rec.get(), i });
        }
    }
    else if (!ws)
    {
        for (int i = 0; i < n; ++i)
        {
            const auto& rec = cells.getRecord(i);
            if (rec.isDeleted()) continue;
            mAllRows.push_back(Row{ &rec.get(), i });
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
                mAllRows.push_back(Row{ &rec.get(), i });
        }
    }
    applyFilter();
    endResetModel();
}

void CellTableModel::setFilter(const QString& filter)
{
    beginResetModel();
    mFilterText = filter.trimmed();
    applyFilter();
    endResetModel();
}

void CellTableModel::applyFilter()
{
    mRows.clear();
    for (const auto& r : mAllRows)
    {
        if (mFilterText.isEmpty())
        {
            mRows.push_back(r);
            continue;
        }
        const auto& rec = r.get();
        const QString formIdStr = QString("0x%1").arg(rec.formId, 8, 16, QChar('0'));
        if (rec.editorId.contains(mFilterText, Qt::CaseInsensitive)
            || rec.cellName.contains(mFilterText, Qt::CaseInsensitive)
            || formIdStr.contains(mFilterText, Qt::CaseInsensitive))
        {
            mRows.push_back(r);
        }
    }
}

// ============================================================================
// RefrTableModel Implementation
// ============================================================================

RefrTableModel::RefrTableModel(Data* data, QObject* parent)
    : QAbstractTableModel(parent), mData(data)
{
}

int RefrTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(mRows.size());
}

int RefrTableModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 9;
}

QVariant RefrTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};
    switch (section)
    {
    case 0: return QStringLiteral("Form ID");
    case 1: return QStringLiteral("Editor ID");
    case 2: return QStringLiteral("Base Object");
    case 3: return QStringLiteral("Type");
    case 4: return QStringLiteral("Position (X, Y, Z)");
    case 5: return QStringLiteral("Rotation (X, Y, Z)");
    case 6: return QStringLiteral("Scale");
    case 7: return QStringLiteral("Persistent");
    case 8: return QStringLiteral("Disabled");
    }
    return {};
}

QVariant RefrTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};
    if (index.row() < 0 || index.row() >= static_cast<int>(mRows.size()))
        return {};
    const auto* r = mRows[index.row()];
    switch (index.column())
    {
    case 0: return QStringLiteral("0x%1").arg(r->formId, 8, 16, QChar('0'));
    case 1: return r->editorId;
    case 2:
    {
        QString typeName;
        return resolveBaseName(r->baseId, &typeName);
    }
    case 3:
    {
        QString typeName;
        resolveBaseName(r->baseId, &typeName);
        return typeName.isEmpty() ? QStringLiteral("Reference") : typeName;
    }
    case 4: return QString("%1, %2, %3").arg(r->posX, 0, 'f', 1).arg(r->posY, 0, 'f', 1).arg(r->posZ, 0, 'f', 1);
    case 5: return QString("%1, %2, %3").arg(r->rotX, 0, 'f', 1).arg(r->rotY, 0, 'f', 1).arg(r->rotZ, 0, 'f', 1);
    case 6: return QString::number(r->scale, 'f', 2);
    case 7: return (r->hasEdid || r->lockLevel != 0) ? QStringLiteral("Yes") : QStringLiteral("No");
    case 8: return r->initiallyDisabled ? QStringLiteral("Yes") : QStringLiteral("No");
    }
    return {};
}

const RefrRecord* RefrTableModel::recordAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(mRows.size()))
        return nullptr;
    return mRows[row];
}

int RefrTableModel::count() const
{
    return static_cast<int>(mRows.size());
}


void RefrTableModel::setCell(const CellRecord* cell)
{
    beginResetModel();
    mAllRows.clear();
    mAllPoints.clear();
    if (!mData || !cell) { endResetModel(); return; }

    const auto& refrs = mData->getRefrCollection();
    const int n = refrs.size();
    const bool isInterior = (cell->flags & 1) != 0;
    const qint32 cx = static_cast<qint32>(cell->cellX);
    const qint32 cy = static_cast<qint32>(cell->cellY);
    const quint32 cellFid = cell->formId;

    for (int i = 0; i < n; ++i)
    {
        const auto& rec = refrs.getRecord(i);
        if (rec.isDeleted()) continue;
        const auto& r = rec.get();

        bool match = false;
        const quint32 pCell = mData->parentCellOfRefr(r.formId);
        if (pCell != 0 && pCell == cellFid)
        {
            match = true;
        }
        else if (!isInterior)
        {
            const qint32 gx = static_cast<qint32>(std::floor(r.posX / kCellUnits));
            const qint32 gy = static_cast<qint32>(std::floor(r.posY / kCellUnits));
            if (gx == cx && gy == cy)
                match = true;
        }

        if (!match) continue;
        mAllRows.push_back(&r);
        mAllPoints.push_back(QPointF(r.posX, r.posY));
    }
    applyFilter();
    endResetModel();
}

void RefrTableModel::setFilter(const QString& filter)
{
    beginResetModel();
    mFilterText = filter.trimmed();
    applyFilter();
    endResetModel();
}

QString RefrTableModel::resolveBaseName(quint32 baseId, QString* outType) const
{
    if (baseId == 0 || !mData)
        return QStringLiteral("None");
    if (mBaseCache.contains(baseId))
    {
        const auto& pair = mBaseCache.value(baseId);
        if (outType) *outType = pair.second;
        return pair.first;
    }

    QString resolvedName;
    QString typeName = QStringLiteral("Reference");
    const auto typedCols = mData->allCollectionsWithTypes();
    for (const auto& tc : typedCols)
    {
        if (tc.collection && tc.collection->containsFormId(baseId))
        {
            typeName = CkId(tc.type).getTypeName();
            if (auto* baseColl = dynamic_cast<BaseCollection*>(tc.collection))
            {
                for (int i = 0; i < baseColl->size(); ++i)
                {
                    if (baseColl->getFormId(i) == baseId)
                    {
                        resolvedName = baseColl->getId(i);
                        break;
                    }
                }
            }
            break;
        }
    }

    if (resolvedName.isEmpty())
        resolvedName = QString("0x%1").arg(baseId, 8, 16, QChar('0'));
    else
        resolvedName = QString("%1 (0x%2)").arg(resolvedName).arg(baseId, 8, 16, QChar('0'));

    mBaseCache.insert(baseId, qMakePair(resolvedName, typeName));
    if (outType) *outType = typeName;
    return resolvedName;
}

void RefrTableModel::applyFilter()
{
    mRows.clear();
    mPoints.clear();
    for (int i = 0; i < static_cast<int>(mAllRows.size()); ++i)
    {
        const auto* r = mAllRows[i];
        const QString formId = QStringLiteral("0x%1").arg(r->formId, 8, 16, QChar('0'));
        if (mFilterText.isEmpty()
            || r->editorId.contains(mFilterText, Qt::CaseInsensitive)
            || formId.contains(mFilterText, Qt::CaseInsensitive)
            || resolveBaseName(r->baseId, nullptr).contains(mFilterText, Qt::CaseInsensitive))
        {
            mRows.push_back(r);
            mPoints.push_back(mAllPoints[i]);
        }
    }
}

// ============================================================================
// CellViewPanel Implementation
// ============================================================================

CellViewPanel::CellViewPanel(Data* data, QWidget* parent)
    : QWidget(parent), mData(data), mRefrContextMenu(nullptr)
{
    LOG_INFO(QStringLiteral("CellViewPanel created"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(2, 2, 2, 2);
    root->setSpacing(2);

    auto* topBar = new QHBoxLayout();
    topBar->setContentsMargins(0, 0, 0, 0);
    topBar->addWidget(new QLabel(QStringLiteral("Worldspace:"), this));
    mWorldspaceCombo = new QComboBox(this);
    mWorldspaceCombo->addItem(QStringLiteral("Interiors"), QStringLiteral("__INTERIORS__"));
    mWorldspaceCombo->addItem(QStringLiteral("(All cells)"), QStringLiteral("__ALL__"));
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

    auto* leftPane = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(2);

    mCellFilterEdit = new QLineEdit(leftPane);
    mCellFilterEdit->setPlaceholderText(QStringLiteral("Filter cells by Name/ID..."));
    mCellFilterEdit->setClearButtonEnabled(true);
    leftLayout->addWidget(mCellFilterEdit);

    mCellTable = new QTableView(leftPane);
    mCellModel = new CellTableModel(mData, this);
    mCellTable->setModel(mCellModel);
    mCellTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mCellTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mCellTable->setAlternatingRowColors(true);
    mCellTable->verticalHeader()->setVisible(false);
    mCellTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mCellTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mCellTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    mCellTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    leftLayout->addWidget(mCellTable, 1);
    splitter->addWidget(leftPane);

    auto* rightSplit = new QSplitter(Qt::Vertical, splitter);
    rightSplit->setChildrenCollapsible(false);

    mMapCanvas = new CellMapCanvas(rightSplit);
    rightSplit->addWidget(mMapCanvas);

    auto* rightBottom = new QWidget(rightSplit);
    auto* rbLayout = new QVBoxLayout(rightBottom);
    rbLayout->setContentsMargins(0, 0, 0, 0);
    rbLayout->setSpacing(2);

    mFilterEdit = new QLineEdit(rightBottom);
    mFilterEdit->setPlaceholderText(QStringLiteral("Filter references by ID/Base..."));
    mFilterEdit->setClearButtonEnabled(true);
    rbLayout->addWidget(mFilterEdit);

    mRefrTable = new QTableView(rightBottom);
    mRefrModel = new RefrTableModel(mData, this);
    mRefrTable->setModel(mRefrModel);
    mRefrTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mRefrTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mRefrTable->setAlternatingRowColors(true);
    mRefrTable->setContextMenuPolicy(Qt::CustomContextMenu);
    mRefrTable->verticalHeader()->setVisible(false);
    mRefrTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mRefrTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mRefrTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    rbLayout->addWidget(mRefrTable, 1);
    rightSplit->addWidget(rightBottom);

    splitter->addWidget(rightSplit);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    root->addWidget(splitter, 1);

    setMinimumSize(600, 400);

    mCellModel->setWorldspace(nullptr, true); // default to Interiors
    connect(mWorldspaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &CellViewPanel::onWorldspaceChanged);
    connect(mCellTable->selectionModel(), &QItemSelectionModel::currentChanged,
        this, &CellViewPanel::onCellSelected);
    connect(mCellTable, &QTableView::doubleClicked,
        this, &CellViewPanel::onCellDoubleClicked);
    connect(mRefrTable->selectionModel(), &QItemSelectionModel::currentChanged,
        this, &CellViewPanel::onRefrTableSelectionChanged);
    connect(mRefrTable, &QTableView::doubleClicked,
        this, &CellViewPanel::onRefrTableDoubleClicked);
    connect(mRefrTable, &QTableView::customContextMenuRequested,
        this, &CellViewPanel::onRefrContextMenu);

    connect(mCellFilterEdit, &QLineEdit::textChanged, this, [this](const QString& text)
    {
        if (mCellModel)
            mCellModel->setFilter(text);
    });
    connect(mFilterEdit, &QLineEdit::textChanged, this, [this](const QString& text)
    {
        if (mRefrModel)
            mRefrModel->setFilter(text);
    });

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
}

const CellRecord* CellViewPanel::currentCell() const
{
    if (!mCellTable || !mCellModel) return nullptr;
    const QModelIndex idx = mCellTable->currentIndex();
    if (!idx.isValid()) return nullptr;
    return mCellModel->recordAt(idx.row());
}

const RefrRecord* CellViewPanel::currentRef() const
{
    if (!mRefrTable || !mRefrModel) return nullptr;
    const QModelIndex idx = mRefrTable->currentIndex();
    if (!idx.isValid()) return nullptr;
    return mRefrModel->recordAt(idx.row());
}

void CellViewPanel::selectCellByFormId(quint32 cellFormId)
{
    if (!mCellModel || !mCellTable) return;
    for (int r = 0; r < mCellModel->rowCount(); ++r)
    {
        const auto* c = mCellModel->recordAt(r);
        if (c && c->formId == cellFormId)
        {
            QModelIndex idx = mCellModel->index(r, 0);
            mCellTable->setCurrentIndex(idx);
            mCellTable->scrollTo(idx);
            return;
        }
    }
}

void CellViewPanel::selectRefByFormId(quint32 refrFormId)
{
    if (!mRefrModel || !mRefrTable) return;
    for (int r = 0; r < mRefrModel->count(); ++r)
    {
        const auto* ref = mRefrModel->recordAt(r);
        if (ref && ref->formId == refrFormId)
        {
            QModelIndex idx = mRefrModel->index(r, 0);
            mRefrTable->setCurrentIndex(idx);
            mRefrTable->scrollTo(idx);
            return;
        }
    }
}

void CellViewPanel::onWorldspaceChanged(int index)
{
    if (!mData || index < 0) return;
    const QString data = mWorldspaceCombo->itemData(index).toString();

    if (data == QStringLiteral("__INTERIORS__"))
    {
        mCellModel->setWorldspace(nullptr, true);
    }
    else if (data == QStringLiteral("__ALL__"))
    {
        mCellModel->setWorldspace(nullptr, false);
    }
    else
    {
        const auto& wsc = mData->getWorldspaceCollection();
        int row = wsc.searchId(data);
        const WorldspaceRecord* ws = (row >= 0) ? &wsc.getRecord(row).get() : nullptr;
        mCellModel->setWorldspace(ws, false);
    }
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
    emit cellSelected(cell->formId);
    mRefrModel->setCell(cell);
    mMapCanvas->setReferences(mRefrModel->points());
    mMapCanvas->setCellGrid(static_cast<qint32>(cell->cellX), static_cast<qint32>(cell->cellY));
}

void CellViewPanel::onCellDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid() || !mCellModel) return;
    const auto* cell = mCellModel->recordAt(index.row());
    if (cell)
    {
        emit cellDoubleClicked(cell->formId);
    }
}

void CellViewPanel::onRefrTableDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid() || !mRefrModel) return;
    const auto* ref = mRefrModel->recordAt(index.row());
    if (ref)
    {
        emit refDoubleClicked(ref->formId);
        editSelectedRef();
    }
}

void CellViewPanel::onRefrContextMenu(const QPoint& pos)
{
    const auto* ref = currentRef();
    if (!ref) return;

    if (mRefrContextMenu)
        delete mRefrContextMenu;
    mRefrContextMenu = new QMenu(this);

    QAction* editAct = mRefrContextMenu->addAction(tr("Edit..."));
    editAct->setShortcut(QKeySequence(Qt::Key_Return));
    connect(editAct, &QAction::triggered, this, &CellViewPanel::editSelectedRef);

    QAction* dupAct = mRefrContextMenu->addAction(tr("Duplicate"));
    dupAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    connect(dupAct, &QAction::triggered, this, &CellViewPanel::duplicateSelectedRef);

    QAction* delAct = mRefrContextMenu->addAction(tr("Delete"));
    delAct->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(delAct, &QAction::triggered, this, &CellViewPanel::deleteSelectedRef);

    mRefrContextMenu->addSeparator();

    QAction* useInfoAct = mRefrContextMenu->addAction(tr("Use Info..."));
    useInfoAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    connect(useInfoAct, &QAction::triggered, this, &CellViewPanel::useInfoSelectedRef);

    mRefrContextMenu->exec(mRefrTable->viewport()->mapToGlobal(pos));
}

void CellViewPanel::editSelectedRef()
{
    const auto* ref = currentRef();
    if (!ref || !mData) return;

    auto& refrs = mData->getRefrCollection();
    int row = -1;
    for (int i = 0; i < refrs.size(); ++i)
    {
        if (refrs.getRecord(i).get().formId == ref->formId)
        {
            row = i;
            break;
        }
    }
    if (row < 0) return;

    auto session = std::make_unique<openck::TypedRecordEditSession<RefrRecord>>(
        &refrs, row, mData->getUndoStack(), QStringLiteral("Edit Placed Reference"));
    openck::QtFormDialogManager::instance().openOrFocus(
        QStringLiteral("0x%1").arg(ref->formId, 8, 16, QChar('0')),
        QStringLiteral("REFR"),
        std::move(session),
        this,
        mData);
}

void CellViewPanel::deleteSelectedRef()
{
    const auto* ref = currentRef();
    if (!ref || !mData) return;

    auto& refrs = mData->getRefrCollection();
    int row = -1;
    for (int i = 0; i < refrs.size(); ++i)
    {
        if (refrs.getRecord(i).get().formId == ref->formId)
        {
            row = i;
            break;
        }
    }
    if (row < 0) return;

    refrs.removeRecordWithUndo(ref->editorId, mData->getUndoStack());
    if (mCellModel && mCellTable)
        onCellSelected(mCellTable->currentIndex());
}

void CellViewPanel::duplicateSelectedRef()
{
    const auto* ref = currentRef();
    if (!ref || !mData) return;

    auto& refrs = mData->getRefrCollection();
    const QString newId = QString("%1_Copy").arg(ref->editorId.isEmpty() ? QStringLiteral("Ref") : ref->editorId);
    refrs.cloneRecordWithUndo(ref->editorId, newId, mData->getUndoStack());

    int newIdx = refrs.searchId(newId);
    if (newIdx >= 0)
    {
        auto& cloned = refrs.getRecord(newIdx).get();
        cloned.formId = mData->createNewRecord(CkId::Type_Refr_);
        cloned.posX += 64.0f;
        cloned.posY += 64.0f;
        const CellRecord* cell = currentCell();
        if (cell)
        {
            mData->setRefrParentCell(cloned.formId, cell->formId);
        }
    }
    if (mCellModel && mCellTable)
        onCellSelected(mCellTable->currentIndex());
}

void CellViewPanel::useInfoSelectedRef()
{
    const auto* ref = currentRef();
    if (!ref || !mData) return;

    UseInfoDialog dlg(mData, ref->baseId, ref->editorId, this);
    dlg.exec();
}

#include "cellsdialog.moc"

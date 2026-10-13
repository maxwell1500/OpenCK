#include "worldspacedialog.hpp"

#include "logger.hpp"

#include "model/world/data.hpp"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>

class WorldspaceTableModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit WorldspaceTableModel(const Data* data, QObject* parent = nullptr)
        : QAbstractItemModel(parent), mData(data)
    {
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid())
            return {};

        if (role == Qt::DisplayRole)
        {
            auto& collection = mData->getWorldspaceCollection();
            if (index.row() >= 0 && index.row() < collection.size())
            {
                const auto& rec = collection.getRecord(index.row()).get();
                switch (index.column())
                {
                case 0: return QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
                case 1: return rec.editorId;
                case 2: return rec.name;
                }
            }
        }

        return {};
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (role != Qt::DisplayRole)
            return {};
        if (orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case 0: return QStringLiteral("Form ID");
            case 1: return QStringLiteral("Editor ID");
            case 2: return QStringLiteral("Name");
            }
        }
        return {};
    }

    QModelIndex index(int row, int column, const QModelIndex& parent) const override
    {
        if (!hasIndex(row, column, parent))
            return {};
        if (row < 0 || row >= rowCount(parent))
            return {};
        return createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex& child) const override
    {
        Q_UNUSED(child)
        return {};
    }

    int rowCount(const QModelIndex& parent) const override
    {
        if (parent.isValid())
            return 0;
        return mData->getWorldspaceCollection().size();
    }

    int columnCount(const QModelIndex& parent) const override
    {
        Q_UNUSED(parent)
        return 3;
    }
private:
    const Data* mData;
};

WorldspacesDialog::WorldspacesDialog(Data* data, QWidget* parent)
    : QDialog(parent), mData(data), mTreeView(nullptr), mModel(nullptr)
{
    setWindowTitle("Worldspaces");
    setMinimumSize(600, 400);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    mTreeView = new QTreeView();
    mModel = new WorldspaceTableModel(mData, this);
    mTreeView->setModel(mModel);
    mTreeView->setRootIsDecorated(false);
    mTreeView->setAlternatingRowColors(true);
    mTreeView->header()->setStretchLastSection(true);
    mTreeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mTreeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mTreeView->header()->setSectionResizeMode(2, QHeaderView::Stretch);

    mainLayout->addWidget(mTreeView);

    auto* buttonLayout = new QHBoxLayout();
    auto* openCellViewBtn = new QPushButton(tr("Open in Cell View"));
    openCellViewBtn->setToolTip(tr("Switch the Cell View panel to display this worldspace"));
    buttonLayout->addWidget(openCellViewBtn);

    auto* editDetailsBtn = new QPushButton(tr("Edit Details..."));
    editDetailsBtn->setToolTip(tr("Edit this worldspace's properties (climate, water, land)"));
    buttonLayout->addWidget(editDetailsBtn);

    buttonLayout->addStretch();

    auto* closeBtn = new QPushButton(tr("Close"));
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);

    connect(openCellViewBtn, &QPushButton::clicked, this, &WorldspacesDialog::onOpenInCellView);
    connect(editDetailsBtn, &QPushButton::clicked, this, &WorldspacesDialog::onEditDetails);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(mTreeView, &QTreeView::doubleClicked, this, &WorldspacesDialog::onItemDoubleClicked);
}

void WorldspacesDialog::onOpenInCellView()
{
    if (!mData || !mTreeView) return;
    QModelIndex idx = mTreeView->currentIndex();
    if (!idx.isValid()) return;
    int row = idx.row();
    const auto& collection = mData->getWorldspaceCollection();
    if (row < 0 || row >= collection.size()) return;

    const auto& ws = collection.getRecord(row).get();
    QString id = ws.editorId.isEmpty() ? QStringLiteral("0x%1").arg(ws.formId, 8, 16, QChar('0')) : ws.editorId;
    emit worldspaceSelected(id);
    accept();
}

void WorldspacesDialog::onEditDetails()
{
    if (!mData || !mTreeView) return;
    QModelIndex idx = mTreeView->currentIndex();
    if (!idx.isValid()) return;
    int row = idx.row();
    auto& collection = mData->getWorldspaceCollection();
    if (row < 0 || row >= collection.size()) return;

    auto& record = collection.getRecord(row).get();
    QString formIdKey = QStringLiteral("0x%1").arg(record.formId, 8, 16, QChar('0'));
    QMessageBox::information(this, tr("Worldspace"),
        tr("Worldspace: %1 (FormID %2)\nUse the Object Window to edit full properties.")
            .arg(record.editorId, formIdKey));
}

void WorldspacesDialog::onItemDoubleClicked(const QModelIndex& index)
{
    Q_UNUSED(index);
    onOpenInCellView();
}

#include "worldspacedialog.moc"
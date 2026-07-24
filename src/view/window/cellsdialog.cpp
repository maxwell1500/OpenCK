#include "cellsdialog.hpp"

#include "logger.hpp"

#include "model/world/data.hpp"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QHeaderView>

class CellTableModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit CellTableModel(const Data* data, QObject* parent = nullptr)
        : QAbstractItemModel(parent), mData(data)
    {
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid())
            return {};

        if (role == Qt::DisplayRole)
        {
            auto& collection = mData->getCellCollection();
            if (index.row() >= 0 && index.row() < collection.size())
            {
                return collection.getRecord(index.row()).get().editorId;
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
            if (section == 0) return "Cell Name";
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
        return mData->getCellCollection().size();
    }

    int columnCount(const QModelIndex& parent) const override
    {
        Q_UNUSED(parent)
        return 1;
    }

private:
    const Data* mData;
};

CellsDialog::CellsDialog(Data* data, QWidget* parent)
    : QDialog(parent), mData(data), mTreeView(nullptr), mModel(nullptr)
{
    setWindowTitle("Cells");
    setMinimumSize(600, 400);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    mTreeView = new QTreeView();
    mModel = new CellTableModel(mData, this);
    mTreeView->setModel(mModel);
    mTreeView->setRootIsDecorated(false);
    mTreeView->setAlternatingRowColors(true);
    mTreeView->header()->setStretchLastSection(false);
    mTreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    mainLayout->addWidget(mTreeView);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* closeBtn = new QPushButton("Close");
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

#include "cellsdialog.moc"
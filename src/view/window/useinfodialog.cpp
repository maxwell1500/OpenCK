#include "useinfodialog.hpp"

#include <QHeaderView>
#include <QTreeWidgetItem>
#include "../../model/world/data.hpp"

UseInfoDialog::UseInfoDialog(Data* data, quint32 targetFormId, const QString& targetEditorId, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mTargetFormId(targetFormId),
      mTargetEditorId(targetEditorId)
{
    setWindowTitle(QString("Use Info - %1 (0x%2)")
        .arg(targetEditorId.isEmpty() ? QStringLiteral("Record") : targetEditorId)
        .arg(targetFormId, 8, 16, QChar('0')));
    resize(750, 450);

    auto* mainLayout = new QVBoxLayout(this);

    mHeaderLabel = new QLabel(this);
    mHeaderLabel->setStyleSheet(QStringLiteral("font-weight: bold; padding: 4px;"));
    mainLayout->addWidget(mHeaderLabel);

    mTreeWidget = new QTreeWidget(this);
    mTreeWidget->setHeaderLabels({
        QStringLiteral("Type"),
        QStringLiteral("Form ID"),
        QStringLiteral("Cell"),
        QStringLiteral("Editor ID"),
        QStringLiteral("Position (X, Y, Z)")
    });
    mTreeWidget->setRootIsDecorated(false);
    mTreeWidget->setAlternatingRowColors(true);
    mTreeWidget->setSortingEnabled(true);
    mTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mTreeWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    mTreeWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    mTreeWidget->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    mainLayout->addWidget(mTreeWidget);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    mOpenButton = new QPushButton(QStringLiteral("Open Selected"), this);
    mOpenButton->setEnabled(false);
    buttonLayout->addWidget(mOpenButton);

    mCloseButton = new QPushButton(QStringLiteral("Close"), this);
    buttonLayout->addWidget(mCloseButton);

    mainLayout->addLayout(buttonLayout);

    connect(mCloseButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(mOpenButton, &QPushButton::clicked, this, &UseInfoDialog::onOpenSelected);
    connect(mTreeWidget, &QTreeWidget::itemDoubleClicked, this, &UseInfoDialog::onItemDoubleClicked);
    connect(mTreeWidget, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
        if (mOpenButton)
            mOpenButton->setEnabled(current != nullptr);
    });

    populateUses();
}

QString UseInfoDialog::cellNameForFormId(quint32 cellFormId) const
{
    if (!mData || cellFormId == 0)
        return QStringLiteral("None / Exterior");

    const auto& cells = mData->getCellCollection();
    for (int i = 0; i < cells.size(); ++i)
    {
        const auto& cell = cells.getRecord(i).get();
        if (cell.formId == cellFormId)
        {
            if (!cell.editorId.isEmpty())
                return cell.editorId;
            return QString("Cell (0x%1)").arg(cellFormId, 8, 16, QChar('0'));
        }
    }
    return QString("0x%1").arg(cellFormId, 8, 16, QChar('0'));
}

void UseInfoDialog::populateUses()
{
    mTreeWidget->clear();
    mTotalCount = 0;

    if (!mData || mTargetFormId == 0)
    {
        mHeaderLabel->setText(QStringLiteral("No active document or invalid FormID"));
        return;
    }

    // 1. Placed references (REFR)
    const auto& refrs = mData->getRefrCollection();
    for (int i = 0; i < refrs.size(); ++i)
    {
        const auto& ref = refrs.getRecord(i).get();
        if (ref.baseId == mTargetFormId)
        {
            const quint32 parentCell = mData->parentCellOfRefr(ref.formId);
            auto* item = new QTreeWidgetItem(mTreeWidget);
            item->setText(0, QStringLiteral("Reference (REFR)"));
            item->setText(1, QString("0x%1").arg(ref.formId, 8, 16, QChar('0')));
            item->setText(2, cellNameForFormId(parentCell));
            item->setText(3, ref.editorId);
            item->setText(4, QString("(%1, %2, %3)").arg(ref.posX).arg(ref.posY).arg(ref.posZ));
            item->setData(0, Qt::UserRole, ref.formId);
            ++mTotalCount;
        }
    }
    mHeaderLabel->setText(QString("Uses of %1 (0x%2): %3 reference(s) found")
        .arg(mTargetEditorId.isEmpty() ? QStringLiteral("Record") : mTargetEditorId)
        .arg(mTargetFormId, 8, 16, QChar('0'))
        .arg(mTotalCount));
}

void UseInfoDialog::onItemDoubleClicked(QTreeWidgetItem* item, int)
{
    if (!item)
        return;
    const quint32 formId = item->data(0, Qt::UserRole).toUInt();
    if (formId != 0)
    {
        emit referenceDoubleClicked(formId);
        accept();
    }
}

void UseInfoDialog::onOpenSelected()
{
    QTreeWidgetItem* item = mTreeWidget->currentItem();
    if (item)
        onItemDoubleClicked(item, 0);
}

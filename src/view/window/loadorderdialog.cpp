#include "loadorderdialog.hpp"

#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

LoadOrderDialog::LoadOrderDialog(const QStringList& currentOrder, QWidget* parent)
    : QDialog(parent), currentOrder(currentOrder)
{
    LOG_DEBUG("LoadOrderDialog created");
    setWindowTitle("Load Order Management");
    setMinimumSize(600, 400);

    auto* mainLayout = new QVBoxLayout(this);
    
    loadOrderList = new QListWidget(this);
    loadOrderList->addItems(currentOrder);
    LOG_INFO(QString("Loaded %1 plugins into load order dialog").arg(currentOrder.size()));
    mainLayout->addWidget(loadOrderList);
    
    auto* buttonLayout = new QHBoxLayout();
    
    addButton = new QPushButton("Add...", this);
    moveUpButton = new QPushButton("Move Up", this);
    moveDownButton = new QPushButton("Move Down", this);
    removeButton = new QPushButton("Remove", this);
    
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(moveUpButton);
    buttonLayout->addWidget(moveDownButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    connect(addButton, &QPushButton::clicked, this, &LoadOrderDialog::addPlugin);
    connect(moveUpButton, &QPushButton::clicked, this, &LoadOrderDialog::moveUp);
    connect(moveDownButton, &QPushButton::clicked, this, &LoadOrderDialog::moveDown);
    connect(removeButton, &QPushButton::clicked, this, &LoadOrderDialog::removePlugin);
    
    LOG_DEBUG("LoadOrderDialog setup complete");
}

LoadOrderDialog::~LoadOrderDialog()
{
    LOG_DEBUG("LoadOrderDialog destroyed");
}

QStringList LoadOrderDialog::getLoadOrder() const
{
    QStringList order;
    for (int i = 0; i < loadOrderList->count(); ++i)
    {
        order << loadOrderList->item(i)->text();
    }
    LOG_INFO(QString("Returning load order with %1 plugins").arg(order.size()));
    return order;
}

void LoadOrderDialog::moveUp()
{
    LOG_DEBUG("Move up button clicked");
    int currentRow = loadOrderList->currentRow();
    if (currentRow > 0)
    {
        QListWidgetItem* currentItem = loadOrderList->takeItem(currentRow);
        loadOrderList->insertItem(currentRow - 1, currentItem);
        loadOrderList->setCurrentRow(currentRow - 1);
        LOG_INFO(QString("Moved plugin '%1' up").arg(currentItem->text()));
    }
    else
    {
        LOG_WARNING("Cannot move item up - already at top");
    }
}

void LoadOrderDialog::moveDown()
{
    LOG_DEBUG("Move down button clicked");
    int currentRow = loadOrderList->currentRow();
    int rowCount = loadOrderList->count();
    if (currentRow >= 0 && currentRow < rowCount - 1)
    {
        QListWidgetItem* currentItem = loadOrderList->takeItem(currentRow);
        loadOrderList->insertItem(currentRow + 1, currentItem);
        loadOrderList->setCurrentRow(currentRow + 1);
        LOG_INFO(QString("Moved plugin '%1' down").arg(currentItem->text()));
    }
    else
    {
        LOG_WARNING("Cannot move item down - already at bottom");
    }
}

void LoadOrderDialog::removePlugin()
{
    LOG_DEBUG("Remove button clicked");
    int currentRow = loadOrderList->currentRow();
    if (currentRow >= 0)
    {
        QListWidgetItem* currentItem = loadOrderList->takeItem(currentRow);
        LOG_INFO(QString("Removed plugin: %1").arg(currentItem->text()));
        delete currentItem;
    }
    else
    {
        LOG_WARNING("No plugin selected for removal");
    }
}

void LoadOrderDialog::addPlugin()
{
    LOG_DEBUG("Add plugin button clicked");
    QString fileName = QFileDialog::getOpenFileName(this, "Add Plugin", QString(), "ESM/ESP Files (*.esm *.esp);;All Files (*)");
    if (!fileName.isEmpty())
    {
        loadOrderList->addItem(fileName);
        LOG_INFO(QString("Added plugin: %1").arg(fileName));
    }
    else
    {
        LOG_DEBUG("No plugin selected");
    }
}

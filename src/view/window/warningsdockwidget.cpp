#include "warningsdockwidget.hpp"

#include "../../model/doc/messages.hpp"

#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

WarningsDockWidget::WarningsDockWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    mTable = new QTableWidget(this);
    mTable->setColumnCount(3);
    mTable->setHorizontalHeaderLabels({"Level", "Message", "Record"});
    mTable->horizontalHeader()->setStretchLastSection(true);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTable->verticalHeader()->setVisible(false);
    layout->addWidget(mTable);
}

void WarningsDockWidget::addMessage(const Message& message)
{
    const int row = mTable->rowCount();
    mTable->insertRow(row);

    auto* levelItem = new QTableWidgetItem(Message::toString(message.level));
    auto* msgItem = new QTableWidgetItem(message.message);
    auto* recordItem = new QTableWidgetItem(message.id.getId());

    if (message.level == Message::Error || message.level == Message::Critical)
        levelItem->setForeground(QColor(200, 60, 60));
    else if (message.level == Message::Warning)
        levelItem->setForeground(QColor(200, 160, 40));

    mTable->setItem(row, 0, levelItem);
    mTable->setItem(row, 1, msgItem);
    mTable->setItem(row, 2, recordItem);
}

void WarningsDockWidget::setMessages(const Messages& messages)
{
    clear();
    for (auto it = messages.begin(); it != messages.end(); ++it)
        addMessage(*it);
}

void WarningsDockWidget::clear()
{
    mTable->setRowCount(0);
}

int WarningsDockWidget::count() const
{
    return mTable->rowCount();
}

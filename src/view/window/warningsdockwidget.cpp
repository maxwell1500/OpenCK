#include "warningsdockwidget.hpp"

#include "../../model/doc/messages.hpp"
#include "../../model/tools/reports.hpp"

#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

WarningsDockWidget::WarningsDockWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void WarningsDockWidget::setupUI()
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
    layout->addWidget(mTable, 1);

    auto* buttonLayout = new QHBoxLayout();
    mExportButton = new QPushButton(tr("Export Report..."), this);
    mExportButton->setToolTip(tr("Write the validation messages to a tab-separated report file"));
    buttonLayout->addWidget(mExportButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    connect(mExportButton, &QPushButton::clicked, this, &WarningsDockWidget::onExport);
}

void WarningsDockWidget::addMessage(const Message& message)
{
    mStoredMessages.append(message);

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
    mStoredMessages.clear();
    mTable->setRowCount(0);
}

int WarningsDockWidget::count() const
{
    return mTable->rowCount();
}

void WarningsDockWidget::onExport()
{
    if (mStoredMessages.isEmpty())
    {
        QMessageBox::information(this, tr("Export Report"),
            tr("No validation messages to export.\nRun Validate first."));
        return;
    }

    const QString path = QFileDialog::getSaveFileName(this, tr("Export Validation Report"),
        QString(), tr("Tab-Separated Report (*.tsv *.txt);;All Files (*)"));
    if (path.isEmpty())
        return;

    if (ReportExport::exportMessages(path, mStoredMessages))
    {
        QMessageBox::information(this, tr("Export Report"),
            tr("Report written to:\n%1\n\n%2 message(s)").arg(path).arg(mStoredMessages.size()));
    }
    else
    {
        QMessageBox::warning(this, tr("Export Report"),
            tr("Could not write the report file."));
    }
}

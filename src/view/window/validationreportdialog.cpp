#include "validationreportdialog.hpp"
#include "logger.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QFont>

ValidationReportDialog::ValidationReportDialog(const AssetValidator::ValidationReport& report,
                                                 QWidget* parent)
    : QDialog(parent),
      mReport(report),
      mTable(new QTableWidget(this)),
      mSummaryLabel(new QLabel(this)),
      mFilterErrorsBtn(new QToolButton(this)),
      mFilterWarningsBtn(new QToolButton(this)),
      mFilterInfosBtn(new QToolButton(this)),
      mClearFiltersBtn(new QPushButton(tr("Clear Filters"), this)),
      mShowErrors(true),
      mShowWarnings(true),
      mShowInfos(true)
{
    setWindowTitle("Asset Validation Report");
    setMinimumSize(800, 500);

    // Filter buttons
    mFilterErrorsBtn->setText(QString("Errors (%1)").arg(report.errors()));
    mFilterErrorsBtn->setCheckable(true);
    mFilterErrorsBtn->setChecked(true);
    mFilterErrorsBtn->setStyleSheet("QToolButton { color: red; font-weight: bold; }");

    mFilterWarningsBtn->setText(QString("Warnings (%1)").arg(report.warnings()));
    mFilterWarningsBtn->setCheckable(true);
    mFilterWarningsBtn->setChecked(true);
    mFilterWarningsBtn->setStyleSheet("QToolButton { color: orange; font-weight: bold; }");

    mFilterInfosBtn->setText(QString("Info (%1)").arg(report.infos()));
    mFilterInfosBtn->setCheckable(true);
    mFilterInfosBtn->setChecked(true);
    mFilterInfosBtn->setStyleSheet("QToolButton { color: blue; }");

    // Layout
    auto* mainLayout = new QVBoxLayout(this);

    // Filter bar
    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(mFilterErrorsBtn);
    filterLayout->addWidget(mFilterWarningsBtn);
    filterLayout->addWidget(mFilterInfosBtn);
    filterLayout->addWidget(mClearFiltersBtn);
    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);

    // Summary
    mSummaryLabel->setText(QString("%1 errors, %2 warnings, %3 infos")
        .arg(report.errors()).arg(report.warnings()).arg(report.infos()));
    mainLayout->addWidget(mSummaryLabel);

    // Table
    mTable->setColumnCount(5);
    mTable->setHorizontalHeaderLabels({"Severity", "Category", "Message", "Record", "File"});
    mTable->horizontalHeader()->setStretchLastSection(true);
    mTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTable->verticalHeader()->setVisible(false);

    mainLayout->addWidget(mTable);

    // Button row
    auto* btnLayout = new QHBoxLayout();
    auto* exportBtn = new QPushButton(tr("Export Report..."), this);
    btnLayout->addStretch();
    btnLayout->addWidget(exportBtn);

    auto* okBtn = new QPushButton(tr("Close"), this);
    btnLayout->addWidget(okBtn);
    mainLayout->addLayout(btnLayout);

    // Connections
    connect(mFilterErrorsBtn, &QToolButton::clicked, this, &ValidationReportDialog::onFilterErrors);
    connect(mFilterWarningsBtn, &QToolButton::clicked, this, &ValidationReportDialog::onFilterWarnings);
    connect(mFilterInfosBtn, &QToolButton::clicked, this, &ValidationReportDialog::onFilterInfos);
    connect(mClearFiltersBtn, &QPushButton::clicked, this, &ValidationReportDialog::onClearFilters);
    connect(exportBtn, &QPushButton::clicked, this, &ValidationReportDialog::onExportReport);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(mTable, &QTableWidget::cellDoubleClicked, this, &ValidationReportDialog::onRowDoubleClicked);

    populateTable();
}

ValidationReportDialog::~ValidationReportDialog()
{
}

void ValidationReportDialog::populateTable()
{
    mTable->setRowCount(mReport.issues.size());
    mRowVisible.resize(mReport.issues.size(), true);

    for (int i = 0; i < mReport.issues.size(); i++)
    {
        const auto& issue = mReport.issues.at(i);

        // Severity icon
        QString severityText;
        QColor severityColor;
        switch (issue.severity)
        {
        case AssetValidator::ValidationIssue::Error:
            severityText = "Error";
            severityColor = Qt::red;
            break;
        case AssetValidator::ValidationIssue::Warning:
            severityText = "Warning";
            severityColor = QColor(255, 165, 0);
            break;
        case AssetValidator::ValidationIssue::Info:
            severityText = "Info";
            severityColor = Qt::blue;
            break;
        }

        auto* severityItem = new QTableWidgetItem(severityText);
        severityItem->setForeground(severityColor);
        QFont boldFont = severityItem->font();
        boldFont.setBold(issue.severity == AssetValidator::ValidationIssue::Error);
        severityItem->setFont(boldFont);
        mTable->setItem(i, 0, severityItem);

        mTable->setItem(i, 1, new QTableWidgetItem(issue.category));
        mTable->setItem(i, 2, new QTableWidgetItem(issue.message));
        mTable->setItem(i, 3, new QTableWidgetItem(issue.recordId));
        mTable->setItem(i, 4, new QTableWidgetItem(issue.filePath));
    }

    applyFilters();
}

void ValidationReportDialog::updateSummaryLabel()
{
    int visibleErrors = 0, visibleWarnings = 0, visibleInfos = 0;
    for (int i = 0; i < mReport.issues.size(); i++)
    {
        if (!mRowVisible.at(i)) continue;
        const auto& issue = mReport.issues.at(i);
        switch (issue.severity)
        {
        case AssetValidator::ValidationIssue::Error: visibleErrors++; break;
        case AssetValidator::ValidationIssue::Warning: visibleWarnings++; break;
        case AssetValidator::ValidationIssue::Info: visibleInfos++; break;
        }
    }

    mSummaryLabel->setText(QString("Showing: %1 errors, %2 warnings, %3 infos (of %4 total)")
        .arg(visibleErrors).arg(visibleWarnings).arg(visibleInfos).arg(mReport.issues.size()));
}

void ValidationReportDialog::applyFilters()
{
    for (int i = 0; i < mReport.issues.size(); i++)
    {
        const auto& issue = mReport.issues.at(i);
        bool visible = true;

        switch (issue.severity)
        {
        case AssetValidator::ValidationIssue::Error:
            visible = mShowErrors;
            break;
        case AssetValidator::ValidationIssue::Warning:
            visible = mShowWarnings;
            break;
        case AssetValidator::ValidationIssue::Info:
            visible = mShowInfos;
            break;
        }

        mRowVisible[i] = visible;
        mTable->setRowHidden(i, !visible);
    }

    updateSummaryLabel();
}

void ValidationReportDialog::onFilterErrors()
{
    mShowErrors = mFilterErrorsBtn->isChecked();
    applyFilters();
}

void ValidationReportDialog::onFilterWarnings()
{
    mShowWarnings = mFilterWarningsBtn->isChecked();
    applyFilters();
}

void ValidationReportDialog::onFilterInfos()
{
    mShowInfos = mFilterInfosBtn->isChecked();
    applyFilters();
}

void ValidationReportDialog::onClearFilters()
{
    mShowErrors = true;
    mShowWarnings = true;
    mShowInfos = true;
    mFilterErrorsBtn->setChecked(true);
    mFilterWarningsBtn->setChecked(true);
    mFilterInfosBtn->setChecked(true);
    applyFilters();
}

void ValidationReportDialog::onRowDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row < 0 || row >= mReport.issues.size())
        return;

    const auto& issue = mReport.issues.at(row);
    if (!issue.recordId.isEmpty())
    {
        emit navigateToRecord(issue.recordId);
    }
}

void ValidationReportDialog::onExportReport()
{
    QString filePath = QFileDialog::getSaveFileName(this,
        "Export Validation Report", "", "Text Files (*.txt);;CSV Files (*.csv)");
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Export Error", "Cannot write to file: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    out << "Asset Validation Report\n";
    out << "========================\n\n";
    out << mSummaryLabel->text() << "\n\n";

    for (int i = 0; i < mReport.issues.size(); i++)
    {
        if (!mRowVisible.at(i)) continue;
        const auto& issue = mReport.issues.at(i);

        QString severity;
        switch (issue.severity)
        {
        case AssetValidator::ValidationIssue::Error: severity = "ERROR"; break;
        case AssetValidator::ValidationIssue::Warning: severity = "WARNING"; break;
        case AssetValidator::ValidationIssue::Info: severity = "INFO"; break;
        }

        out << QString("[%1] [%2] %3").arg(severity, issue.category, issue.message);
        if (!issue.recordId.isEmpty())
            out << " (Record: " << issue.recordId << ")";
        if (!issue.filePath.isEmpty())
            out << " (File: " << issue.filePath << ")";
        out << "\n";
    }

    file.close();
    LOG_INFO(QString("Validation report exported to %1").arg(filePath));
}

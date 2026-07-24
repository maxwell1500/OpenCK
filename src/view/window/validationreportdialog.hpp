#pragma once

#include "../../model/tools/assetvalidator.hpp"

#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVector>

class ValidationReportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ValidationReportDialog(const AssetValidator::ValidationReport& report,
                                     QWidget* parent = nullptr);
    ~ValidationReportDialog();

signals:
    void navigateToRecord(const QString& recordId);

private slots:
    void onRowDoubleClicked(int row, int column);
    void onFilterErrors();
    void onFilterWarnings();
    void onFilterInfos();
    void onExportReport();
    void onClearFilters();

private:
    void populateTable();
    void updateSummaryLabel();
    void applyFilters();

    AssetValidator::ValidationReport mReport;
    QVector<bool> mRowVisible;

    QTableWidget* mTable;
    QLabel* mSummaryLabel;
    QToolButton* mFilterErrorsBtn;
    QToolButton* mFilterWarningsBtn;
    QToolButton* mFilterInfosBtn;
    QPushButton* mClearFiltersBtn;

    bool mShowErrors;
    bool mShowWarnings;
    bool mShowInfos;
};

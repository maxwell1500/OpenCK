#include "pluginmergedialog.hpp"
#include "../../model/tools/pluginmerger.hpp"

#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

using openck::MergeWriteReport;

PluginMergeDialog::PluginMergeDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Three-Way Plugin Merge"));
    setMinimumSize(700, 520);
    buildUi();
}

PluginMergeDialog::~PluginMergeDialog() = default;

void PluginMergeDialog::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    auto* grid = new QGridLayout();

    auto addRow = [&](int row, const QString& label, QLineEdit* edit,
                      void (PluginMergeDialog::*slot)()) {
        grid->addWidget(new QLabel(label, this), row, 0);
        grid->addWidget(edit, row, 1);
        auto* btn = new QPushButton(tr("Browse..."), this);
        connect(btn, &QPushButton::clicked, this, slot);
        grid->addWidget(btn, row, 2);
    };

    mBaseEdit = new QLineEdit(this);
    mBaseEdit->setPlaceholderText(tr("common ancestor plugin"));
    addRow(0, tr("Base:"), mBaseEdit, &PluginMergeDialog::onBrowseBase);

    mMineEdit = new QLineEdit(this);
    mMineEdit->setPlaceholderText(tr("your branch's plugin"));
    addRow(1, tr("Mine:"), mMineEdit, &PluginMergeDialog::onBrowseMine);

    mTheirsEdit = new QLineEdit(this);
    mTheirsEdit->setPlaceholderText(tr("the other modder's plugin"));
    addRow(2, tr("Theirs:"), mTheirsEdit, &PluginMergeDialog::onBrowseTheirs);

    mOutputEdit = new QLineEdit(this);
    mOutputEdit->setPlaceholderText(tr("merged output plugin"));
    addRow(3, tr("Output:"), mOutputEdit, &PluginMergeDialog::onBrowseOutput);

    mMergeBtn = new QPushButton(tr("Merge"), this);
    grid->addWidget(mMergeBtn, 4, 1);
    connect(mMergeBtn, &QPushButton::clicked, this, &PluginMergeDialog::onMerge);
    layout->addLayout(grid);

    mReport = new QTextEdit(this);
    mReport->setReadOnly(true);
    layout->addWidget(mReport);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn);
}

void PluginMergeDialog::onBrowseBase()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Base Plugin"), QString(), tr("Plugins (*.esm *.esp);;All Files (*)"));
    if (!path.isEmpty())
        mBaseEdit->setText(path);
}

void PluginMergeDialog::onBrowseMine()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("My Plugin"), QString(), tr("Plugins (*.esm *.esp);;All Files (*)"));
    if (!path.isEmpty())
        mMineEdit->setText(path);
}

void PluginMergeDialog::onBrowseTheirs()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Their Plugin"), QString(), tr("Plugins (*.esm *.esp);;All Files (*)"));
    if (!path.isEmpty())
        mTheirsEdit->setText(path);
}

void PluginMergeDialog::onBrowseOutput()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Output Plugin"), QString(), tr("Plugins (*.esm *.esp)"));
    if (!path.isEmpty())
        mOutputEdit->setText(path);
}

void PluginMergeDialog::onMerge()
{
    const QString base = mBaseEdit->text().trimmed();
    const QString mine = mMineEdit->text().trimmed();
    const QString theirs = mTheirsEdit->text().trimmed();
    const QString output = mOutputEdit->text().trimmed();

    for (const QString& p : { base, mine, theirs })
    {
        if (p.isEmpty() || !QFileInfo::exists(p))
        {
            QMessageBox::warning(this, tr("Three-Way Merge"),
                                 tr("Base, mine and theirs must all be existing plugin files."));
            return;
        }
    }
    if (output.isEmpty())
    {
        QMessageBox::warning(this, tr("Three-Way Merge"),
                             tr("Pick an output plugin path."));
        return;
    }

    const MergeWriteReport report =
        openck::PluginMerger::mergeToFile(base, mine, theirs, output);

    if (!report.ok())
    {
        mReport->setPlainText(report.error);
        QMessageBox::warning(this, tr("Three-Way Merge"), report.error);
        return;
    }

    QString text = tr("Wrote %1\n%2 record(s) written, %3 conflict(s) kept from mine, "
                      "%4 record(s) deleted by one side.")
                       .arg(report.outputPath)
                       .arg(report.recordsWritten)
                       .arg(report.conflictsRetained)
                       .arg(report.skipped);
    if (!report.conflictReasons.isEmpty())
    {
        text += tr("\n\nConflicts to resolve:\n");
        for (const QString& reason : report.conflictReasons)
            text += tr("  - %1\n").arg(reason);
    }
    else
    {
        text += tr("\nNo conflicts: every differing record was touched by only one side.");
    }
    mReport->setPlainText(text);
}

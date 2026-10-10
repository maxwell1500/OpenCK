#include "recorddiffdialog.hpp"
#include "../../model/tools/plugindiff.hpp"

#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

using openck::DiffStatus;
using openck::PluginDiffReport;
using openck::RecordDiffEntry;

namespace {

QString subName(const openck::SubDiff& sub)
{
    return QString::fromLatin1(openck::snapshotName(sub.name).toUtf8());
}

// Printable preview of a payload: ASCII runs kept, the rest as dots.
QString preview(const QByteArray& payload)
{
    QString out;
    out.reserve(payload.size());
    for (char c : payload)
    {
        const unsigned char u = static_cast<unsigned char>(c);
        out.append((u >= 0x20 && u < 0x7f) ? QChar(u) : QChar(u'.'));
    }
    return out;
}

QString hexOf(const QByteArray& payload, int limit = 64)
{
    const QByteArray view = payload.left(limit);
    return QString::fromLatin1(view.toHex());
}

QString statusColor(DiffStatus s)
{
    switch (s)
    {
        case DiffStatus::Added:    return QStringLiteral("#2E7D32");
        case DiffStatus::Removed:  return QStringLiteral("#C62828");
        case DiffStatus::Modified: return QStringLiteral("#F9A825");
        case DiffStatus::Same:     break;
    }
    return QStringLiteral("#9E9E9E");
}

} // namespace

RecordDiffDialog::RecordDiffDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Record Diff"));
    setMinimumSize(900, 600);
    buildUi();
}

RecordDiffDialog::~RecordDiffDialog() = default;

void RecordDiffDialog::buildUi()
{
    auto* layout = new QVBoxLayout(this);

    auto* picker = new QGridLayout();
    picker->addWidget(new QLabel(tr("Left (base):"), this), 0, 0);
    mLeftEdit = new QLineEdit(this);
    picker->addWidget(mLeftEdit, 0, 1);
    auto* leftBtn = new QPushButton(tr("Browse..."), this);
    picker->addWidget(leftBtn, 0, 2);
    connect(leftBtn, &QPushButton::clicked, this, &RecordDiffDialog::onBrowseLeft);

    picker->addWidget(new QLabel(tr("Right (edited):"), this), 1, 0);
    mRightEdit = new QLineEdit(this);
    picker->addWidget(mRightEdit, 1, 1);
    auto* rightBtn = new QPushButton(tr("Browse..."), this);
    picker->addWidget(rightBtn, 1, 2);
    connect(rightBtn, &QPushButton::clicked, this, &RecordDiffDialog::onBrowseRight);

    mCompareBtn = new QPushButton(tr("Compare"), this);
    picker->addWidget(mCompareBtn, 2, 1);
    connect(mCompareBtn, &QPushButton::clicked, this, &RecordDiffDialog::onCompare);
    layout->addLayout(picker);

    mSummaryLabel = new QLabel(tr("Pick two plugins and press Compare."), this);
    mSummaryLabel->setWordWrap(true);
    layout->addWidget(mSummaryLabel);

    auto* split = new QSplitter(Qt::Vertical, this);

    mRecordTree = new QTreeWidget(split);
    mRecordTree->setColumnCount(3);
    mRecordTree->setHeaderLabels({ tr("Record"), tr("Status"), tr("Editor ID") });
    mRecordTree->setRootIsDecorated(false);
    connect(mRecordTree, &QTreeWidget::itemClicked, this,
        [this](QTreeWidgetItem* item, int column) { onRecordSelected(item, column); });

    mDetailTree = new QTreeWidget(split);
    mDetailTree->setColumnCount(4);
    mDetailTree->setHeaderLabels(
        { tr("Subrecord"), tr("Status"), tr("Left"), tr("Right") });
    mDetailTree->setRootIsDecorated(false);
    split->addWidget(mRecordTree);
    split->addWidget(mDetailTree);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 3);
    layout->addWidget(split);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn);
}

void RecordDiffDialog::onBrowseLeft()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Left Plugin"), QString(), tr("Plugins (*.esm *.esp);;All Files (*)"));
    if (!path.isEmpty())
        mLeftEdit->setText(path);
}

void RecordDiffDialog::onBrowseRight()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Right Plugin"), QString(), tr("Plugins (*.esm *.esp);;All Files (*)"));
    if (!path.isEmpty())
        mRightEdit->setText(path);
}

void RecordDiffDialog::onCompare()
{
    const QString left = mLeftEdit->text().trimmed();
    const QString right = mRightEdit->text().trimmed();
    if (left.isEmpty() || right.isEmpty()
        || !QFileInfo::exists(left) || !QFileInfo::exists(right))
    {
        QMessageBox::warning(this, tr("Record Diff"),
                             tr("Pick two existing plugin files."));
        return;
    }

    const PluginDiffReport report = openck::PluginDiffer::diff(left, right);
    fillSummary(report);
    fillRecords(report);
    mDetailTree->clear();
}

void RecordDiffDialog::fillSummary(const PluginDiffReport& report)
{
    if (!report.ok())
    {
        mSummaryLabel->setText(report.error);
        return;
    }
    mSummaryLabel->setText(tr("%1 change(s): %2 modified, %3 added, %4 removed "
                              "(%5 records identical)")
        .arg(report.modifiedRecords + report.addedRecords + report.removedRecords)
        .arg(report.modifiedRecords)
        .arg(report.addedRecords)
        .arg(report.removedRecords)
        .arg(report.sameRecords));
}

void RecordDiffDialog::fillRecords(const PluginDiffReport& report)
{
    mRecordTree->clear();
    for (const RecordDiffEntry& entry : report.records)
    {
        auto* item = new QTreeWidgetItem(mRecordTree);
        item->setText(0, QStringLiteral("%1 0x%2")
            .arg(QString::fromLatin1(openck::snapshotName(entry.type).toUtf8()),
                 QString::number(entry.formId, 16).rightJustified(8, QChar('0'))));
        item->setText(1, openck::diffStatusName(entry.status));
        item->setText(2, entry.editorId);
        item->setForeground(1, QBrush(QColor(statusColor(entry.status))));
        item->setData(0, Qt::UserRole, QVariant::fromValue(entry.formId));
        item->setData(0, Qt::UserRole + 1,
                      QVariant::fromValue(static_cast<int>(entry.type)));
    }
    mRecordTree->resizeColumnToContents(0);
    mRecordTree->resizeColumnToContents(1);
}

void RecordDiffDialog::onRecordSelected(QTreeWidgetItem* item, int)
{
    if (!item)
        return;
    mDetailTree->clear();

    const quint32 formId = item->data(0, Qt::UserRole).toUInt();
    const NAME type = static_cast<NAME>(item->data(0, Qt::UserRole + 1).toInt());

    const PluginDiffReport report =
        openck::PluginDiffer::diff(mLeftEdit->text().trimmed(),
                                   mRightEdit->text().trimmed());
    for (const RecordDiffEntry& entry : report.records)
    {
        if (entry.formId != formId || entry.type != type)
            continue;

        for (const openck::SubDiff& sub : entry.subs)
        {
            auto* row = new QTreeWidgetItem(mDetailTree);
            row->setText(0, subName(sub));
            row->setText(1, openck::diffStatusName(sub.status));
            row->setText(2, tr("%1  %2").arg(hexOf(sub.left), preview(sub.left)));
            row->setText(3, tr("%1  %2").arg(hexOf(sub.right), preview(sub.right)));
            row->setForeground(1, QBrush(QColor(statusColor(sub.status))));
        }
        if (entry.subs.isEmpty())
        {
            auto* row = new QTreeWidgetItem(mDetailTree);
            row->setText(1, openck::diffStatusName(entry.status));
            if (entry.status == DiffStatus::Added)
                row->setText(3, tr("(record present only on the right)"));
            if (entry.status == DiffStatus::Removed)
                row->setText(2, tr("(record present only on the left)"));
        }
        break;
    }
    for (int i = 0; i < mDetailTree->columnCount(); ++i)
        mDetailTree->resizeColumnToContents(i);
}

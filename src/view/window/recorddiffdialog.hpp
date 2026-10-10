#ifndef RECORDDIFFDIALOG_HPP
#define RECORDDIFFDIALOG_HPP

#include <QDialog>
#include <QList>

class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

#include "../../model/tools/plugindiff.hpp"

// Phase 10.1: side-by-side visual record diff between two plugins.
//
// Top table lists every differing record (color-coded Added/Removed/
// Modified); expanding a record shows its subrecord alignment with the two
// payloads side by side (hex + printable preview) so a reviewer sees what
// actually changed without opening the Creation Kit.
class RecordDiffDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecordDiffDialog(QWidget* parent = nullptr);
    ~RecordDiffDialog() override;

private slots:
    void onBrowseLeft();
    void onBrowseRight();
    void onCompare();
    void onRecordSelected(QTreeWidgetItem* item, int column);

private:
    void buildUi();
    void fillSummary(const openck::PluginDiffReport& report);
    void fillRecords(const openck::PluginDiffReport& report);

    QLineEdit* mLeftEdit = nullptr;
    QLineEdit* mRightEdit = nullptr;
    QPushButton* mCompareBtn = nullptr;
    QLabel* mSummaryLabel = nullptr;
    QTreeWidget* mRecordTree = nullptr;
    QTreeWidgetItem* mDetailItem = nullptr;
    QTreeWidget* mDetailTree = nullptr;
};

#endif // RECORDDIFFDIALOG_HPP

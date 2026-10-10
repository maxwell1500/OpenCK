#ifndef PLUGINMERGEDIALOG_HPP
#define PLUGINMERGEDIALOG_HPP

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QVBoxLayout;

// Phase 10.3: three-way plugin merge UI (base / mine / theirs -> output),
// reporting every conflict kept for manual resolution.
class PluginMergeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PluginMergeDialog(QWidget* parent = nullptr);
    ~PluginMergeDialog() override;

private slots:
    void onBrowseBase();
    void onBrowseMine();
    void onBrowseTheirs();
    void onBrowseOutput();
    void onMerge();

private:
    void buildUi();

    QLineEdit* mBaseEdit = nullptr;
    QLineEdit* mMineEdit = nullptr;
    QLineEdit* mTheirsEdit = nullptr;
    QLineEdit* mOutputEdit = nullptr;
    QPushButton* mMergeBtn = nullptr;
    QTextEdit* mReport = nullptr;
};

#endif // PLUGINMERGEDIALOG_HPP

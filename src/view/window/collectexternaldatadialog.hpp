#pragma once

#include <QDialog>
#include <QString>
#include <atomic>
#include <memory>

class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QTreeWidget;
class QProgressBar;
class Data;

// Shows what a plugin depends on that it does not carry, and gathers those files
// into a folder that can ship with the mod.
//
// It is a window in its own right rather than a panel in the archive browser,
// because it is useful on its own and because a user with two monitors can keep
// it beside the browser. It therefore stays usable while the browser is closed,
// and closing it does not disturb the browser.
//
// Nothing here blocks the underlying analysis: the plan is built once on open and
// can be rebuilt after an option changes, because building it scans every
// archive in the data directory and must not happen on a toggle.
class CollectExternalDataDialog : public QDialog
{
    Q_OBJECT

public:
    // `dataDir` is the game's data root; `pluginProvider` supplies the plugin,
    // which may be null -- in which case the dialog explains that a plugin must
    // be open rather than showing an empty plan.
    CollectExternalDataDialog(const QString& dataDir,
                              std::function<Data*()> pluginProvider,
                              const QString& toolIniPath = QString(),
                              QWidget* parent = nullptr);
    ~CollectExternalDataDialog() override;

    // Exposed for tests: the plan as the dialog currently displays it, so the
    // wiring can be checked without driving the widgets.
    int collectableCount() const;

private slots:
    void rebuild();
    void chooseDestination();
    void collect();
    void onCancel();
    void onProgress(int done, int total, const QString& current);
    void onFinished(int written, const QStringList& failures, bool cancelled);

private:
    void setBusy(bool busy);
    void refreshSummary();

    QString mDataDir;
    std::function<Data*()> mPluginProvider;
    QString mToolIniPath;
    // Kept so the plan can be rebuilt after an option changes without rescanning
    // to find the plugin again.
    QVector<struct AssetReference> mReferences;

    QLabel* mSummary = nullptr;
    QTreeWidget* mTree = nullptr;
    QCheckBox* mIgnoreArchivedBox = nullptr;
    QCheckBox* mResourceOnlyBox = nullptr;
    QLineEdit* mDestination = nullptr;
    QPushButton* mBrowseBtn = nullptr;
    QPushButton* mCollectBtn = nullptr;
    QPushButton* mCancelBtn = nullptr;
    QPushButton* mCloseBtn = nullptr;
    QProgressBar* mProgress = nullptr;
    // Shared with the worker thread, which outlives a single collect() call so a
    // second run cannot read a freed flag.
    std::shared_ptr<std::atomic<bool>> mCancelFlag;
};
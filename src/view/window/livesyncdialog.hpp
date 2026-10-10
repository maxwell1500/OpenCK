#ifndef LIVESYNCDIALOG_HPP
#define LIVESYNCDIALOG_HPP

#include <QDialog>

class QLabel;
class QListWidget;
class QPushButton;
class QVBoxLayout;
class BlenderBridge;

// Phase 12.2: live-sync status window for Blender sessions.
//
// One row per asset currently open in Blender: what is being watched, the
// last export's verification state, and the actions that can move a
// verified export into the plugin. Commit stays explicit — a verified
// export only means it parses, not that the artist is finished with it.
class LiveSyncDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LiveSyncDialog(BlenderBridge* bridge, QWidget* parent = nullptr);
    ~LiveSyncDialog() override;

private slots:
    void refresh();
    void onExportChanged(const QString& path);
    void onExportVerified(const QString& path, qint64 size);
    void onExportRejected(const QString& path, const QString& reason);
    void onCommitted(const QString& assetPath, const QString& fromExport);
    void onCommit();
    void onStopWatching();

private:
    void buildUi();

    BlenderBridge* mBridge;
    QListWidget* mList;
    QLabel* mDetailLabel;
    QPushButton* mCommitBtn;
    QPushButton* mStopBtn;
};

#endif // LIVESYNCDIALOG_HPP

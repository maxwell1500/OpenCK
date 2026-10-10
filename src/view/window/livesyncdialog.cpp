#include "livesyncdialog.hpp"
#include "../../model/tools/blenderbridge.hpp"

#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

LiveSyncDialog::LiveSyncDialog(BlenderBridge* bridge, QWidget* parent)
    : QDialog(parent), mBridge(bridge)
{
    setWindowTitle(tr("Blender Live-Sync"));
    setMinimumWidth(520);
    buildUi();

    connect(mBridge, &BlenderBridge::exportChanged, this, &LiveSyncDialog::onExportChanged);
    connect(mBridge, &BlenderBridge::exportVerified, this, &LiveSyncDialog::onExportVerified);
    connect(mBridge, &BlenderBridge::exportRejected, this, &LiveSyncDialog::onExportRejected);
    connect(mBridge, &BlenderBridge::committed, this, &LiveSyncDialog::onCommitted);
    refresh();
}

LiveSyncDialog::~LiveSyncDialog() = default;

void LiveSyncDialog::buildUi()
{
    auto* layout = new QVBoxLayout(this);

    mList = new QListWidget(this);
    layout->addWidget(mList);

    mDetailLabel = new QLabel(tr("No session. Right-click a record with a 3D model "
                                  "and choose \"Open in Blender (Live Sync)...\"."), this);
    mDetailLabel->setWordWrap(true);
    layout->addWidget(mDetailLabel);

    mCommitBtn = new QPushButton(tr("Commit Verified Export to Asset"), this);
    mCommitBtn->setEnabled(false);
    connect(mCommitBtn, &QPushButton::clicked, this, &LiveSyncDialog::onCommit);
    layout->addWidget(mCommitBtn);

    mStopBtn = new QPushButton(tr("Stop Watching"), this);
    mStopBtn->setEnabled(false);
    connect(mStopBtn, &QPushButton::clicked, this, &LiveSyncDialog::onStopWatching);
    layout->addWidget(mStopBtn);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn);
}

void LiveSyncDialog::refresh()
{
    mList->clear();
    if (!mBridge || !mBridge->hasSession())
    {
        mDetailLabel->setText(tr("No Blender session is being watched."));
        mCommitBtn->setEnabled(false);
        mStopBtn->setEnabled(false);
        return;
    }

    const BlenderBridge::AssetContext& context = mBridge->context();
    QStringList rows;
    rows << tr("Mesh: %1").arg(context.nifPath);
    rows << (context.skeletonPath.isEmpty()
                 ? tr("Skeleton: (none resolved)")
                 : tr("Skeleton: %1").arg(context.skeletonPath));
    rows << (context.collisionPaths.isEmpty()
                 ? tr("Collision: (none found)")
                 : tr("Collision: %1").arg(context.collisionPaths.size()));
    rows << tr("Watching: %1").arg(mBridge->outputPath());
    mList->addItems(rows);

    mCommitBtn->setEnabled(true);
    mStopBtn->setEnabled(true);
}

void LiveSyncDialog::onExportChanged(const QString& path)
{
    mDetailLabel->setText(tr("Export changed: %1 — verifying...")
                              .arg(QFileInfo(path).fileName()));
}

void LiveSyncDialog::onExportVerified(const QString& path, qint64 size)
{
    mDetailLabel->setText(tr("Export verified (%1 bytes, parses as a NIF): %2")
                              .arg(size)
                              .arg(QFileInfo(path).fileName()));
    mCommitBtn->setEnabled(true);
}

void LiveSyncDialog::onExportRejected(const QString& path, const QString& reason)
{
    // The original asset was never touched; say so explicitly, because a
    // failed round trip is exactly when a modder wonders what state their
    // plugin is in.
    mDetailLabel->setText(tr("Export rejected (%1): %2. The plugin asset is unchanged.")
                              .arg(QFileInfo(path).fileName(), reason));
    mCommitBtn->setEnabled(false);
}

void LiveSyncDialog::onCommitted(const QString& assetPath, const QString&)
{
    mDetailLabel->setText(tr("Committed to %1").arg(assetPath));
    mCommitBtn->setEnabled(false);
}

void LiveSyncDialog::onCommit()
{
    if (!mBridge || !mBridge->hasSession())
        return;
    QString error;
    if (!mBridge->commit(&error))
    {
        QMessageBox::warning(this, tr("Blender Live-Sync"),
                             tr("Commit failed: %1").arg(error));
        return;
    }
    refresh();
}

void LiveSyncDialog::onStopWatching()
{
    if (mBridge)
        mBridge->stop();
    refresh();
}

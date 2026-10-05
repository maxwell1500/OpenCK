#include "collectexternaldatadialog.hpp"

#include "model/tools/assetdependencyscanner.hpp"
#include "model/tools/assetresolver.hpp"
#include "model/tools/externaldatacollector.hpp"
#include "ba2/resourcearchiveconfig.hpp"
#include "../../model/world/data.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QThread>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <atomic>

namespace {

// Clamps a window to the screen it is actually opening on. Multi-screen is the
// reason this exists: a hardcoded size that suits a 27" panel is unusable on a
// laptop, and a window restored onto a monitor that no longer exists must not be
// left off-screen. Uses the screen under the requested centre when it is known,
// and the primary screen otherwise, so a single-screen setup behaves exactly as
// before -- just bounded.
void fitToScreen(QWidget* widget, int desiredWidth, int desiredHeight)
{
    QScreen* screen = nullptr;
    if (QApplication::primaryScreen() == nullptr)
        return;
    // The screen the widget would appear on: under its own centre once it has a
    // position, otherwise the primary.
    if (widget->geometry().isValid() && !widget->geometry().isNull()) {
        const QPoint centre = widget->geometry().center();
        if (QScreen* under = QGuiApplication::screenAt(centre))
            screen = under;
    }
    if (!screen)
        screen = widget->screen() ? widget->screen() : QApplication::primaryScreen();
    const QRect available = screen->availableGeometry();
    widget->resize(qMin(desiredWidth, available.width()),
                   qMin(desiredHeight, available.height()));
    // Only move the window when it would otherwise land off-screen; a window the
    // user positioned deliberately is left alone.
    if (!available.intersects(widget->geometry()))
        widget->move(available.topLeft());
}

// Runs the write half off the GUI thread so the progress bar animates and Cancel
// is answered. Building the plan is not threaded: it is a scan the browser
// already performs synchronously when it lists an archive.
class CollectWorker : public QObject
{
    Q_OBJECT
public:
    CollectWorker(ExternalDataCollector::Plan plan, QString destination,
                  std::shared_ptr<std::atomic<bool>> cancel)
        : mPlan(std::move(plan)), mDestination(std::move(destination))
        , mCancel(std::move(cancel))
    {
    }

public slots:
    void run()
    {
        auto cancelled = [this]() { return mCancel->load(std::memory_order_relaxed); };
        // This runs on the worker thread, so emitting here is automatically
        // delivered to the dialog on the GUI thread by Qt::AutoConnection.
        const auto outcome = ExternalDataCollector::collect(
            mPlan, mDestination, cancelled,
            [this](int done, int total, const QString& current) {
                emit progress(done, total, current);
            });
        emit finished(outcome.written, outcome.failures, outcome.cancelled);
    }

signals:
    void progress(int done, int total, const QString& current);
    void finished(int written, const QStringList& failures, bool cancelled);

private:
    ExternalDataCollector::Plan mPlan;
    QString mDestination;
    std::shared_ptr<std::atomic<bool>> mCancel;
};

} // namespace

#include "collectexternaldatadialog.moc"

CollectExternalDataDialog::CollectExternalDataDialog(const QString& dataDir,
                                                     std::function<Data*()> pluginProvider,
                                                     const QString& toolIniPath,
                                                     QWidget* parent)
    : QDialog(parent)
    , mDataDir(dataDir)
    , mPluginProvider(std::move(pluginProvider))
    , mToolIniPath(toolIniPath)
{
    setWindowTitle(tr("Collect External Data"));
    setAttribute(Qt::WA_DeleteOnClose, false);

    auto* layout = new QVBoxLayout(this);

    mSummary = new QLabel(this);
    mSummary->setWordWrap(true);
    layout->addWidget(mSummary);

    mTree = new QTreeWidget(this);
    mTree->setObjectName(QStringLiteral("planTree"));
    mTree->setColumnCount(4);
    mTree->setHeaderLabels({ tr("Asset"), tr("State"), tr("Source"), tr("Referenced by") });
    mTree->setRootIsDecorated(false);
    mTree->setUniformRowHeights(true);
    mTree->setSortingEnabled(true);
    layout->addWidget(mTree, 1);

    auto* options = new QHBoxLayout();
    mIgnoreArchivedBox = new QCheckBox(tr("Ignore files already present inside archives"), this);
    mIgnoreArchivedBox->setObjectName(QStringLiteral("ignoreArchivedBox"));
    mIgnoreArchivedBox->setToolTip(
        tr("Leave off when the game's own archives are not part of what you ship, "
           "which is the usual case for a mod."));
    mResourceOnlyBox = new QCheckBox(tr("Only collect from the game's own archives"), this);
    mResourceOnlyBox->setObjectName(QStringLiteral("resourceOnlyBox"));
    mResourceOnlyBox->setChecked(true);
    mResourceOnlyBox->setToolTip(
        tr("Requires a tool configuration file naming the game's resource archives. "
           "Without one, every archive that provides a file is considered fair game."));
    options->addWidget(mIgnoreArchivedBox);
    options->addWidget(mResourceOnlyBox);
    options->addStretch();
    layout->addLayout(options);

    auto* destRow = new QHBoxLayout();
    mDestination = new QLineEdit(this);
    mDestination->setObjectName(QStringLiteral("destinationEdit"));
    mBrowseBtn = new QPushButton(tr("Browse..."), this);
    destRow->addWidget(new QLabel(tr("Collect into:"), this));
    destRow->addWidget(mDestination, 1);
    destRow->addWidget(mBrowseBtn);
    layout->addLayout(destRow);

    mProgress = new QProgressBar(this);
    mProgress->setObjectName(QStringLiteral("progressBar"));
    mProgress->setVisible(false);
    layout->addWidget(mProgress);

    auto* buttons = new QHBoxLayout();
    mCollectBtn = new QPushButton(tr("Collect"), this);
    mCollectBtn->setObjectName(QStringLiteral("collectBtn"));
    mCancelBtn = new QPushButton(tr("Cancel"), this);
    mCancelBtn->setObjectName(QStringLiteral("cancelBtn"));
    mCancelBtn->setVisible(false);
    mCloseBtn = new QPushButton(tr("Close"), this);
    buttons->addWidget(mCollectBtn);
    buttons->addWidget(mCancelBtn);
    buttons->addStretch();
    buttons->addWidget(mCloseBtn);
    layout->addLayout(buttons);

    connect(mBrowseBtn, &QPushButton::clicked, this, &CollectExternalDataDialog::chooseDestination);
    connect(mCollectBtn, &QPushButton::clicked, this, &CollectExternalDataDialog::collect);
    connect(mCancelBtn, &QPushButton::clicked, this, &CollectExternalDataDialog::onCancel);
    connect(mCloseBtn, &QPushButton::clicked, this, &QDialog::close);
    connect(mIgnoreArchivedBox, &QCheckBox::toggled, this, &CollectExternalDataDialog::rebuild);
    connect(mResourceOnlyBox, &QCheckBox::toggled, this, &CollectExternalDataDialog::rebuild);

    // Geometry is per-window state worth keeping, and the main window already
    // persists its own the same way.
    QSettings settings;
    settings.beginGroup(QStringLiteral("CollectExternalDataDialog"));
    const QByteArray saved = settings.value(QStringLiteral("geometry")).toByteArray();
    if (!saved.isEmpty())
        restoreGeometry(saved);
    fitToScreen(this, 820, 560);
    settings.endGroup();

    rebuild();
}

CollectExternalDataDialog::~CollectExternalDataDialog()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("CollectExternalDataDialog"));
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    settings.endGroup();
}

int CollectExternalDataDialog::collectableCount() const
{
    int n = 0;
    for (int i = 0; i < mTree->topLevelItemCount(); ++i) {
        if (mTree->topLevelItem(i)->data(0, Qt::UserRole).toInt() == 1)
            ++n;
        }
    return n;
}

void CollectExternalDataDialog::rebuild()
{
    mTree->clear();
    mReferences.clear();

    Data* plugin = mPluginProvider ? mPluginProvider() : nullptr;
    if (!plugin) {
        mSummary->setText(tr("Open a plugin first. Collecting external data works from "
                             "the assets a plugin references, so there is nothing to "
                             "collect without one."));
        mCollectBtn->setEnabled(false);
        return;
    }
    mCollectBtn->setEnabled(true);

    ResourceArchiveConfig config;
    if (!mToolIniPath.isEmpty())
        config = ResourceArchiveConfig::fromIni(mToolIniPath);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    AssetResolver resolver(mDataDir);
    mReferences = AssetDependencyScanner::collectReferences(*plugin);
    ExternalDataCollector::Options options;
    options.ignoreFilesInsideArchives = mIgnoreArchivedBox->isChecked();
    options.resourceArchivesOnly = mResourceOnlyBox->isChecked();
    const auto plan = ExternalDataCollector::buildPlan(
        mReferences, resolver, options,
        options.resourceArchivesOnly && !config.isEmpty() ? &config : nullptr);
    QApplication::restoreOverrideCursor();

    const auto addRows = [this](const QVector<ExternalDataCollector::Item>& items, int state) {
        for (const ExternalDataCollector::Item& item : items) {
            auto* row = new QTreeWidgetItem(mTree);
            row->setText(0, item.assetPath);
            row->setText(2, item.sourceArchive.isEmpty()
                                ? QString()
                                : QFileInfo(item.sourceArchive).fileName());
            row->setText(3, item.referencedBy.join(QStringLiteral(", ")));
            row->setData(0, Qt::UserRole, state);
        }
    };
    addRows(plan.toCollect, 1);
    addRows(plan.coveredByArchives, 2);
    addRows(plan.alreadyLoose, 3);
    addRows(plan.inNonResourceArchive, 4);
    addRows(plan.unavailable, 5);

    for (int i = 0; i < mTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* row = mTree->topLevelItem(i);
        switch (row->data(0, Qt::UserRole).toInt()) {
        case 1: row->setText(1, tr("will be collected")); break;
        case 2: row->setText(1, tr("covered by an archive you are shipping")); break;
        case 3: row->setText(1, tr("already loose")); break;
        case 4: row->setText(1, tr("only in another mod's archive")); break;
        default: row->setText(1, tr("not found anywhere")); break;
        }
        // The state is in column 1 as text for reading, but sorting by it would
        // be alphabetical, so the sort key is the numeric state.
        row->setData(1, Qt::UserRole, row->data(0, Qt::UserRole));
    }
    mTree->resizeColumnToContents(0);
    refreshSummary();
}

void CollectExternalDataDialog::refreshSummary()
{
    int collect = 0, covered = 0, loose = 0, other = 0, missing = 0;
    for (int i = 0; i < mTree->topLevelItemCount(); ++i) {
        switch (mTree->topLevelItem(i)->data(0, Qt::UserRole).toInt()) {
        case 1: ++collect; break;
        case 2: ++covered; break;
        case 3: ++loose; break;
        case 4: ++other; break;
        default: ++missing; break;
        }
    }
    QString text = tr("%1 of %2 referenced assets need collecting: %3 already loose, "
                      "%4 covered by an archive, %5 in another mod's archive, %6 missing.")
                       .arg(collect).arg(collect + covered + loose + other + missing)
                       .arg(loose).arg(covered).arg(other).arg(missing);
    if (collect == 0)
        text += tr("  Nothing to do.");
    mSummary->setText(text);
    mCollectBtn->setEnabled(collect > 0);
}

void CollectExternalDataDialog::chooseDestination()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Collect Into"), mDestination->text());
    if (!dir.isEmpty())
        mDestination->setText(dir);
}

void CollectExternalDataDialog::setBusy(bool busy)
{
    mCollectBtn->setEnabled(!busy && collectableCount() > 0);
    mBrowseBtn->setEnabled(!busy);
    mDestination->setEnabled(!busy);
    mIgnoreArchivedBox->setEnabled(!busy);
    mResourceOnlyBox->setEnabled(!busy);
    mProgress->setVisible(busy);
}

void CollectExternalDataDialog::collect()
{
    Data* plugin = mPluginProvider ? mPluginProvider() : nullptr;
    if (!plugin) return;
    const QString destination = mDestination->text().trimmed();
    if (destination.isEmpty()) {
        QMessageBox::information(this, tr("Collect External Data"),
                                 tr("Choose a folder to collect into first."));
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    AssetResolver resolver(mDataDir);
    ResourceArchiveConfig config;
    if (!mToolIniPath.isEmpty())
        config = ResourceArchiveConfig::fromIni(mToolIniPath);
    ExternalDataCollector::Options options;
    options.ignoreFilesInsideArchives = mIgnoreArchivedBox->isChecked();
    options.resourceArchivesOnly = mResourceOnlyBox->isChecked();
    const auto plan = ExternalDataCollector::buildPlan(
        AssetDependencyScanner::collectReferences(*plugin), resolver, options,
        options.resourceArchivesOnly && !config.isEmpty() ? &config : nullptr);
    QApplication::restoreOverrideCursor();

    if (plan.toCollect.isEmpty()) {
        QMessageBox::information(this, tr("Collect External Data"),
                                 tr("There is nothing to collect with these options."));
        return;
    }

    // Rebuild the plan at collect time rather than reusing the displayed one: the
    // user may have changed a filter or added files since it was built, and
    // writing a stale plan would quietly do the wrong thing.
    // Rebuild the plan at collect time rather than reusing the displayed one: the
    // user may have changed the filter or added files since it was built, and
    // writing a stale plan would quietly do the wrong thing.
    setBusy(true);
    mProgress->setRange(0, plan.toCollect.size());
    mProgress->setValue(0);
    mProgress->setFormat(tr("%v of %m"));
    mCancelBtn->setVisible(true);
    mCancelBtn->setEnabled(true);

    mCancelFlag = std::make_shared<std::atomic<bool>>(false);
    auto* worker = new CollectWorker(plan, destination, mCancelFlag);
    auto* thread = new QThread(this);

    // The worker lives on its own thread, so both connections are explicitly
    // queued: without that, `finished` would run on the worker thread and touch
    // widgets from there.
    connect(worker, &CollectWorker::progress, this,
            &CollectExternalDataDialog::onProgress, Qt::QueuedConnection);
    connect(worker, &CollectWorker::finished, this,
            &CollectExternalDataDialog::onFinished, Qt::QueuedConnection);
    connect(worker, &CollectWorker::finished, thread, &QThread::quit);
    connect(worker, &CollectWorker::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    worker->moveToThread(thread);
    connect(thread, &QThread::started, worker, &CollectWorker::run);
    thread->start();
}

void CollectExternalDataDialog::onCancel()
{
    if (mCancelFlag)
        mCancelFlag->store(true, std::memory_order_relaxed);
    mCancelBtn->setEnabled(false);
    // The worker only checks between files, so say what will actually happen
    // rather than pretending the click took effect immediately.
    mProgress->setFormat(tr("Stopping after the current file..."));
}

void CollectExternalDataDialog::onProgress(int done, int total, const QString& current)
{
    if (total > 0) {
        mProgress->setRange(0, total);
        mProgress->setValue(done);
    }
    // The file being written, so a long run is legible rather than a bare bar.
    if (!current.isEmpty())
        mProgress->setFormat(current);
}

void CollectExternalDataDialog::onFinished(int written, const QStringList& failures,
                                           bool cancelled)
{
    mCancelBtn->setVisible(false);
    mCancelFlag.reset();
    setBusy(false);
    mProgress->setFormat(QString());
    if (cancelled) {
        QMessageBox::information(this, tr("Collection Cancelled"),
                                 tr("Stopped early. %n file(s) were written before "
                                    "cancelling; partial output has been left in place.",
                                    nullptr, written));
    } else if (!failures.isEmpty()) {
        QMessageBox::warning(this, tr("Collection Incomplete"),
                             tr("%1 file(s) written, %2 failed.\n\n%3")
                                 .arg(written).arg(failures.size())
                                 .arg(failures.mid(0, 12).join(QStringLiteral("\n"))));
    } else {
        QMessageBox::information(this, tr("Collection Complete"),
                                 tr("%n file(s) collected.", nullptr, written));
    }
}

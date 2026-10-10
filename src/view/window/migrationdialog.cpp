#include "migrationdialog.hpp"
#include "../../model/tools/archiveconverter.hpp"
#include "../../model/tools/recordmigrator.hpp"
#include "../../model/doc/document.hpp"
#include "../../libs/files/log/logger.hpp"

#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

MigrationDialog::MigrationDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Cross-Game Migration"));
    setMinimumWidth(640);
    setupUi();
}

MigrationDialog::~MigrationDialog() = default;

void MigrationDialog::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    mTabs = new QTabWidget(this);

    // --- Archives ---
    auto* archiveTab = new QWidget(mTabs);
    auto* archiveLayout = new QGridLayout(archiveTab);

    archiveLayout->addWidget(new QLabel(tr("Source archive:"), archiveTab), 0, 0);
    mArchiveSourceEdit = new QLineEdit(archiveTab);
    mArchiveSourceEdit->setPlaceholderText(tr("path to a .bsa or .ba2"));
    archiveLayout->addWidget(mArchiveSourceEdit, 0, 1);
    auto* browseArchive = new QPushButton(tr("Browse..."), archiveTab);
    archiveLayout->addWidget(browseArchive, 0, 2);
    connect(browseArchive, &QPushButton::clicked, this,
            &MigrationDialog::onBrowseArchiveSource);

    archiveLayout->addWidget(new QLabel(tr("Target game:"), archiveTab), 1, 0);
    mArchiveTargetCombo = new QComboBox(archiveTab);
    mArchiveTargetCombo->addItem(
        GameFormat::gameName(GameFormat::Game::Oblivion),
        static_cast<int>(GameFormat::Game::Oblivion));
    mArchiveTargetCombo->addItem(
        GameFormat::gameName(GameFormat::Game::Skyrim),
        static_cast<int>(GameFormat::Game::Skyrim));
    mArchiveTargetCombo->addItem(
        GameFormat::gameName(GameFormat::Game::Fallout4),
        static_cast<int>(GameFormat::Game::Fallout4));
    mArchiveTargetCombo->addItem(
        QStringLiteral("BA2 (GNRL)"),
        static_cast<int>(GameFormat::Game::Starfield));
    archiveLayout->addWidget(mArchiveTargetCombo, 1, 1);

    mArchiveConvertBtn = new QPushButton(tr("Convert"), archiveTab);
    archiveLayout->addWidget(mArchiveConvertBtn, 2, 1);
    connect(mArchiveConvertBtn, &QPushButton::clicked, this,
            &MigrationDialog::onConvertArchive);

    mArchiveResultLabel = new QLabel(tr("-"), archiveTab);
    mArchiveResultLabel->setWordWrap(true);
    archiveLayout->addWidget(mArchiveResultLabel, 3, 0, 1, 3);

    // --- Records ---
    auto* recordTab = new QWidget(mTabs);
    auto* recordLayout = new QGridLayout(recordTab);

    recordLayout->addWidget(new QLabel(tr("Source plugin:"), recordTab), 0, 0);
    mRecordSourceEdit = new QLineEdit(recordTab);
    mRecordSourceEdit->setPlaceholderText(tr("path to a .esm/.esp to read"));
    recordLayout->addWidget(mRecordSourceEdit, 0, 1);
    auto* browseRecord = new QPushButton(tr("Browse..."), recordTab);
    recordLayout->addWidget(browseRecord, 0, 2);
    connect(browseRecord, &QPushButton::clicked, this,
            &MigrationDialog::onBrowseRecordSource);

    recordLayout->addWidget(new QLabel(tr("Target game:"), recordTab), 1, 0);
    mRecordTargetCombo = new QComboBox(recordTab);
    for (GameFormat::Game game : { GameFormat::Game::Oblivion,
                                   GameFormat::Game::Skyrim,
                                   GameFormat::Game::Fallout4,
                                   GameFormat::Game::Starfield })
    {
        mRecordTargetCombo->addItem(GameFormat::gameName(game),
                                    static_cast<int>(game));
    }
    recordLayout->addWidget(mRecordTargetCombo, 1, 1);

    recordLayout->addWidget(new QLabel(tr("Output plugin:"), recordTab), 2, 0);
    mRecordOutputEdit = new QLineEdit(recordTab);
    mRecordOutputEdit->setPlaceholderText(tr("path to the new .esp"));
    recordLayout->addWidget(mRecordOutputEdit, 2, 1);
    auto* browseOutput = new QPushButton(tr("Browse..."), recordTab);
    recordLayout->addWidget(browseOutput, 2, 2);
    connect(browseOutput, &QPushButton::clicked, this,
            &MigrationDialog::onBrowseRecordOutput);

    mRecordConvertBtn = new QPushButton(tr("Migrate"), recordTab);
    recordLayout->addWidget(mRecordConvertBtn, 3, 1);
    connect(mRecordConvertBtn, &QPushButton::clicked, this,
            &MigrationDialog::onConvertRecords);

    mRecordResultLabel = new QLabel(tr("-"), recordTab);
    mRecordResultLabel->setWordWrap(true);
    recordLayout->addWidget(mRecordResultLabel, 4, 0, 1, 3);

    mTabs->addTab(archiveTab, tr("Archives"));
    mTabs->addTab(recordTab, tr("Records"));
    layout->addWidget(mTabs);

    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn);
}

void MigrationDialog::onBrowseArchiveSource()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Source Archive"), QString(),
        tr("Archives (*.bsa *.ba2);;All Files (*)"));
    if (!path.isEmpty())
    {
        mArchiveSourceEdit->setText(path);
    }
}

void MigrationDialog::onBrowseArchiveOutput()
{
    // Kept for symmetry with the other pickers; conversion derives the
    // output path from the source.
}

void MigrationDialog::onConvertArchive()
{
    const QString source = mArchiveSourceEdit->text().trimmed();
    if (source.isEmpty() || !QFileInfo::exists(source))
    {
        QMessageBox::warning(this, tr("Cross-Game Migration"),
                             tr("Pick an existing source archive."));
        return;
    }

    const GameFormat::Game target = static_cast<GameFormat::Game>(
        mArchiveTargetCombo->currentData().toInt());
    const bool wantsBa2 = target == GameFormat::Game::Starfield;
    const QFileInfo sourceInfo(source);
    const QString output = sourceInfo.absolutePath() + "/"
        + sourceInfo.completeBaseName()
        + (wantsBa2 ? QStringLiteral(".ba2") : QStringLiteral(".bsa"));

    ArchiveConversionReport report = wantsBa2
        ? ArchiveConverter::convertBsaToBa2(source, output)
        : ArchiveConverter::convertBsa(source, target, output);

    mArchiveResultLabel->setText(
        tr("Output: %1\n%2").arg(output, report.summary()));
    if (!report.ok())
    {
        QMessageBox::warning(this, tr("Cross-Game Migration"),
                             tr("Conversion reported problems:\n%1")
                                 .arg(report.failures.join("\n")));
    }
    LOG_INFO(QString("MigrationDialog: archive conversion %1").arg(report.summary()));
}

void MigrationDialog::onBrowseRecordSource()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Source Plugin"), QString(),
        tr("Plugins (*.esm *.esp);;All Files (*)"));
    if (!path.isEmpty())
    {
        mRecordSourceEdit->setText(path);
    }
}

void MigrationDialog::onBrowseRecordOutput()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Output Plugin"), QString(),
        tr("Plugins (*.esm *.esp)"));
    if (!path.isEmpty())
    {
        mRecordOutputEdit->setText(path);
    }
}

void MigrationDialog::onConvertRecords()
{
    const QString source = mRecordSourceEdit->text().trimmed();
    const QString output = mRecordOutputEdit->text().trimmed();
    if (source.isEmpty() || !QFileInfo::exists(source))
    {
        QMessageBox::warning(this, tr("Cross-Game Migration"),
                             tr("Pick an existing source plugin."));
        return;
    }
    if (output.isEmpty())
    {
        QMessageBox::warning(this, tr("Cross-Game Migration"),
                             tr("Pick an output plugin path."));
        return;
    }

    const GameFormat::Game target = static_cast<GameFormat::Game>(
        mRecordTargetCombo->currentData().toInt());
    const QString sourceName = QFileInfo(source).fileName();

    // Load the source the way the editor does: preload, then drain.
    Document sourceDoc{ QStringList{ sourceName }, source, false };
    {
        Data& data = sourceDoc.getData();
        if (data.preload(sourceName, false) <= 0)
        {
            QMessageBox::critical(this, tr("Cross-Game Migration"),
                                  tr("Could not preload %1.").arg(sourceName));
            return;
        }
        Messages messages(Message::Default);
        int guard = 0;
        while (!data.continueLoading(messages))
        {
            if (++guard > 5000000)
            {
                QMessageBox::critical(this, tr("Cross-Game Migration"),
                                      tr("Loading %1 did not converge.")
                                          .arg(sourceName));
                return;
            }
        }
    }

    // A new plugin in the target game's family receives the records; the
    // output starts with no masters (a plugin of its own).
    NewPluginOptions options;
    options.game = target;
    options.author = QStringLiteral("OpenCK migration");
    options.nextObjectId = 0x800;
    Document destDoc{ QStringList(), output, true, options };

    const MigrationReport report =
        RecordMigrator::migrate(sourceDoc.getData(), destDoc.getData());

    destDoc.save(output);

    mRecordResultLabel->setText(
        tr("Wrote %1\n%2 record(s) migrated, %3 skipped")
            .arg(output)
            .arg(report.migrated.size())
            .arg(report.skipped.size()));
    LOG_INFO(QString("MigrationDialog: migrated %1 record(s) into %2")
                 .arg(report.migrated.size()).arg(output));
}
